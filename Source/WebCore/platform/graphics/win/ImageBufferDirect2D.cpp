/*
 * Copyright (C) 2016-2018 Apple Inc. All rights reserved.
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
#include "ImageBuffer.h"

#if USE(DIRECT2D)

#include "BitmapImage.h"
#include "COMPtr.h"
#include "GraphicsContext.h"
#include "ImageData.h"
#include "ImageDecoderDirect2D.h"
#include "IntRect.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include <d2d1.h>
#include <math.h>
#include <wincodec.h>
#include <wtf/Assertions.h>
#include <wtf/CheckedArithmetic.h>
#include <wtf/MainThread.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/Base64.h>
#include <wtf/text/WTFString.h>


namespace WebCore {

static FloatSize scaleSizeToUserSpace(const FloatSize& logicalSize, const IntSize& backingStoreSize, const IntSize& internalSize)
{
    float xMagnification = static_cast<float>(backingStoreSize.width()) / internalSize.width();
    float yMagnification = static_cast<float>(backingStoreSize.height()) / internalSize.height();
    return FloatSize(logicalSize.width() * xMagnification, logicalSize.height() * yMagnification);
}

std::unique_ptr<ImageBuffer> ImageBuffer::createCompatibleBuffer(const FloatSize& size, const GraphicsContext& context)
{
    if (size.isEmpty())
        return nullptr;

    RenderingMode renderingMode = context.renderingMode();
    IntSize scaledSize = ImageBuffer::compatibleBufferSize(size, context);
    bool success = false;
    std::unique_ptr<ImageBuffer> buffer(new ImageBuffer(scaledSize, 1, ColorSpaceSRGB, renderingMode, nullptr, &context, success));

    if (!success)
        return nullptr;

    // Set up a corresponding scale factor on the graphics context.
    buffer->context().scale(FloatSize(scaledSize.width() / size.width(), scaledSize.height() / size.height()));
    return buffer;
}

ImageBuffer::ImageBuffer(const FloatSize& size, float resolutionScale, ColorSpace /*colorSpace*/, RenderingMode renderingMode, const HostWindow*, const GraphicsContext*, bool& success)
    : m_logicalSize(size)
    , m_resolutionScale(resolutionScale)
{
    success = false; // Make early return mean failure.
    float scaledWidth = std::ceil(resolutionScale * size.width());
    float scaledHeight = std::ceil(resolutionScale * size.height());

    // FIXME: Should we automatically use a lower resolution?
    if (!FloatSize(scaledWidth, scaledHeight).isExpressibleAsIntSize())
        return;

    m_size = IntSize(scaledWidth, scaledHeight);
    m_data.backingStoreSize = m_size;

    bool accelerateRendering = renderingMode == Accelerated;
    if (m_size.width() <= 0 || m_size.height() <= 0)
        return;

    // Prevent integer overflows
    m_data.bytesPerRow = 4 * Checked<unsigned, RecordOverflow>(m_data.backingStoreSize.width());
    Checked<size_t, RecordOverflow> numBytes = Checked<unsigned, RecordOverflow>(m_data.backingStoreSize.height()) * m_data.bytesPerRow;
    if (numBytes.hasOverflowed())
        return;

    m_data.data = Vector<char>(numBytes.unsafeGet(), 0);

    HRESULT hr = ImageDecoderDirect2D::systemImagingFactory()->CreateBitmapFromMemory(m_size.width(), m_size.height(), GUID_WICPixelFormat32bppPBGRA, static_cast<UINT>(m_data.bytesPerRow.unsafeGet()), static_cast<UINT>(numBytes.unsafeGet()), reinterpret_cast<BYTE*>(m_data.data.data()), &m_data.bitmapSource);
    if (!SUCCEEDED(hr))
        return;

    auto targetProperties = D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0, D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

    COMPtr<ID2D1RenderTarget> bitmapContext;
    hr = GraphicsContext::systemFactory()->CreateWicBitmapRenderTarget(m_data.bitmapSource.get(), &targetProperties, &bitmapContext);
    if (!bitmapContext || !SUCCEEDED(hr))
        return;

    // Note: This places the bitmapcontext into a locked state because of the BeginDraw call in the constructor.
    m_data.context = std::make_unique<GraphicsContext>(bitmapContext.get());

    success = true;
}

ImageBuffer::ImageBuffer(const FloatSize& size, float resolutionScale, ColorSpace imageColorSpace, RenderingMode renderingMode, const HostWindow*, bool& success)
    : ImageBuffer(size, resolutionScale, imageColorSpace, renderingMode, nullptr, nullptr, success)
{
}

ImageBuffer::~ImageBuffer() = default;

FloatSize ImageBuffer::sizeForDestinationSize(FloatSize destinationSize) const
{
    return scaleSizeToUserSpace(destinationSize, m_data.backingStoreSize, internalSize());
}

GraphicsContext& ImageBuffer::context() const
{
    return *m_data.context;
}

void ImageBuffer::flushContext() const
{
    context().flush();
}

static COMPtr<IWICBitmap> createCroppedImageIfNecessary(IWICBitmap* image, const IntSize& bounds)
{
    FloatSize imageSize = image ? nativeImageSize(image) : FloatSize();

    if (image && (static_cast<size_t>(imageSize.width()) != static_cast<size_t>(bounds.width()) || static_cast<size_t>(imageSize.height()) != static_cast<size_t>(bounds.height()))) {
        D2D_POINT_2U origin = { };
        WICRect croppedDimensions = { 0, 0, bounds.width(), bounds.height() };

        COMPtr<IWICBitmapClipper> bitmapClipper;
        HRESULT hr = ImageDecoderDirect2D::systemImagingFactory()->CreateBitmapClipper(&bitmapClipper);
        if (SUCCEEDED(hr)) {
            hr = bitmapClipper->Initialize(image, &croppedDimensions);
            if (SUCCEEDED(hr)) {
                COMPtr<IWICBitmap> croppedBitmap;
                hr = ImageDecoderDirect2D::systemImagingFactory()->CreateBitmapFromSource(image, WICBitmapNoCache, &croppedBitmap);
                if (SUCCEEDED(hr))
                    return croppedBitmap;
            }
        }
    }

    return image;
}

static RefPtr<Image> createBitmapImageAfterScalingIfNeeded(COMPtr<IWICBitmap>&& image, IntSize internalSize, IntSize logicalSize, IntSize backingStoreSize, float resolutionScale, PreserveResolution preserveResolution)
{
    if (resolutionScale == 1 || preserveResolution == PreserveResolution::Yes)
        image = createCroppedImageIfNecessary(image.get(), internalSize);
    else {
        // FIXME: Need to implement scaled version
        notImplemented();
    }

    if (!image)
        return nullptr;

    return BitmapImage::create(WTFMove(image));
}

RefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, PreserveResolution preserveResolution) const
{
    COMPtr<IWICBitmap> image;
    if (m_resolutionScale == 1 || preserveResolution == PreserveResolution::Yes)
        image = copyNativeImage(copyBehavior);
    else
        image = copyNativeImage(DontCopyBackingStore);

    return createBitmapImageAfterScalingIfNeeded(WTFMove(image), internalSize(), logicalSize(), m_data.backingStoreSize, m_resolutionScale, preserveResolution);
}

RefPtr<Image> ImageBuffer::sinkIntoImage(std::unique_ptr<ImageBuffer> imageBuffer, PreserveResolution preserveResolution)
{
    IntSize internalSize = imageBuffer->internalSize();
    IntSize logicalSize = imageBuffer->logicalSize();
    IntSize backingStoreSize = imageBuffer->m_data.backingStoreSize;
    float resolutionScale = imageBuffer->m_resolutionScale;

    return createBitmapImageAfterScalingIfNeeded(sinkIntoNativeImage(WTFMove(imageBuffer)), internalSize, logicalSize, backingStoreSize, resolutionScale, preserveResolution);
}

BackingStoreCopy ImageBuffer::fastCopyImageMode()
{
    return DontCopyBackingStore;
}

COMPtr<IWICBitmap> ImageBuffer::sinkIntoNativeImage(std::unique_ptr<ImageBuffer> imageBuffer)
{
    // FIXME: See if we can reuse the on-hardware image.
    return imageBuffer->copyNativeImage(DontCopyBackingStore);
}

COMPtr<IWICBitmap> ImageBuffer::copyNativeImage(BackingStoreCopy copyBehavior) const
{
    // FIXME: m_data.data is nullptr even when asking to copy backing store leading to test failures.
    if (copyBehavior == CopyBackingStore && m_data.data.isEmpty())
        copyBehavior = DontCopyBackingStore;

    Checked<size_t, RecordOverflow> numBytes = Checked<unsigned, RecordOverflow>(m_data.backingStoreSize.height()) * m_data.bytesPerRow;
    if (numBytes.hasOverflowed())
        return nullptr;

    HRESULT hr = S_OK;
    COMPtr<IWICBitmap> image;
    if (!context().isAcceleratedContext()) {
        switch (copyBehavior) {
        case DontCopyBackingStore:
            hr = ImageDecoderDirect2D::systemImagingFactory()->CreateBitmapFromSource(m_data.bitmapSource.get(), WICBitmapNoCache, &image);
            break;
        case CopyBackingStore:
            hr = ImageDecoderDirect2D::systemImagingFactory()->CreateBitmapFromSource(m_data.bitmapSource.get(), WICBitmapCacheOnDemand, &image);
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }
#if USE(IOSURFACE_CANVAS_BACKING_STORE)
    else
        image = m_data.surface->createImage();
#endif

    return image;
}

void ImageBuffer::drawConsuming(std::unique_ptr<ImageBuffer> imageBuffer, GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode)
{
    imageBuffer->draw(destContext, destRect, srcRect, op, blendMode);
}

void ImageBuffer::draw(GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect, CompositeOperator op, BlendMode blendMode)
{
    FloatRect adjustedSrcRect = srcRect;
    adjustedSrcRect.scale(m_resolutionScale, m_resolutionScale);

    FloatSize currentImageSize = nativeImageSize(m_data.bitmapSource);

    // You can't convert a IWICBitmap to a ID2D1Bitmap with an active GraphicsContext attached to it.
    m_data.context->endDraw();

    destContext.drawNativeImage(m_data.bitmapSource, currentImageSize, destRect, adjustedSrcRect, op, blendMode);

    m_data.context->beginDraw();

    destContext.flush();
}

void ImageBuffer::drawPattern(GraphicsContext& destContext, const FloatRect& destRect, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, const FloatSize& spacing, CompositeOperator op, BlendMode blendMode)
{
    FloatRect adjustedSrcRect = srcRect;
    adjustedSrcRect.scale(m_resolutionScale, m_resolutionScale);

    if (!context().isAcceleratedContext()) {
        if (&destContext == &context() || destContext.isAcceleratedContext()) {
            if (RefPtr<Image> copy = copyImage(CopyBackingStore)) // Drawing into our own buffer, need to deep copy.
                copy->drawPattern(destContext, destRect, adjustedSrcRect, patternTransform, phase, spacing, op, blendMode);
        } else {
            if (RefPtr<Image> imageForRendering = copyImage(DontCopyBackingStore))
                imageForRendering->drawPattern(destContext, destRect, adjustedSrcRect, patternTransform, phase, spacing, op, blendMode);
        }
    } else {
        if (RefPtr<Image> copy = copyImage(CopyBackingStore))
            copy->drawPattern(destContext, destRect, adjustedSrcRect, patternTransform, phase, spacing, op, blendMode);
    }
}

RefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, IntSize* pixelArrayDimensions, CoordinateSystem coordinateSystem) const
{
    if (context().isAcceleratedContext())
        flushContext();

    IntRect srcRect = rect;
    if (coordinateSystem == LogicalCoordinateSystem)
        srcRect.scale(m_resolutionScale);

    if (pixelArrayDimensions)
        *pixelArrayDimensions = srcRect.size();

    return m_data.getData(AlphaPremultiplication::Unpremultiplied, srcRect, internalSize(), context().isAcceleratedContext(), 1);
}

RefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, IntSize* pixelArrayDimensions, CoordinateSystem coordinateSystem) const
{
    if (context().isAcceleratedContext())
        flushContext();

    IntRect srcRect = rect;
    if (coordinateSystem == LogicalCoordinateSystem)
        srcRect.scale(m_resolutionScale);

    if (pixelArrayDimensions)
        *pixelArrayDimensions = srcRect.size();

    return m_data.getData(AlphaPremultiplication::Premultiplied, srcRect, internalSize(), context().isAcceleratedContext(), 1);
}

void ImageBuffer::putByteArray(const Uint8ClampedArray& source, AlphaPremultiplication bufferFormat, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem coordinateSystem)
{
    if (context().isAcceleratedContext())
        flushContext();

    IntRect scaledSourceRect = sourceRect;
    IntSize scaledSourceSize = sourceSize;
    if (coordinateSystem == LogicalCoordinateSystem) {
        scaledSourceRect.scale(m_resolutionScale);
        scaledSourceSize.scale(m_resolutionScale);
    }

    m_data.putData(source, bufferFormat, scaledSourceSize, scaledSourceRect, destPoint, internalSize(), context().isAcceleratedContext(), 1);
}

String ImageBuffer::toDataURL(const String&, Optional<double>, PreserveResolution) const
{
    notImplemented();
    return "data:,"_s;
}

Vector<uint8_t> ImageBuffer::toData(const String& mimeType, Optional<double> quality) const
{
    notImplemented();
    return { };
}

String ImageDataToDataURL(const ImageData& source, const String& mimeType, const double* quality)
{
    notImplemented();
    return "data:,"_s;
}

void ImageBuffer::transformColorSpace(ColorSpace, ColorSpace)
{
    notImplemented();
}

} // namespace WebCore

#endif
