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

#include "config.h"
#include "WebGPUDevice.h"

#if ENABLE(WEBGPU)

#include "GPUPipelineStageDescriptor.h"
#include "GPURenderPipelineDescriptor.h"
#include "GPUShaderModuleDescriptor.h"
#include "Logging.h"
#include "WebGPUPipelineStageDescriptor.h"
#include "WebGPURenderPipeline.h"
#include "WebGPURenderPipelineDescriptor.h"
#include "WebGPUShaderModule.h"
#include "WebGPUShaderModuleDescriptor.h"
#include "WebGPUShaderStage.h"

namespace WebCore {

RefPtr<WebGPUDevice> WebGPUDevice::create(Ref<WebGPUAdapter>&& adapter)
{
    auto device = GPUDevice::create(); // FIXME: Take adapter into account when creating m_device.
    if (!device)
        return nullptr;

    return adoptRef(new WebGPUDevice(WTFMove(adapter), WTFMove(device)));
}

WebGPUDevice::WebGPUDevice(Ref<WebGPUAdapter>&& adapter, RefPtr<GPUDevice>&& device)
    : m_adapter(WTFMove(adapter))
    , m_device(device)
{
    UNUSED_PARAM(m_adapter);
}

RefPtr<WebGPUShaderModule> WebGPUDevice::createShaderModule(WebGPUShaderModuleDescriptor&& descriptor) const
{
    return WebGPUShaderModule::create(m_device->createShaderModule(GPUShaderModuleDescriptor { descriptor.code }));
}

RefPtr<WebGPURenderPipeline> WebGPUDevice::createRenderPipeline(WebGPURenderPipelineDescriptor&& descriptor) const
{
    const char* const functionName = "WebGPUDevice::createRenderPipeline()";
#if LOG_DISABLED
    UNUSED_PARAM(functionName);
#endif

    if (descriptor.stages.isEmpty()) {
        LOG(WebGPU, "%s: No stages in WebGPURenderPipelineDescriptor!", functionName);
        return nullptr;
    }

    GPUPipelineStageDescriptor vertexStage;
    GPUPipelineStageDescriptor fragmentStage;

    for (const auto& stageDescriptor : descriptor.stages) {
        if (!stageDescriptor.module || !stageDescriptor.module->module() || stageDescriptor.entryPoint.isEmpty()) {
            LOG(WebGPU, "%s: Invalid WebGPUPipelineStageDescriptor!", functionName);
            return nullptr;
        }

        switch (stageDescriptor.stage) {
        case WebGPUShaderStage::VERTEX:
            if (vertexStage.module) {
                LOG(WebGPU, "%s: Multiple vertex stages in WebGPURenderPipelineDescriptor!", functionName);
                return nullptr;
            }

            vertexStage.module = stageDescriptor.module->module();
            vertexStage.entryPoint = stageDescriptor.entryPoint;
            break;
        case WebGPUShaderStage::FRAGMENT:
            if (fragmentStage.module) {
                LOG(WebGPU, "%s: Multiple fragment stages in WebGPURenderPipelineDescriptor!", functionName);
                return nullptr;
            }

            fragmentStage.module = stageDescriptor.module->module();
            fragmentStage.entryPoint = stageDescriptor.entryPoint;
            break;
        default:
            LOG(WebGPU, "%s: Invalid shader stage in WebGPURenderPipelineDescriptor!", functionName);
            return nullptr;
        }
    }

    // Metal (if not other APIs) requires at least the vertex shader.
    if (!vertexStage.module || vertexStage.entryPoint.isEmpty()) {
        LOG(WebGPU, "%s: Invalid vertex stage in WebGPURenderPipelineDescriptor!", functionName);
        return nullptr;
    }

    return WebGPURenderPipeline::create(m_device->createRenderPipeline(GPURenderPipelineDescriptor { WTFMove(vertexStage), WTFMove(fragmentStage), static_cast<GPURenderPipelineDescriptor::PrimitiveTopology>(descriptor.primitiveTopology) }));
}

} // namespace WebCore

#endif // ENABLE(WEBGPU)
