#pragma once
#ifndef __PlasticMaterial_h__
#define __PlasticMaterial_h__

#include "Material\Material.h"
#include "Core\PBR.h"
#include "Texture\Texture.h"

namespace PBR {
class PlasticMaterial : public Material {
  public:
    // kdÂþ·´Éä£¬ks¾µÃæ·´Éä
    PlasticMaterial(const std::shared_ptr<Texture<Spectrum>> &Kd,
                    const std::shared_ptr<Texture<Spectrum>> &Ks,
                    const std::shared_ptr<Texture<float>> &roughness,
                    const std::shared_ptr<Texture<float>> &bumpMap,
                    bool remapRoughness)
        : Kd(Kd),
          Ks(Ks),
          roughness(roughness),
          bumpMap(bumpMap),
          remapRoughness(remapRoughness) {}
    void ComputeScatteringFunctions(SurfaceInteraction *si, TransportMode mode,
                                    bool allowMultipleLobes) const;

  private:
    std::shared_ptr<Texture<Spectrum>> Kd, Ks;
    std::shared_ptr<Texture<float>> roughness, bumpMap;
    const bool remapRoughness;
};





}







#endif





