#pragma once
#ifndef __SCENE_H__
#define __SCENE_H__

#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\primitive.h"
#include <vector>
#include "Light/Light.h"

namespace PBR {

	class Scene {
	public:
		// Scene Public Methods
		Scene(std::shared_ptr<Primitive> aggregate,
			const std::vector<std::shared_ptr<Light>>& lights);
		const Bounds3f& WorldBound() const { return worldBound; }
		bool Intersect(const Ray& ray, SurfaceInteraction* isect) const;
		bool IntersectP(const Ray& ray) const;

		std::vector<std::shared_ptr<Light>> lights;
	private:
		// Scene Private Data
		std::shared_ptr<Primitive> aggregate;
		Bounds3f worldBound;
	};




}

#endif