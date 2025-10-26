#pragma
#ifndef SHAPE_H
#define SHAPE_H

#include "Core/Geometry.h"
#include "Core/PBR.h"
#include "Core/Transform.h"

namespace PBR {

	class Shape {
	public:
		Shape(const Transform* ObjectToWorld, const Transform* WorldToObject,
			bool reverseOrientation);
		virtual ~Shape();
		virtual Bounds3f ObjectBound() const = 0;
		virtual Bounds3f WorldBound() const;
		virtual bool Intersect(const Ray& ray, float* tHit,
			SurfaceInteraction* isect,
			bool testAlphaTexture = true) const = 0;
		virtual bool IntersectP(const Ray& ray,
			bool testAlphaTexture = true) const {
			return Intersect(ray, nullptr, nullptr, testAlphaTexture);
		}
		virtual float Area() const = 0;

		// Sample a point on the surface of the shape and return the PDF with
		// respect to area on the surface.
		virtual Interaction Sample(const Point2f& u, float* pdf) const = 0;
		virtual float Pdf(const Interaction&) const { return 1 / Area(); }

		// Sample a point on the shape given a reference point |ref| and
		// return the PDF with respect to solid angle from |ref|.
		virtual Interaction Sample(const Interaction& ref, const Point2f& u, float* pdf) const;
		virtual float Pdf(const Interaction& ref, const Vector3f& wi) const;

		const Transform* ObjectToWorld, * WorldToObject;
		const bool reverseOrientation;
		const bool transformSwapsHandedness;
	};

}


#endif // 


