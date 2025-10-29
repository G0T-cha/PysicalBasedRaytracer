#include "Core\PBR.h"
#include "Shape\Shape.h"
#include "Core\interaction.h"

namespace PBR {
    static long long nShapesCreated = 0;
    Shape::Shape(const Transform* ObjectToWorld, const Transform* WorldToObject,
        bool reverseOrientation)
        : ObjectToWorld(ObjectToWorld),
        WorldToObject(WorldToObject),
        reverseOrientation(reverseOrientation), 
		transformSwapsHandedness(ObjectToWorld->SwapsHandedness()) {
        ++nShapesCreated;
    }
    Shape::~Shape() {}
    Bounds3f Shape::WorldBound() const { return (*ObjectToWorld)(ObjectBound()); }

	Interaction Shape::Sample(const Interaction& ref, const Point2f& u, float* pdf) const {
		Interaction intr = Sample(u, pdf);
		Vector3f wi = intr.p - ref.p;
		if (wi.LengthSquared() == 0)
			*pdf = 0;
		else {
			wi = Normalize(wi);
			// 雅可比行列式，将面积pdf转化为立体角pdf
			*pdf *= DistanceSquared(ref.p, intr.p) / AbsDot(intr.n, -wi);
			if (std::isinf(*pdf)) *pdf = 0.f;
		}
		return intr;
	}
	float Shape::Pdf(const Interaction& ref, const Vector3f& wi) const {
		//给定一个方向 wi，计算出光源采样策略采样到该方向的 PDF 值。
		Ray ray = ref.SpawnRay(wi);
		float tHit;
		SurfaceInteraction isectLight;
		if (!Intersect(ray, &tHit, &isectLight, false)) return 0;
		
		float pdf = DistanceSquared(ref.p, isectLight.p) /
			(AbsDot(isectLight.n, -wi) * Area());
		if (std::isinf(pdf)) pdf = 0.f;
		return pdf;
	}
}