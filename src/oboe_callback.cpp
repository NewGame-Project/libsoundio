//
// Created by Administrator on 2026/3/9.
//

#include "oboe_callback.h"

#include "soundio_private.h"
#include "oboe/AudioStreamBuilder.h"


oboe::DataCallbackResult oboe_callback::onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numFrames)
{
    struct SoundIoOutStreamPrivate* os = outstream_private;
    struct SoundIoOutStream* outstream = &os->pub;
    struct SoundIoOutStreamOboe* osca = &os->backend_data.oboe;

    osca->request_num_frames = numFrames;
    osca->request_audio_data = static_cast<float*>(audioData);
    outstream->write_callback(outstream, osca->request_num_frames, osca->request_num_frames);

    return oboe::DataCallbackResult::Continue;
}
