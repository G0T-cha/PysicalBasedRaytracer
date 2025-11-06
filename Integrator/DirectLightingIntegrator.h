#pragma once
#ifndef __DirectLightingIntegrator_h__
#define __DirectLightingIntegrator_h__

#include "Core\PBR.h"
#include "Integrator\Integrator.h"

namespace PBR {

    // 两种采样策略：均匀采样所有光源（遍历场景中的每一个光源），
    //             均匀采样一个光源（从场景中所有光源里随机挑选一个进行采样）
    enum class LightStrategy { UniformSampleAll, UniformSampleOne };

    // 直接光采样
    class DirectLightingIntegrator : public SamplerIntegrator {
    public:
        // 采样策略
        DirectLightingIntegrator(LightStrategy strategy, int maxDepth,
            std::shared_ptr<const Camera> camera,
            std::shared_ptr<Sampler> sampler,
            const Bounds2i& pixelBounds,
            FrameBuffer* framebuffer)
            : SamplerIntegrator(camera, sampler, pixelBounds, framebuffer),
            strategy(strategy),
            maxDepth(maxDepth) {}
        // 计算入射辐射，即沿着 ray ，进入相机的光（Spectrum）
        Spectrum Li(const RayDifferential& ray, const Scene& scene,
            Sampler& sampler, int depth) const;
        // 预处理
        void Preprocess(const Scene& scene, Sampler& sampler);

    private:
        const LightStrategy strategy;
        const int maxDepth;
        // nLightSamples[i] 代表存储了场景中第 i 个光源需要被采样的次数
        std::vector<int> nLightSamples;
    };




}

#endif