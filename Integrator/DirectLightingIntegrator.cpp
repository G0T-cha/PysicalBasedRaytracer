#include "Integrator\DirectLightingIntegrator.h"
#include "Core\Scene.h"
#include "Sampler\Sampler.h"
#include "Light\Light.h"

namespace PBR {
	// 预处理：为 UniformSampleAll 策略设置采样器
	void DirectLightingIntegrator::Preprocess(const Scene& scene,
		Sampler& sampler) {
		if (strategy == LightStrategy::UniformSampleAll) {
			// 根据采样器确定采样次数
			for (const auto& light : scene.lights)
				nLightSamples.push_back(sampler.RoundCount(light->nSamples));

			// 支持镜面反射
			for (int i = 0; i < maxDepth; ++i) {
				// 为第j个光源预先请求2D随机数组
				for (size_t j = 0; j < scene.lights.size(); ++j) {
					// 采样光源
					sampler.Request2DArray(nLightSamples[j]);
					// 采样 BSDF
					sampler.Request2DArray(nLightSamples[j]);
				}
			}
		}
	}

	Spectrum DirectLightingIntegrator::Li(const RayDifferential& ray,
		const Scene& scene, Sampler& sampler, int depth) const {
		// 这里和Whitted积分器相同
		Spectrum L(0.f);
		SurfaceInteraction isect;
		if (!scene.Intersect(ray, &isect)) {
			for (const auto& light : scene.lights) L += light->Le(ray);
			return L;
		}
		isect.ComputeScatteringFunctions(ray);
		if (!isect.bsdf)
			return Li(isect.SpawnRay(ray.d), scene, sampler, depth);
		Vector3f wo = isect.wo;
		L += isect.Le(wo);

		if (scene.lights.size() > 0) {
			// 检查策略，委托给具体函数，参数为交点、场景、采样器、样本
			if (strategy == LightStrategy::UniformSampleAll)
				L += UniformSampleAllLights(isect, scene, sampler,
					nLightSamples);
			else
				L += UniformSampleOneLight(isect, scene, sampler);
		}

		// 这里也和Whitted积分器相同
		if (depth + 1 < maxDepth) {
			// Trace rays for specular reflection and refraction
			L += SpecularReflect(ray, isect, scene, sampler, depth);
			//L += SpecularTransmit(ray, isect, scene, sampler, depth);
		}
		return L;
	}
}