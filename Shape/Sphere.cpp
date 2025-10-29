#include "Core\PBR.h"
#include "Shape\Sphere.h"
#include "Core\Interaction.h"


namespace PBR {

    bool Sphere::Intersect(const Ray& r, float* tHit, SurfaceInteraction* isect,
        bool testAlphaTexture) const {

        Point3f pHit;
        //将光线转化到物体空间求交
        Ray ray = (*WorldToObject)(r);

        Vector3f oc = ray.o - Point3f(0.0f, 0.0f, 0.0f);
        float a = Dot(ray.d, ray.d);
        float b = 2.0 * Dot(oc, ray.d);
        float c = Dot(oc, oc) - radius * radius;
        float discriminant = b * b - 4 * a * c;

        return (discriminant > 0);
    }
    bool Sphere::IntersectP(const Ray& r, bool testAlphaTexture) const {
        return false;
    }

    Bounds3f Sphere::ObjectBound() const {
        return Bounds3f(Point3f(-radius, -radius, -radius),
            Point3f(radius, radius, radius));
    }

    float Sphere::Area() const { return 4 * Pi * radius * radius; }

    Interaction Sphere::Sample(const Point2f& u, float* pdf) const {
        Interaction it;
        return it;
    }


}