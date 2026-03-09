//
// Created by Administrator on 2026/3/9.
//

#include "oboe.h"

#include "soundio_private.h"
#include "oboe/Oboe.h"

static const int kChannelCount = 2;

// static const int OUTPUT_ELEMENT = 0;
// static const int INPUT_ELEMENT = 1;

static enum SoundIoDeviceAim aims[] = {
    SoundIoDeviceAimInput,
    SoundIoDeviceAimOutput,
};

static void deinit_sound_io_devices_info(struct SoundIoDevicesInfo** sidi)
{
    if (*sidi == NULL)
    {
        return;
    }
    free(*sidi);
    *sidi = NULL;
}


static int refresh_devices(struct SoundIoPrivate* si)
{
    struct SoundIo* soundio = &si->pub;
    struct SoundIoOboe* sio = &si->backend_data.oboe;

    int err;

    struct SoundIoDevicesInfo* devices_info = NULL;

    if (!(devices_info = ALLOCATE(struct SoundIoDevicesInfo, 1)))
    {
        return SoundIoErrorNoMem;
    }

    devices_info->default_output_index = 0;
    devices_info->default_input_index = 0;

    // AVAudioSession* session = AVAudioSession.sharedInstance;

    for (int aim_i = 0; aim_i < ARRAY_LENGTH(aims); aim_i += 1)
    {
        enum SoundIoDeviceAim aim = aims[aim_i];
        struct SoundIoDevicePrivate* dev = ALLOCATE(struct SoundIoDevicePrivate, 1);

        if (!dev)
        {
            deinit_sound_io_devices_info(&devices_info);
            return SoundIoErrorNoMem;
        }

        struct SoundIoDevice* device = &dev->pub;

        device->ref_count = 1;
        device->soundio = soundio;
        device->is_raw = false;
        device->aim = aim;

        device->layout_count = 1;
        device->layouts = &device->current_layout;
        // device->current_layout.channel_count = aim == SoundIoDeviceAimOutput ? (int) session.outputNumberOfChannels : (int) session.inputNumberOfChannels;
        device->current_layout.channel_count = 2; // 定死双通道
        device->format_count = 1;
        device->formats = ALLOCATE(enum SoundIoFormat, device->format_count);

        if (!device->formats)
        {
            free(dev);
            deinit_sound_io_devices_info(&devices_info);
            return SoundIoErrorNoMem;
        }

        device->formats[0] = SoundIoFormatS32LE;
        // device->sample_rate_current = session.sampleRate;
        device->sample_rate_current = 0; // 系统给什么用什么
        device->sample_rate_count = 1;
        device->sample_rates = &dev->prealloc_sample_rate_range;
        device->sample_rates[0].min = device->sample_rate_current;
        device->sample_rates[0].max = device->sample_rate_current;

        // device->software_latency_current = aim == SoundIoDeviceAimOutput ? session.outputLatency : session.inputLatency;
        device->software_latency_current = 0.1; // 默认十毫秒
        device->software_latency_min = 0.01;
        device->software_latency_max = 1;

        struct SoundIoListDevicePtr* device_list;
        if (device->aim == SoundIoDeviceAimOutput)
        {
            device_list = &devices_info->output_devices;
        }
        else
        {
            assert(device->aim == SoundIoDeviceAimInput);
            device_list = &devices_info->input_devices;
        }

        if ((err = SoundIoListDevicePtr_append(device_list, device)))
        {
            free(dev);
            deinit_sound_io_devices_info(&devices_info);
            return err;
        }
    }

    soundio_os_mutex_lock(sio->mutex);
    soundio_destroy_devices_info(sio->ready_devices_info);
    sio->ready_devices_info = devices_info;
    soundio_os_mutex_unlock(sio->mutex);

    return 0;
}

static void shutdown_backend(struct SoundIoPrivate* si, int err)
{
    struct SoundIo* soundio = &si->pub;
    struct SoundIoOboe* sio = &si->backend_data.oboe;
    soundio_os_mutex_lock(sio->mutex);
    sio->shutdown_err = err;
    SOUNDIO_ATOMIC_STORE(sio->have_devices_flag, true);
    soundio_os_mutex_unlock(sio->mutex);
    soundio_os_cond_signal(sio->cond, NULL);
    soundio_os_cond_signal(sio->have_devices_cond, NULL);
    soundio->on_events_signal(soundio);
}

static void device_thread_run(void* arg)
{
    struct SoundIoPrivate* si = (struct SoundIoPrivate*) arg;
    struct SoundIo* soundio = &si->pub;
    struct SoundIoOboe* sio = &si->backend_data.oboe;
    int err;

    for (;;)
    {
        if (!SOUNDIO_ATOMIC_FLAG_TEST_AND_SET(sio->abort_flag))
            break;
        if (SOUNDIO_ATOMIC_LOAD(sio->service_restarted))
        {
            shutdown_backend(si, SoundIoErrorBackendDisconnected);
            return;
        }
        if (SOUNDIO_ATOMIC_EXCHANGE(sio->device_scan_queued, false))
        {
            err = refresh_devices(si);
            if (err)
            {
                shutdown_backend(si, err);
            }

            if (!SOUNDIO_ATOMIC_EXCHANGE(sio->have_devices_flag, true))
                soundio_os_cond_signal(sio->have_devices_cond, NULL);
            soundio_os_cond_signal(sio->cond, NULL);
            soundio->on_events_signal(soundio);
        }
        soundio_os_cond_wait(sio->scan_devices_cond, NULL);
    }
}


static void flush_events_oboe(struct SoundIoPrivate* si)
{
    struct SoundIo* soundio = &si->pub;
    struct SoundIoOboe* sio = &si->backend_data.oboe;

    // block until have devices
    while (!SOUNDIO_ATOMIC_LOAD(sio->have_devices_flag))
        soundio_os_cond_wait(sio->have_devices_cond, NULL);

    bool change = false;
    bool cb_shutdown = false;
    struct SoundIoDevicesInfo* old_devices_info = NULL;

    soundio_os_mutex_lock(sio->mutex);

    if (sio->shutdown_err && !sio->emitted_shutdown_cb)
    {
        sio->emitted_shutdown_cb = true;
        cb_shutdown = true;
    }
    else if (sio->ready_devices_info)
    {
        old_devices_info = si->safe_devices_info;
        si->safe_devices_info = sio->ready_devices_info;
        sio->ready_devices_info = NULL;
        change = true;
    }

    soundio_os_mutex_unlock(sio->mutex);

    if (cb_shutdown)
        soundio->on_backend_disconnect(soundio, sio->shutdown_err);
    else if (change)
        soundio->on_devices_change(soundio);

    soundio_destroy_devices_info(old_devices_info);
}

static void wait_events_oboe(struct SoundIoPrivate* si)
{
    struct SoundIoOboe* sio = &si->backend_data.oboe;
    flush_events_oboe(si);
    soundio_os_cond_wait(sio->cond, NULL);
}

static void wakeup_oboe(struct SoundIoPrivate* si)
{
    struct SoundIoOboe* sio = &si->backend_data.oboe;
    soundio_os_cond_signal(sio->cond, NULL);
}

static void force_device_scan_oboe(struct SoundIoPrivate* si)
{
    struct SoundIoOboe* sio = &si->backend_data.oboe;
    SOUNDIO_ATOMIC_STORE(sio->device_scan_queued, true);
    soundio_os_cond_signal(sio->scan_devices_cond, NULL);
}

static void outstream_destroy_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os)
{
    struct SoundIoOutStreamOboe* oso = &os->backend_data.oboe;

    if (oso->output_stream)
    {
        auto* stream = oso->output_stream;
        stream->requestStop();
        stream->close();
        oso->output_stream = nullptr;
    }

    if (oso->callback)
    {
        delete oso->callback;
        oso->callback = nullptr;
    }
}

static int outstream_open_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os)
{
    struct SoundIoOutStreamOboe* oso = &os->backend_data.oboe;
    struct SoundIoOutStream* outstream = &os->pub;
    struct SoundIoDevice* device = outstream->device;
    struct SoundIoDevicePrivate* dev = (struct SoundIoDevicePrivate*) device;
    struct SoundIoDeviceOboe* sdo = &dev->backend_data.oboe;

    if (outstream->software_latency == 0.0)
    {
        outstream->software_latency = device->software_latency_current;
    }

    outstream->software_latency = soundio_double_clamp(
        device->software_latency_min,
        outstream->software_latency,
        device->software_latency_max);

    // Allocate the callback object on the heap
    oso->callback = new oboe_callback(os);

    oboe::AudioStreamBuilder builder;
    builder.setSharingMode(oboe::SharingMode::Exclusive)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setChannelCount(kChannelCount)
            ->setFormat(oboe::AudioFormat::Float)
            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
            ->setDataCallback(oso->callback);

    oboe::AudioStream* stream = nullptr;
    oboe::Result result = builder.openStream(&stream);
    if (result != oboe::Result::OK)
    {
        outstream_destroy_oboe(si, os);
        return SoundIoErrorOpeningDevice;
    }
    oso->output_stream = stream;

    outstream->sample_rate = stream->getHardwareSampleRate();
    outstream->bytes_per_frame = stream->getBytesPerFrame();
    outstream->bytes_per_sample = stream->getBytesPerSample();
    sdo->latency_frames = stream->calculateLatencyMillis().value();

    os->backend_data.oboe.volume = 1;
    oso->hardware_latency = sdo->latency_frames / static_cast<double>(outstream->sample_rate);

    return 0;
}

static int outstream_pause_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os, bool pause)
{
    struct SoundIoOutStreamOboe* osca = &os->backend_data.oboe;
    oboe::AudioStream* stream = static_cast<oboe::AudioStream*>(osca->output_stream);
    oboe::Result result;
    if (pause)
    {
        result = stream->requestPause();
        if (result != oboe::Result::OK)
        {
            return SoundIoErrorStreaming;
        }
    }
    else
    {
        result = stream->requestFlush();
        if (result != oboe::Result::OK)
        {
            return SoundIoErrorStreaming;
        }
        result = stream->requestStart();
        if (result != oboe::Result::OK)
        {
            return SoundIoErrorStreaming;
        }
    }

    return 0;
}

static int outstream_start_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os)
{
    return outstream_pause_oboe(si, os, false);
}


static int outstream_begin_write_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os,
                                      struct SoundIoChannelArea** out_areas, int* frame_count)
{
    struct SoundIoOutStream* outstream = &os->pub;
    struct SoundIoOutStreamOboe* oso = &os->backend_data.oboe;

    oso->write_frame_count = oso->request_num_frames;
    *frame_count = oso->write_frame_count;

    for (int ch = 0; ch < outstream->layout.channel_count; ch += 1)
    {
        oso->areas[ch].ptr = reinterpret_cast<char*>(oso->request_audio_data) + outstream->bytes_per_sample * ch;
        oso->areas[ch].step = outstream->bytes_per_frame;
    }
    *out_areas = oso->areas;
    return 0;
}

static int outstream_end_write_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os)
{
    struct SoundIoOutStreamOboe* osca = &os->backend_data.oboe;
    osca->buffer_index += 1;
    return 0;
}

static int outstream_clear_buffer_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os)
{
    return SoundIoErrorIncompatibleBackend;
}

static int outstream_get_latency_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os,
                                      double* out_latency)
{
    struct SoundIoOutStreamOboe* oso = &os->backend_data.oboe;
    *out_latency = oso->hardware_latency;
    return 0;
}

static int outstream_set_volume_oboe(struct SoundIoPrivate* si, struct SoundIoOutStreamPrivate* os, float volume)
{
    struct SoundIoOutStream* outstream = &os->pub;
    outstream->volume = volume;
    return 0;
}


static int instream_open_oboe(struct SoundIoPrivate* si, struct SoundIoInStreamPrivate* is)
{
    return SoundIoErrorBackendUnavailable;
}

static int instream_pause_oboe(struct SoundIoPrivate* si, struct SoundIoInStreamPrivate* is, bool pause)
{
    return SoundIoErrorBackendUnavailable;
}

static int instream_start_oboe(struct SoundIoPrivate* si, struct SoundIoInStreamPrivate* is)
{
    return SoundIoErrorBackendUnavailable;
}

static int instream_begin_read_oboe(struct SoundIoPrivate* si, struct SoundIoInStreamPrivate* is,
                                    struct SoundIoChannelArea** out_areas, int* frame_count)
{
    return SoundIoErrorBackendUnavailable;
}

static int instream_end_read_oboe(struct SoundIoPrivate* si, struct SoundIoInStreamPrivate* is)
{
    return SoundIoErrorBackendUnavailable;
}

static int instream_get_latency_oboe(struct SoundIoPrivate* si, struct SoundIoInStreamPrivate* is,
                                     double* out_latency)
{
    return SoundIoErrorBackendUnavailable;
}


static void instream_destroy_oboe(struct SoundIoPrivate* si, struct SoundIoInStreamPrivate* is)
{
}


static void destroy_oboe(struct SoundIoPrivate* si)
{
    struct SoundIoOboe* sio = &si->backend_data.oboe;

    if (sio->thread)
    {
        SOUNDIO_ATOMIC_FLAG_CLEAR(sio->abort_flag);
        soundio_os_cond_signal(sio->scan_devices_cond, NULL);
        soundio_os_thread_destroy(sio->thread);
    }

    if (sio->cond)
        soundio_os_cond_destroy(sio->cond);

    if (sio->have_devices_cond)
        soundio_os_cond_destroy(sio->have_devices_cond);

    if (sio->scan_devices_cond)
        soundio_os_cond_destroy(sio->scan_devices_cond);

    if (sio->mutex)
        soundio_os_mutex_destroy(sio->mutex);

    soundio_destroy_devices_info(sio->ready_devices_info);
}

int soundio_oboe_init(struct SoundIoPrivate* si)
{
    struct SoundIoOboe* sica = &si->backend_data.oboe;

    int32_t err;
    SOUNDIO_ATOMIC_STORE(sica->have_devices_flag, false);
    SOUNDIO_ATOMIC_STORE(sica->device_scan_queued, true);
    SOUNDIO_ATOMIC_STORE(sica->service_restarted, false);
    SOUNDIO_ATOMIC_FLAG_TEST_AND_SET(sica->abort_flag);

    sica->mutex = soundio_os_mutex_create();
    if (!sica->mutex)
    {
        destroy_oboe(si);
        return SoundIoErrorNoMem;
    }

    sica->cond = soundio_os_cond_create();
    if (!sica->cond)
    {
        destroy_oboe(si);
        return SoundIoErrorNoMem;
    }

    sica->have_devices_cond = soundio_os_cond_create();
    if (!sica->have_devices_cond)
    {
        destroy_oboe(si);
        return SoundIoErrorNoMem;
    }

    sica->scan_devices_cond = soundio_os_cond_create();
    if (!sica->scan_devices_cond)
    {
        destroy_oboe(si);
        return SoundIoErrorNoMem;
    }

    if ((err = soundio_os_thread_create(device_thread_run, si, nullptr, &sica->thread)))
    {
        destroy_oboe(si);
        return err;
    }

    si->destroy = destroy_oboe;
    si->flush_events = flush_events_oboe;
    si->wait_events = wait_events_oboe;
    si->wakeup = wakeup_oboe;
    si->force_device_scan = force_device_scan_oboe;

    si->outstream_open = outstream_open_oboe;
    si->outstream_destroy = outstream_destroy_oboe;
    si->outstream_start = outstream_start_oboe;
    si->outstream_begin_write = outstream_begin_write_oboe;
    si->outstream_end_write = outstream_end_write_oboe;
    si->outstream_clear_buffer = outstream_clear_buffer_oboe;
    si->outstream_pause = outstream_pause_oboe;
    si->outstream_get_latency = outstream_get_latency_oboe;
    si->outstream_set_volume = outstream_set_volume_oboe;

    si->instream_open = instream_open_oboe;
    si->instream_destroy = instream_destroy_oboe;
    si->instream_start = instream_start_oboe;
    si->instream_begin_read = instream_begin_read_oboe;
    si->instream_end_read = instream_end_read_oboe;
    si->instream_pause = instream_pause_oboe;
    si->instream_get_latency = instream_get_latency_oboe;

    return 0;
}

