#pragma once
#ifndef INTEGRATOR_H
#define INTEGRATOR_H
#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\FrameBuffer.h"

namespace PBR{
	// ����������
	class Integrator {
	public:
		virtual ~Integrator() {}
		// ��Ҫʵ����Ⱦ����������Ⱦ���̵����
		virtual void Render(const Scene& scene, double& timeConsume) = 0;
		float IntegratorRenderTime; //��Ⱦһ���õ�ʱ��
	};

	// �������Ļ�����������������ؿ���Ļ�����
	class SamplerIntegrator : public Integrator {
	public:
		// ��������������������򡢻���
		SamplerIntegrator(std::shared_ptr<const Camera> camera,
			std::shared_ptr<Sampler> sampler,
			const Bounds2i& pixelBounds, FrameBuffer* m_FrameBuffer)
			: camera(camera), sampler(sampler), pixelBounds(pixelBounds), m_FrameBuffer(m_FrameBuffer) {}
		virtual void Preprocess(const Scene& scene, Sampler& sampler) {}
		// ��Ⱦ
		void Render(const Scene& scene, double& timeConsume);
		// ���ռ���
		virtual Spectrum Li(const Ray& ray, const Scene& scene, Sampler& sampler, int depth = 0) const = 0;
		// �������뾵�淴��
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