#pragma once
#ifndef INTEGRATOR_H
#define INTEGRATOR_H
#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\FrameBuffer.h"

namespace PBR{
	// 积分器基类
	class Integrator {
	public:
		virtual ~Integrator() {}
		// 需要实现渲染函数，是渲染过程的入口
		virtual void Render(const Scene& scene, double& timeConsume) = 0;
		float IntegratorRenderTime; //渲染一次用的时间
	};

	// 带采样的积分器，如大多基于蒙特卡洛的积分器
	class SamplerIntegrator : public Integrator {
	public:
		// 相机、采样器、像素区域、画布
		SamplerIntegrator(std::shared_ptr<const Camera> camera,
			std::shared_ptr<Sampler> sampler,
			const Bounds2i& pixelBounds, FrameBuffer* m_FrameBuffer)
			: camera(camera), sampler(sampler), pixelBounds(pixelBounds), m_FrameBuffer(m_FrameBuffer) {}
		virtual void Preprocess(const Scene& scene, Sampler& sampler) {}
		// 渲染
		void Render(const Scene& scene, double& timeConsume);
		// 光照计算
		virtual Spectrum Li(const Ray& ray, const Scene& scene, Sampler& sampler, int depth = 0) const = 0;
		// 处理理想镜面反射
		Spectrum SpecularReflect(const Ray& ray,
			const SurfaceInteraction& isect,
			const Scene& scene, Sampler& sampler,
			int depth) const;
	protected:
		std::shared_ptr<const Camera> camera;

	private:
		std::shared_ptr<Sampler> sampler;
		const Bounds2i pixelBounds;
		FrameBuffer* m_FrameBuffer;
	};
}



#endif