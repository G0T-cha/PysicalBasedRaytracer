#pragma once
#ifndef INTERACTION_H
#define INTERACTION_H

#include "Core\Geometry.h"
#include "Shape\Shape.h"
#include "Material\Material.h"
#include "Media\Medium.h"

namespace PBR {
	struct Interaction {
		Interaction() : time(0) {}
		Interaction(const Point3f& p, const Normal3f& n, const Vector3f& pError,
			const Vector3f& wo, float time, const MediumInterface &mediumInterface)
			: p(p),
			time(time),
			pError(pError),
			wo(Normalize(wo)),
			n(n),
			mediumInterface(mediumInterface) {}
		Interaction(const Point3f& p, const Vector3f& wo, float time, const MediumInterface& mediumInterface)
			: p(p), time(time), wo(wo), mediumInterface(mediumInterface) {}
		Interaction(const Point3f& p, float time, const MediumInterface& mediumInterface)
			: p(p), time(time), mediumInterface(mediumInterface) {}
		// 是否是物理表面
		bool IsSurfaceInteraction() const { return n != Normal3f(); }
		// 偏移光线
		Ray SpawnRay(const Vector3f& d) const {
			Point3f o = OffsetRayOrigin(p, pError, n, d);
			return Ray(o, d, Infinity, time, GetMedium(d));
		}
		// 阴影
		Ray SpawnRayTo(const Point3f& p2) const {
			Point3f origin = OffsetRayOrigin(p, pError, n, p2 - p);
			Vector3f d = p2 - p;
			return Ray(origin, d, 1 - ShadowEpsilon, time, GetMedium(d));
		}
		//偏移阴影
		Ray SpawnRayTo(const Interaction& it) const {
			Point3f origin = OffsetRayOrigin(p, pError, n, it.p - p);
			Point3f target = OffsetRayOrigin(it.p, it.pError, it.n, origin - it.p);
			Vector3f d = target - origin;
			return Ray(origin, d, 1 - ShadowEpsilon, time, GetMedium(d));
		}
		// 判断这个交点是否是介质内部的散射点
		bool IsMediumInteraction() const { return !IsSurfaceInteraction(); }
		// 根据光线方向判断进入哪一侧的介质
		const Medium* GetMedium(const Vector3f& w) const {
			return Dot(w, n) > 0 ? mediumInterface.outside : mediumInterface.inside;
		}
		const Medium* GetMedium() const {
			//CHECK_EQ(mediumInterface.inside, mediumInterface.outside);
			return mediumInterface.inside;
		}
		Point3f p;
		float time;
		Vector3f pError;
		Vector3f wo;
		Normal3f n;
		MediumInterface mediumInterface;
	};

	// 参与介质散射-体积交点
	class MediumInteraction : public Interaction {
	public:
		MediumInteraction() : phase(nullptr) {}
		MediumInteraction(const Point3f& p, const Vector3f& wo, float time,
			const Medium* medium, PhaseFunction* phase)
			: Interaction(p, wo, time, medium), phase(phase) {}
		bool IsValid() const { return phase != nullptr; }

		// 指向相位函数的智能指针
		std::shared_ptr<PhaseFunction> phase = nullptr;
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

		void ComputeDifferentials(const RayDifferential& ray) const;
		void SetShadingGeometry(const Vector3f& dpdu, const Vector3f& dpdv, const Normal3f& dndu, const Normal3f& dndv, bool orientationIsAuthoritative);		
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
		mutable Vector3f dpdx, dpdy; //光线微分方向
		mutable float dudx = 0, dvdx = 0, dudy = 0, dvdy = 0;

		mutable bool testd = false;
	};

}
#endif