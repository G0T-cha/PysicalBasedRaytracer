#include "Integrator\PathIntegrator.h"
#include "Light\LightDistrib.h"
#include "Core\Spectrum.h"
#include "Core\interaction.h"
#include "Core\Scene.h"
#include "Material\Reflection.h"
#include "Sampler\Sampler.h"
#include "Light\Light.h"

namespace PBR {

	static long long totalPaths = 0;
	static long long zeroRadiancePaths = 0;
	static long long pathLength = 0;

	PathIntegrator::PathIntegrator(int maxDepth,
		std::shared_ptr<const Camera> camera,
		std::shared_ptr<Sampler> sampler,
		const Bounds2i& pixelBounds, float rrThreshold,
		const std::string& lightSampleStrategy, FrameBuffer* framebuffer)
		: SamplerIntegrator(camera, sampler, pixelBounds, framebuffer),
		maxDepth(maxDepth),
		rrThreshold(rrThreshold),
		lightSampleStrategy(lightSampleStrategy) {}

	// 按照策略（如均匀采样、能量采样）创建加权轮盘
	void PathIntegrator::Preprocess(const Scene& scene, Sampler& sampler) {
		lightDistribution =
			CreateLightSampleDistribution(lightSampleStrategy, scene);
	}

	Spectrum PathIntegrator::Li(const RayDifferential& r, const Scene& scene,
		Sampler& sampler, int depth) const {
		//最终颜色L，光路当前能量权重beta（累计材质衰减）
		Spectrum L(0.f), beta(1.f);
		Ray ray(r);
		bool specularBounce = false;
		int bounces;
		// 辐射缩放累积效应
		float etaScale = 1;

		for (bounces = 0;; ++bounces) { 
			SurfaceInteraction isect;
			bool foundIntersection = scene.Intersect(ray, &isect);

			// 若是相机光线或者来自完美镜面弹射
			if (bounces == 0 || specularBounce) {
				// 击中：添加击中点自发光
				if (foundIntersection) {
					L += beta * isect.Le(-ray.d);
				}
				// 未击中：环境光
				else {
					for (const auto& light : scene.infiniteLights)
						L += beta * light->Le(ray);
				}
			}

			// 未击中/弹射最大次数 这条光线中止
			if (!foundIntersection || bounces >= maxDepth) break;

			// 加载材质，若为空，则让光线穿过
			isect.ComputeScatteringFunctions(ray, true);
			if (!isect.bsdf) {
				ray = isect.SpawnRay(ray.d);
				bounces--;
				continue;
			}

			// 根据交点，获得加权轮盘，均匀采样和能量采样将忽略输入参数
			const Distribution1D* distrib = lightDistribution->Lookup(isect.p);

			// 处理非镜面
			if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~BSDF_SPECULAR)) > 0) {
				++totalPaths;
				// 计算该点直接光照
				// 使用随机挑选一个光源的策略，包含多重重要性采样和阴影测试
				Spectrum Ld = beta * UniformSampleOneLight(isect, scene, sampler, false, distrib);
				if (Ld.IsBlack()) ++zeroRadiancePaths;
				L += Ld;
			}

			// 计算间接光照
			Vector3f wo = -ray.d, wi;
			float pdf;
			BxDFType flags;
			// BSDF 采样 wi
			Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf,
				BSDF_ALL, &flags);
			if (f.IsBlack() || pdf == 0.f) break;
			beta *= f * AbsDot(wi, isect.shading.n) / pdf;

			specularBounce = (flags & BSDF_SPECULAR) != 0;
			if ((flags & BSDF_SPECULAR) && (flags & BSDF_TRANSMISSION)) {
				float eta = isect.bsdf->eta;
				etaScale *= (Dot(wo, isect.n) > 0) ? (eta * eta) : 1 / (eta * eta);
			}
			// 创建下一次循环的光线
			ray = isect.SpawnRay(wi);
			Spectrum rrBeta = beta * etaScale;
			// 轮盘赌，弹射三次以上及beta足够暗会继续
			if (rrBeta.MaxComponentValue() < rrThreshold && bounces > 3) {
				//路径越暗，q越高
				float q = std::max((float).05, 1 - rrBeta.MaxComponentValue());
				if (sampler.Get1D() < q) break;
				beta /= 1 - q;
			}
		}
		return L;
	}




}









