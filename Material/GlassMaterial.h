#pragma once
#ifndef __GlassMaterial_h__
#define __GlassMaterial_h__

#include "Material\Material.h"
#include "Core\PBR.h"
#include "Texture\Texture.h"

namespace PBR {
class GlassMaterial : public Material {
  public:
    // Kr反射色（高光颜色）、Kt投射色（玻璃本身颜色）、Roughness粗糙度、index折射率
    GlassMaterial(const std::shared_ptr<Texture<Spectrum>> &Kr,
                  const std::shared_ptr<Texture<Spectrum>> &Kt,
                  const std::shared_ptr<Texture<float>> &uRoughness,
                  const std::shared_ptr<Texture<float>> &vRoughness,
                  const std::shared_ptr<Texture<float>> &index,
                  const std::shared_ptr<Texture<float>> &bumpMap,
                  bool remapRoughness)
        : Kr(Kr),
          Kt(Kt),
          uRoughness(uRoughness),
          vRoughness(vRoughness),
          index(index),
          bumpMap(bumpMap),
          remapRoughness(remapRoughness) {}
    void ComputeScatteringFunctions(SurfaceInteraction *si, 
                                    TransportMode mode,
                                    bool allowMultipleLobes) const;

  private:
    std::shared_ptr<Texture<Spectrum>> Kr, Kt;
    std::shared_ptr<Texture<float>> uRoughness, vRoughness;
    std::shared_ptr<Texture<float>> index;
    std::shared_ptr<Texture<float>> bumpMap;
    bool remapRoughness;
};

}







#endif





