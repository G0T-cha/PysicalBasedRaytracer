#pragma once
#ifndef __WhittedIntegrator_h__
#define __WhittedIntegrator_h__
#include "Integrator\Integrator.h"

namespace PBR {
class WhittedIntegrator : public SamplerIntegrator {
  public:
    // Whitted 积分器（额外保存一个递归深度
    WhittedIntegrator(int maxDepth, std::shared_ptr<const Camera> camera,
                      std::shared_ptr<Sampler> sampler,
                      const Bounds2i &pixelBounds, FrameBuffer * m_FrameBuffer)
        : SamplerIntegrator(camera, sampler, pixelBounds, m_FrameBuffer), maxDepth(maxDepth) {}
    // 重写光线计算函数
    Spectrum Li(const Ray &ray, const Scene &scene,
                Sampler &sampler, int depth) const;
  private:
    const int maxDepth;
};













}









#endif






