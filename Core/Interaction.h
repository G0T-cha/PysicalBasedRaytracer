#pragma once
#ifndef INTERACTION_H
#define INTERACTION_H

#include "Core\Geometry.h"
#include "Shape\Shape.h"
#include "Material\Material.h"

namespace PBR {
	struct Interaction {
		Interaction() : time(0) {}
		Interaction(const Point3f& p, const Normal3f& n, const Vector3f& pError,
			const Vector3f& wo, float time)
			: p(p),
			time(time),
			pError(pError),
			wo(Normalize(wo)),
			n(n) {}
		Interaction(const Point3f& p, const Vector3f& wo, float time)
			: p(p), time(time), wo(wo) {}
		Interaction(const Point3f& p, float time)
			: p(p), time(time) {}
		//偏移光线
		Ray SpawnRay(const Vector3f& d) const {
			Point3f o = OffsetRayOrigin(p, pError, n, d);
			return Ray(o, d, Infinity, time);
		}
		//阴影
		Ray SpawnRayTo(const Point3f& p2) const {
			Point3f origin = OffsetRayOrigin(p, pError, n, p2 - p);
			Vector3f d = p2 - p;
			return Ray(origin, d, 1 - ShadowEpsilon, time);
		}
		//偏移阴影
		Ray SpawnRayTo(const Interaction& it) const {
			Point3f origin = OffsetRayOrigin(p, pError, n, it.p - p);
			Point3f target = OffsetRayOrigin(it.p, it.pError, it.n, origin - it.p);
			Vector3f d = target - origin;
			return Ray(origin, d, 1 - ShadowEpsilon, time);
		}
		Point3f p;
		float time;
		Vector3f pError;
		Vector3f wo;
		Normal3f n;
	};

	class SurfaceInteraction : public Interaction {
	public:
		SurfaceInteraction() {}
		SurfaceInteraction(const Point3f& p, const Vector3f& pError,
			const Point2f& uv, const Vector3f& wo,
			const Vector3f& dpdu, const Vector3f& dpdv,
			const Normal3f& dndu, const Normal3f& dndv, float time,
			const Shape* sh,
			int faceIndex = 0);
		~SurfaceInteraction();
		void ComputeScatteringFunctions(
			const Ray& ray,
			bool allowMultipleLobes = false,
			TransportMode mode = TransportMode::Radiance);

		Spectrum Le(const Vector3f& w) const;

		const Primitive* primitive = nullptr;
		std::shared_ptr<BSDF> bsdf = nullptr;
		Point2f uv;
		Vector3f dpdu, dpdv;
		Normal3f dndu, dndv;
		const Shape* shape = nullptr;
		struct {
			Normal3f n;
			Vector3f dpdu, dpdv;
			Normal3f dndu, dndv;
		} shading;
	};

}
#endif