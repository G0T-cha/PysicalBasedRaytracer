#include "Light.h"

#include "Core\Scene.h"
#include "Core\interaction.h"

namespace PBR {
    static long long numLights = 0;
    static long long numAreaLights = 0;
    Light::Light(int flags, const Transform& LightToWorld, const MediumInterface& mediumInterface, int nSamples)
        : flags(flags),
        nSamples(std::max(1, nSamples)),
        mediumInterface(mediumInterface),
        LightToWorld(LightToWorld),
        WorldToLight(Inverse(LightToWorld)) {
        ++numLights;
    }


    bool VisibilityTester::Unoccluded(const Scene& scene) const {
        //测试两个交点有无遮挡
        return !scene.IntersectP(p0.SpawnRayTo(p1));
    }

    AreaLight::AreaLight(const Transform& LightToWorld, const MediumInterface& medium, int nSamples)
        : Light((int)LightFlags::Area, LightToWorld, medium, nSamples) {
        ++numAreaLights;
    }

    // 从点 p0 传播到 p1 的过程中
    // 有多少能量（光通量）能透过介质和表面到达终点
    Spectrum VisibilityTester::Tr(const Scene& scene, Sampler& sampler) const {
        Ray ray(p0.SpawnRayTo(p1));
        Spectrum Tr(1.f);
        while (true) {
            SurfaceInteraction isect;
            bool hitSurface = scene.Intersect(ray, &isect);
            // Handle opaque surface along ray's path
            if (hitSurface && isect.primitive->GetMaterial() != nullptr)
                return Spectrum(0.0f);
            // Update transmittance for current ray segment
            if (ray.medium) Tr *= ray.medium->Tr(ray, sampler);
            // Generate next ray segment or return final transmittance
            if (!hitSurface) break;
            ray = isect.SpawnRayTo(p1);
        }
        return Tr;
    }
}