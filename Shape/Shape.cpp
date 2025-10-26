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
			// Convert from area measure, as returned by the Sample() call
			// above, to solid angle measure.
			*pdf *= DistanceSquared(ref.p, intr.p) / AbsDot(intr.n, -wi);
			if (std::isinf(*pdf)) *pdf = 0.f;
		}
		return intr;
	}
	float Shape::Pdf(const Interaction& ref, const Vector3f& wi) const {
		// Intersect sample ray with area light geometry
		Ray ray = ref.SpawnRay(wi);
		float tHit;
		SurfaceInteraction isectLight;
		// Ignore any alpha textures used for trimming the shape when performing
		// this intersection. Hack for the "San Miguel" scene, where this is used
		// to make an invisible area light.
		if (!Intersect(ray, &tHit, &isectLight, false)) return 0;

		// Convert light sample weight to solid angle measure
		float pdf = DistanceSquared(ref.p, isectLight.p) /
			(AbsDot(isectLight.n, -wi) * Area());
		if (std::isinf(pdf)) pdf = 0.f;
		return pdf;
	}
}