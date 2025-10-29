#pragma once
#ifndef __MatteMaterial_h__
#define __MatteMaterial_h__
#include "Material\Material.h"
#include "Texture\Texture.h"

namespace PBR {
    class MatteMaterial : public Material {
    public:
        // Âş·´ÉäÑÕÉ«¡¢´Ö²Ú¶È¡¢°¼Í¹ÌùÍ¼
        MatteMaterial(const std::shared_ptr<Texture<Spectrum>>& Kd,
            const std::shared_ptr<Texture<float>>& sigma,
            const std::shared_ptr<Texture<float>>& bumpMap)
            : Kd(Kd), sigma(sigma), bumpMap(bumpMap) {}
        void ComputeScatteringFunctions(SurfaceInteraction* si,
            TransportMode mode,
            bool allowMultipleLobes) const;
    private:
        std::shared_ptr<Texture<Spectrum>> Kd;
        std::shared_ptr<Texture<float>> sigma, bumpMap;
    };

}

#endif