#include "Light.h"

#include "Core\Scene.h"
#include "Core\interaction.h"

namespace PBR {
    static long long numLights = 0;
    static long long numAreaLights = 0;
    Light::Light(int flags, const Transform& LightToWorld, int nSamples)
        : flags(flags),
        nSamples(std::max(1, nSamples)),
        LightToWorld(LightToWorld),
        WorldToLight(Inverse(LightToWorld)) {
        ++numLights;
    }


    bool VisibilityTester::Unoccluded(const Scene& scene) const {
        //测试两个交点有无遮挡
        return !scene.IntersectP(p0.SpawnRayTo(p1));
    }

    AreaLight::AreaLight(const Transform& LightToWorld, int nSamples)
        : Light((int)LightFlags::Area, LightToWorld, nSamples) {
        ++numAreaLights;
    }
}