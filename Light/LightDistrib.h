#pragma once
#ifndef __LightDistrib_h__
#define __LightDistrib_h__

#include "Core\PBR.h"
#include <string>

namespace PBR {
    class LightDistribution {
    public:
        virtual ~LightDistribution() {}

        // 根据场景中的一个具体位置 p，返回一个最适合在该位置采样光源的加权轮盘 Distribution1D
        virtual const Distribution1D* Lookup(const Point3f& p) const = 0;
    };

    // 根据名字来创建并返回一个具体的光源采样策略
    std::unique_ptr<LightDistribution> CreateLightSampleDistribution(
        const std::string& name, const Scene& scene);

    // 均匀采样策略
    class UniformLightDistribution : public LightDistribution {
    public:
        UniformLightDistribution(const Scene& scene);
        const Distribution1D* Lookup(const Point3f& p) const;

    private:
        std::unique_ptr<Distribution1D> distrib;
    };

    // 能量密度采样策略
    class PowerLightDistribution : public LightDistribution {
    public:
        PowerLightDistribution(const Scene& scene);
        const Distribution1D* Lookup(const Point3f& p) const;

    private:
        std::unique_ptr<Distribution1D> distrib;
    };
}



#endif
