/*
 * Copyright (C) 2020 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/Forward.h>

#if ENABLE(EXPERIMENTAL_FEATURES)
#define DEFAULT_EXPERIMENTAL_FEATURES_ENABLED true
#else
#define DEFAULT_EXPERIMENTAL_FEATURES_ENABLED false
#endif

#if PLATFORM(MAC)
#define DEFAULT_SUBPIXEL_ANTIALIASED_LAYER_TEXT_ENABLED true
#else
#define DEFAULT_SUBPIXEL_ANTIALIASED_LAYER_TEXT_ENABLED false
#endif

namespace WebKit {

#if PLATFORM(COCOA) && HAVE(SYSTEM_FEATURE_FLAGS)
bool isFeatureFlagEnabled(const String&);
#endif

#if ENABLE(WEBGPU)
bool defaultWebGPUEnabled();
#endif

#if HAVE(INCREMENTAL_PDF_APIS)
bool defaultIncrementalPDFEnabled();
#endif

#if ENABLE(WEBXR)
bool defaultWebXREnabled();
#endif

bool defaultVisualViewportAPIEnabled();
bool defaultModernUnprefixedWebAudioEnabled();

} // namespace WebKit
