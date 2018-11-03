/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#if ENABLE(CSS_PAINTING_API)

#include "GeneratedImage.h"
#include "PaintWorkletGlobalScope.h"

namespace WebCore {

class ImageBuffer;

class CustomPaintImage final : public GeneratedImage {
public:
    static Ref<CustomPaintImage> create(const PaintWorkletGlobalScope::PaintDefinition& definition, const FloatSize& size)
    {
        return adoptRef(*new CustomPaintImage(definition, size));
    }

    virtual ~CustomPaintImage();
    bool isCustomPaintImage() const override { return true; }

private:
    CustomPaintImage(const PaintWorkletGlobalScope::PaintDefinition&, const FloatSize&);

    ImageDrawResult doCustomPaint(GraphicsContext&, const FloatSize&);

    ImageDrawResult draw(GraphicsContext&, const FloatRect& dstRect, const FloatRect& srcRect, CompositeOperator, BlendMode, DecodingMode, ImageOrientationDescription) final;
    void drawPattern(GraphicsContext&, const FloatRect& destRect, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, const FloatSize& spacing, CompositeOperator, BlendMode) final;

    Ref<CSSPaintCallback> m_paintCallback;
};

}

SPECIALIZE_TYPE_TRAITS_IMAGE(CustomPaintImage)
#endif
