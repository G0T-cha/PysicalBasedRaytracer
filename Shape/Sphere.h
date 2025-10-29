#pragma once
#ifndef SPHERE_H
#define SPHERE_H

#include "Core\PBR.h"
#include "Shape\Shape.h"

namespace PBR {
    class Sphere : public Shape {
    public:
        Sphere(const Transform* ObjectToWorld, const Transform* WorldToObject,
            bool reverseOrientation, float radius)
            : Shape(ObjectToWorld, WorldToObject, reverseOrientation),
            radius(radius) {}
        Bounds3f ObjectBound() const;
        bool Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect,
            bool testAlphaTexture) const;
        bool IntersectP(const Ray& ray, bool testAlphaTexture) const;
        float Area() const;
        Interaction Sample(const Point2f& u, float* pdf) const;
    private:
        const float radius;
    };

}

#endif