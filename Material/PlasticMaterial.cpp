#include "Material\PlasticMaterial.h"
#include "Material\Reflection.h"
#include "Material\Microfacet.h"

namespace PBR {

// PlasticMaterial Method Definitions
void PlasticMaterial::ComputeScatteringFunctions(SurfaceInteraction *si, TransportMode mode,
    bool allowMultipleLobes) const {
    if (normalMap) {
        // --- a. 从法线贴图纹理中获取颜色值 ---
        Spectrum nMap = normalMap->Evaluate(*si).Clamp();

        // --- b. 将颜色 [0,1] 解包为法线向量 [-1,1] ---
        // (注意: 假设 G 通道是 Y，B 通道是 Z。如果您的贴图是 "DirectX" 风格，可能需要翻转 Y)
        float x = (nMap[0] * 2.f) - 1.f; // R -> X
        float y = (nMap[1] * 2.f) - 1.f; // G -> Y
        float z = (nMap[2] * 2.f) - 1.f; // B -> Z
        Vector3f tangentNormal = Normalize(Vector3f(x, y, z));

        // --- c. 将法线从切线空间 (TBN) 转换到世界空间 ---
        // (我们使用 SurfaceInteraction 中存储的 dpdu, dpdv 和 n)
        const Vector3f& dpdu = si->shading.dpdu;
        const Vector3f& dpdv = si->shading.dpdv;
        const Normal3f& n = si->shading.n;

        Vector3f worldNormal = Normalize(dpdu * tangentNormal.x +
            dpdv * tangentNormal.y +
            Vector3f(n) * tangentNormal.z);

        // --- d. 扰动 (Perturb) 着色法线 ---
        si->shading.n = Normal3f(worldNormal);

        // (可选：确保 dpdu/dpdv 与新法线正交)
        // si->shading.dpdu = Normalize(si->shading.dpdu - si->shading.n * Dot(si->shading.n, si->shading.dpdu));
        // si->shading.dpdv = Cross(si->shading.n, si->shading.dpdu);
    }
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












