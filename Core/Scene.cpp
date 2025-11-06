#include "Core\Scene.h"

namespace PBR {
	static long long nIntersectionTests = 0;
	static long long nShadowTests = 0;

	Scene::Scene(std::shared_ptr<Primitive> aggregate,
		const std::vector<std::shared_ptr<Light>>& lights)
		: lights(lights), aggregate(aggregate) {
		// Scene Constructor Implementation
		worldBound = aggregate->WorldBound();
		for (const auto& light : lights) {
			light->Preprocess(*this);
			if (light->flags & (int)LightFlags::Infinite)
				infiniteLights.push_back(light);
		}
	}

	// Scene Method Definitions
	bool Scene::Intersect(const Ray& ray, SurfaceInteraction* isect) const {
		++nIntersectionTests;
		return aggregate->Intersect(ray, isect);
	}

	bool Scene::IntersectP(const Ray& ray) const {
		++nShadowTests;
		return aggregate->IntersectP(ray);
	}

	// 沿着一条光线追踪其在场景中的传播过程，
	// 计算它在穿过介质时的透射率
	// 同时判断它是否最终被不透明表面挡住
	bool Scene::IntersectTr(Ray ray, Sampler& sampler, SurfaceInteraction* isect,
		Spectrum* Tr) const {
		*Tr = Spectrum(1.f);
		while (true) {
			bool hitSurface = Intersect(ray, isect);
			if (ray.medium) *Tr *= ray.medium->Tr(ray, sampler);
			if (!hitSurface) return false;
			if (isect->primitive->GetMaterial() != nullptr) return true;
			ray = isect->SpawnRay(ray.d);
		}
	}
}