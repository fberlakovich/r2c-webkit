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

#import "config.h"
#import "GPURenderPipeline.h"

#if ENABLE(WEBGPU)

#import "GPURenderPipelineDescriptor.h"
#import "Logging.h"
#import "WebGPUShaderStage.h"

#import <Metal/Metal.h>
#import <wtf/BlockObjCExceptions.h>

namespace WebCore {

static bool setFunctionsForPipelineDescriptor(const char* const functionName, MTLRenderPipelineDescriptor *mtlDescriptor, GPURenderPipelineDescriptor&& descriptor)
{
#if LOG_DISABLED
    UNUSED_PARAM(functionName);
#endif
    for (const auto& stageDescriptor : descriptor.stages) {
        auto mtlLibrary = retainPtr(stageDescriptor.module.platformShaderModule());

        if (!mtlLibrary) {
            LOG(WebGPU, "%s: MTLLibrary does not exist!", functionName);
            return false;
        }

        auto function = adoptNS([mtlLibrary newFunctionWithName:stageDescriptor.entryPoint]);

        if (!function) {
            LOG(WebGPU, "%s: MTLFunction %s not found!", functionName, stageDescriptor.entryPoint.utf8().data());
            return false;
        }

        switch (stageDescriptor.stage) {
        case WebGPUShaderStage::VERTEX:
            [mtlDescriptor setVertexFunction:function.get()];
            break;
        case WebGPUShaderStage::FRAGMENT:
            [mtlDescriptor setFragmentFunction:function.get()];
            break;
        default:
            LOG(WebGPU, "%s: Invalid shader stage specified!", functionName);
            return false;
            break;
        }
    }

    return true;
}

RefPtr<GPURenderPipeline> GPURenderPipeline::create(const GPUDevice& device, GPURenderPipelineDescriptor&& descriptor)
{
    const char* const functionName = "GPURenderPipeline::create()";

    if (!device.platformDevice()) {
        LOG(WebGPU, "%s: MTLDevice does not exist!", functionName);
        return nullptr;
    }

    RetainPtr<MTLRenderPipelineDescriptor> mtlDescriptor;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    mtlDescriptor = adoptNS([MTLRenderPipelineDescriptor new]);

    END_BLOCK_OBJC_EXCEPTIONS;

    if (!mtlDescriptor) {
        LOG(WebGPU, "%s: Error creating MTLDescriptor!", functionName);
        return nullptr;
    }

    if (!setFunctionsForPipelineDescriptor(functionName, mtlDescriptor.get(), WTFMove(descriptor)))
        return nullptr;

    // FIXME: Get the pixelFormat as configured for the context/CAMetalLayer.
    mtlDescriptor.get().colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

    PlatformRenderPipelineSmartPtr pipeline;

    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    if ([mtlDescriptor vertexFunction])
        pipeline = adoptNS([device.platformDevice() newRenderPipelineStateWithDescriptor:mtlDescriptor.get() error:nil]);
    else
        LOG(WebGPU, "%s: No vertex function assigned for MTLRenderPipelineDescriptor!", functionName);

    END_BLOCK_OBJC_EXCEPTIONS;

    if (!pipeline) {
        LOG(WebGPU, "%s: Error creating MTLRenderPipelineState!", functionName);
        return nullptr;
    }

    return adoptRef(new GPURenderPipeline(WTFMove(pipeline)));
}

GPURenderPipeline::GPURenderPipeline(PlatformRenderPipelineSmartPtr&& pipeline)
    : m_platformRenderPipeline(WTFMove(pipeline))
{
    UNUSED_PARAM(m_platformRenderPipeline);
}

} // namespace WebCore

#endif // ENABLE(WEBGPU)
