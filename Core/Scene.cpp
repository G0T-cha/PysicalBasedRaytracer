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
			//if (light->flags & (int)LightFlags::Infinite)
			//	infiniteLights.push_back(light);
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
}