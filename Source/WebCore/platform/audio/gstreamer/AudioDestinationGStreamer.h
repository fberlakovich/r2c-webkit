/*
 *  Copyright (C) 2011, 2012 Igalia S.L
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef AudioDestinationGStreamer_h
#define AudioDestinationGStreamer_h

#include "AudioBus.h"
#include "AudioDestination.h"
#include "GRefPtrGStreamer.h"
#include <wtf/Forward.h>

typedef struct _GstElement GstElement;
typedef struct _GstPad GstPad;
typedef struct _GstMessage GstMessage;

namespace WebCore {

class AudioDestinationGStreamer : public AudioDestination {
public:
    AudioDestinationGStreamer(AudioIOCallback&, float sampleRate);
    virtual ~AudioDestinationGStreamer();

    void start(Function<void(Function<void()>&&)>&& dispatchToRenderThread) override;
    void stop() override;

    bool isPlaying() override { return m_isPlaying; }
    float sampleRate() const override { return m_sampleRate; }
    unsigned framesPerBuffer() const final;
    AudioIOCallback& callback() const { return m_callback; }

    gboolean handleMessage(GstMessage*);

private:
    AudioIOCallback& m_callback;
    RefPtr<AudioBus> m_renderBus;

    float m_sampleRate;
    bool m_isPlaying;
    bool m_audioSinkAvailable;
    GstElement* m_pipeline;
    GRefPtr<GstElement> m_src;
};

} // namespace WebCore

#endif // AudioDestinationGStreamer_h
