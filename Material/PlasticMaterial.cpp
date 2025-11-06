#include "Material\PlasticMaterial.h"
#include "Material\Reflection.h"
#include "Material\Microfacet.h"

namespace PBR {

// PlasticMaterial Method Definitions
void PlasticMaterial::ComputeScatteringFunctions(SurfaceInteraction *si, TransportMode mode,
    bool allowMultipleLobes) const {
    //if (bumpMap) Bump(bumpMap, si);
    si->bsdf = std::make_shared<BSDF>(*si);
    // 底色
    Spectrum kd = Kd->Evaluate(*si).Clamp();
    if (!kd.IsBlack())
        si->bsdf->Add(new LambertianReflection(kd));

    // 高光
    Spectrum ks = Ks->Evaluate(*si).Clamp();
    if (!ks.IsBlack()) {
        // 菲涅尔 - 塑料
        Fresnel *fresnel = new FresnelDielectric(1.5f, 1.f);
        float rough = roughness->Evaluate(*si);
        if (remapRoughness)
            rough = TrowbridgeReitzDistribution::RoughnessToAlpha(rough);
        // 微反射
        MicrofacetDistribution *distrib = new TrowbridgeReitzDistribution(rough, rough);
        BxDF *spec = new MicrofacetReflection(ks, distrib, fresnel);
        si->bsdf->Add(spec);
    }
}


}












