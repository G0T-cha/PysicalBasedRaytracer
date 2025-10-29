#pragma once
#ifndef __Mirror_h__
#define __Mirror_h__

#include "Core\PBR.h"
#include "Material\Reflection.h"
#include "Texture\Texture.h"

namespace PBR {
class MirrorMaterial : public Material {
  public:
    //æµ√Ê∑¥…‰—’…´°¢∞ºÕπÃ˘Õº
    MirrorMaterial(const std::shared_ptr<Texture<Spectrum>> &r,
                   const std::shared_ptr<Texture<float>> &bump) {
        Kr = r;
        bumpMap = bump;
    }
    void ComputeScatteringFunctions(SurfaceInteraction *si, TransportMode mode,
                                    bool allowMultipleLobes) const;

  private:
    std::shared_ptr<Texture<Spectrum>> Kr;
    std::shared_ptr<Texture<float>> bumpMap;
};






}







#endif



