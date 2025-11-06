#pragma once
#ifndef __VolPathIntegrator_h__
#define __VolPathIntegrator_h__

#include "Integrator\Integrator.h"
#include "Core\PBR.h"
#include "Light\LightDistrib.h"

namespace PBR {
class VolPathIntegrator : public SamplerIntegrator {
public:
	// VolPathIntegrator Public Methods
	VolPathIntegrator(int maxDepth, std::shared_ptr<const Camera> camera,
		std::shared_ptr<Sampler> sampler,
		const Bounds2i &pixelBounds, float rrThreshold = 1,
		const std::string &lightSampleStrategy = "spatial",
		FrameBuffer * m_FrameBuffer = nullptr)
		: SamplerIntegrator(camera, sampler, pixelBounds, m_FrameBuffer),
		maxDepth(maxDepth),
		rrThreshold(rrThreshold),
		lightSampleStrategy(lightSampleStrategy) { }
	Spectrum Li(const RayDifferential &ray, const Scene &scene,
		Sampler &sampler, int depth) const;
	void Preprocess(const Scene &scene, Sampler &sampler);

private:
	const int maxDepth;
	const float rrThreshold;
	const std::string lightSampleStrategy;
	std::unique_ptr<LightDistribution> lightDistribution;
};


}











#endif






