#include "Core\PBR.h"

#include "Material\MatteMaterial.h"
#include "Material\Reflection.h"

#include "Core\Spectrum.h"
#include "Core\interaction.h"


namespace PBR {

    // MatteMaterial Method Definitions
    void MatteMaterial::ComputeScatteringFunctions(SurfaceInteraction* si,
        TransportMode mode,
        bool allowMultipleLobes) const {
        // 创建BSDF容器
        si->bsdf = std::make_shared<BSDF>(*si);
        //获取漫反射颜色
        Spectrum r = Kd->Evaluate(*si).Clamp();
        float sig = Clamp(sigma->Evaluate(*si), 0, 90);
        if (!r.IsBlack()) {
            //为交点添加漫反射BxDF模型
            if (sig == 0)
                si->bsdf->Add(new LambertianReflection(r));
            else
                si->bsdf->Add(new OrenNayar(r, sig));
        }
    }


}