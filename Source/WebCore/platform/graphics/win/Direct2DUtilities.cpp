/*
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION embedded systems
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

#include "config.h"
#include "Direct2DUtilities.h"

#if USE(DIRECT2D)

#include "COMPtr.h"
#include "FloatPoint.h"
#include "GraphicsContext.h"
#include "ImageDecoderDirect2D.h"
#include "IntRect.h"
#include "IntSize.h"
#include <d2d1.h>
#include <wincodec.h>


namespace WebCore {

namespace Direct2D {

IntSize bitmapSize(IWICBitmapSource* bitmapSource)
{
    UINT width, height;
    HRESULT hr = bitmapSource->GetSize(&width, &height);
    if (!SUCCEEDED(hr))
        return { };

    return IntSize(width, height);
}

FloatPoint bitmapResolution(IWICBitmapSource* bitmapSource)
{
    constexpr double dpiBase = 96.0;

    double dpiX, dpiY;
    HRESULT hr = bitmapSource->GetResolution(&dpiX, &dpiY);
    if (!SUCCEEDED(hr))
        return { };

    FloatPoint result(dpiX, dpiY);
    result.scale(1 / dpiBase);
    return result;
}

unsigned bitsPerPixel(GUID bitmapFormat)
{
    COMPtr<IWICComponentInfo> componentInfo;
    HRESULT hr = ImageDecoderDirect2D::systemImagingFactory()->CreateComponentInfo(bitmapFormat, &componentInfo);
    if (!SUCCEEDED(hr))
        return 4;

    COMPtr<IWICPixelFormatInfo> pixelFormat;
    pixelFormat.query(componentInfo);
    if (!pixelFormat)
        return 4;

    UINT bpp = 0;
    hr = pixelFormat->GetBitsPerPixel(&bpp);
    if (!SUCCEEDED(hr))
        return 4;

    return bpp;
}

COMPtr<IWICBitmap> createDirect2DImageSurfaceWithData(void* data, const IntSize& size, unsigned stride)
{
    Checked<size_t, RecordOverflow> numBytes = Checked<unsigned, RecordOverflow>(size.height()) * stride;
    if (numBytes.hasOverflowed())
        return nullptr;

    COMPtr<IWICBitmap> surface;
    HRESULT hr = ImageDecoderDirect2D::systemImagingFactory()->CreateBitmapFromMemory(size.width(), size.height(), GUID_WICPixelFormat32bppPBGRA, stride, static_cast<UINT>(numBytes.unsafeGet()), reinterpret_cast<BYTE*>(data), &surface);
    if (!SUCCEEDED(hr))
        return nullptr;

    return surface;
}

COMPtr<ID2D1RenderTarget> createRenderTargetFromWICBitmap(IWICBitmap* bitmapSource)
{
    auto targetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

    COMPtr<ID2D1RenderTarget> bitmapContext;
    HRESULT hr = GraphicsContext::systemFactory()->CreateWicBitmapRenderTarget(bitmapSource, &targetProperties, &bitmapContext);
    if (!bitmapContext || !SUCCEEDED(hr))
        return nullptr;

    return bitmapContext;
}

void copyRectFromOneSurfaceToAnother(IWICBitmap* from, IWICBitmap* to, const IntSize& sourceOffset, const IntRect& rect, const IntSize& destOffset)
{
    /*
    RefPtr<cairo_t> context = adoptRef(cairo_create(to));
    cairo_translate(context.get(), destOffset.width(), destOffset.height());
    cairo_set_operator(context.get(), cairoOperator);
    copyRectFromCairoSurfaceToContext(from, context.get(), sourceOffset, rect);
    */
}

} // namespace Direct2D

} // namespace WebCore

#endif // USE(DIRECT2D)
