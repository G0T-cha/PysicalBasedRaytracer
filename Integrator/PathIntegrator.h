#pragma once
#ifndef __PathIntegrator_h__
#define __PathIntegrator_h__

#include "Integrator\Integrator.h"
#include "Core\PBR.h"

namespace PBR {

    class PathIntegrator : public SamplerIntegrator {
    public:
        // maxDepth 光线在被强制终止前允许的最大反弹次数
        // rrThreshold 俄罗斯轮盘赌
        PathIntegrator(int maxDepth, std::shared_ptr<const Camera> camera,
            std::shared_ptr<Sampler> sampler,
            const Bounds2i& pixelBounds, float rrThreshold = 1,
            const std::string& lightSampleStrategy = "spatial",
            FrameBuffer* framebuffer = nullptr);
        // 创建加权轮盘
        void Preprocess(const Scene& scene, Sampler& sampler);
        // 计算一条光线最终带回的能量
        Spectrum Li(const RayDifferential& ray, const Scene& scene,
            Sampler& sampler, int depth) const;

    private:
        // PathIntegrator Private Data
        const int maxDepth;
        const float rrThreshold;
        const std::string lightSampleStrategy;
        std::unique_ptr<LightDistribution> lightDistribution;
    };


}


#endif