#include "Light\LightDistrib.h"
#include "Sampler\Sampler.h"
#include "Core\Geometry.h"
#include "Core\Interaction.h"
#include "Core\Scene.h"
#include "Light\Light.h"
#include "Material\Reflection.h"
#include "Integrator\VolPathIntegrator.h"
#include "Core\Spectrum.h"

namespace PBR {

static long long volumeInteractions = 0;
static long long surfaceInteractions = 0;

void VolPathIntegrator::Preprocess(const Scene &scene, Sampler &sampler) {
	lightDistribution =
		CreateLightSampleDistribution(lightSampleStrategy, scene);
}

Spectrum VolPathIntegrator::Li(const RayDifferential &r, const Scene &scene,
	Sampler &sampler, int depth) const {	
	Spectrum L(0.f), beta(1.f);
	RayDifferential ray(r);
	bool specularBounce = false;
	int bounces;
	float etaScale = 1;

	for (bounces = 0;; ++bounces) {
		SurfaceInteraction isect;
		bool foundIntersection = scene.Intersect(ray, &isect);

		// 若光线当前穿过介质，进行体积散射采样
		MediumInteraction mi;
		// 会随机决定：是否在体积中产生一次散射，若有，散射点信息放入 mi，返回该段路径的透射率
		if (ray.medium) beta *= ray.medium->Sample(ray, sampler, &mi);
		if (beta.IsBlack()) break;

		// 如果散射
		if (mi.IsValid()) {
			// 计算在体积点处光源直接照明的贡献
			if (bounces >= maxDepth) break;
			++volumeInteractions;
			const Distribution1D *lightDistrib =
				lightDistribution->Lookup(mi.p);
			L += beta * UniformSampleOneLight(mi, scene, sampler, true, lightDistrib);

			// 采样新的散射方向，继续路径追踪
			Vector3f wo = -ray.d, wi;
			mi.phase->Sample_p(wo, &wi, sampler.Get2D());
			ray = mi.SpawnRay(wi);
			specularBounce = false;
		}
		// 后面和路径追踪一样
		else {
			++surfaceInteractions;
			if (bounces == 0 || specularBounce) {
				if (foundIntersection)
					L += beta * isect.Le(-ray.d);
				else
					for (const auto &light : scene.infiniteLights)
						L += beta * light->Le(ray);
			}

			if (!foundIntersection || bounces >= maxDepth) break;

			isect.ComputeScatteringFunctions(ray, true);
			if (!isect.bsdf) {
				ray = isect.SpawnRay(ray.d);
				bounces--;
				continue;
			}

			const Distribution1D *lightDistrib =
				lightDistribution->Lookup(isect.p);
			L += beta * UniformSampleOneLight(isect, scene, sampler, true, lightDistrib);

			Vector3f wo = -ray.d, wi;
			float pdf;
			BxDFType flags;
			Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf,
				BSDF_ALL, &flags);
			if (f.IsBlack() || pdf == 0.f) break;
			beta *= f * AbsDot(wi, isect.shading.n) / pdf;
			//DCHECK(std::isinf(beta.y()) == false);
			specularBounce = (flags & BSDF_SPECULAR) != 0;
			if ((flags & BSDF_SPECULAR) && (flags & BSDF_TRANSMISSION)) {
				float eta = isect.bsdf->eta;
				etaScale *=
					(Dot(wo, isect.n) > 0) ? (eta * eta) : 1 / (eta * eta);
			}
			ray = isect.SpawnRay(wi);
		}

		Spectrum rrBeta = beta * etaScale;
		if (rrBeta.MaxComponentValue() < rrThreshold && bounces > 3) {
			float q = std::max((float).05, 1 - rrBeta.MaxComponentValue());
			if (sampler.Get1D() < q) break;
			beta /= 1 - q;
			//DCHECK(std::isinf(beta.y()) == false);
		}
	}


	return L;

}





}






