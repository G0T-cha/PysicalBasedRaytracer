#include "Material\Mirror.h"


namespace PBR {
void MirrorMaterial::ComputeScatteringFunctions(SurfaceInteraction *si,
                                                TransportMode mode,
                                                bool allowMultipleLobes) const {
    //if (bumpMap) Bump(bumpMap, si);
    
	si->bsdf = std::make_shared<BSDF>(*si);
    Spectrum R = Kr->Evaluate(*si).Clamp();
    //添加镜面反射BxDF，使用 FresnelNoOp 代表 100% 反射
    if (!R.IsBlack())
        si->bsdf->Add(new SpecularReflection(R, new FresnelNoOp));
    }
}












