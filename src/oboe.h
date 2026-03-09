//
// Created by Administrator on 2026/3/9.
//

#ifndef AUDIORENDERER_OBOE_H
#define AUDIORENDERER_OBOE_H

#include "soundio_internal.h"
#include "os.h"
#include "atomics.h"
#include "oboe_callback.h"

struct SoundIoPrivate;

#ifdef __cplusplus
extern "C"
{
#endif

int soundio_oboe_init(struct SoundIoPrivate* si);

struct SoundIoOboe
{
    struct SoundIoOsMutex* mutex;
    struct SoundIoOsCond* cond;
    struct SoundIoOsThread* thread;
    struct SoundIoAtomicFlag abort_flag;

    // this one is ready to be read with flush_events. protected by mutex
    struct SoundIoDevicesInfo* ready_devices_info;
    struct SoundIoAtomicBool have_devices_flag;
    struct SoundIoOsCond* have_devices_cond;
    struct SoundIoOsCond* scan_devices_cond;

    struct SoundIoAtomicBool device_scan_queued;
    struct SoundIoAtomicBool service_restarted;
    int shutdown_err;
    bool emitted_shutdown_cb;
};

struct SoundIoOutStreamOboe
{
    int buffer_index;
    int write_frame_count;
    double hardware_latency;
    float volume;
    float* request_audio_data;
    int request_num_frames;

    oboe::AudioStream* output_stream; /* oboe::AudioStream* in C++ */
    oboe_callback* callback; /* oboe_callback* in C++ */

    struct SoundIoChannelArea areas[SOUNDIO_MAX_CHANNELS];
};

struct SoundIoInStreamOboe
{
    int frames_left;
    double hardware_latency;
    struct SoundIoChannelArea areas[SOUNDIO_MAX_CHANNELS];
};

#ifdef __cplusplus
}
#endif

#endif //AUDIORENDERER_OBOE_H
