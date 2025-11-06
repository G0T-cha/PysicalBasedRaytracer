#include "Media\HomogeneousMedium.h"
#include "Core\Geometry.h"
#include "Core\PBR.h"
#include "Sampler\Sampler.h"
#include "Core\interaction.h"

namespace PBR {

// 比尔-朗伯定律
Spectrum HomogeneousMedium::Tr(const Ray &ray, Sampler &sampler) const {
    return Exp(-sigma_t * std::min(ray.tMax * ray.d.Length(), MaxFloat));
}

// 介质采样
Spectrum HomogeneousMedium::Sample(const Ray &ray, Sampler &sampler,
                                   MediumInteraction *mi) const {
    // 采样一个散射距离
    int channel = std::min((int)(sampler.Get1D() * Spectrum::nSamples),
                           Spectrum::nSamples - 1);
    float dist = -std::log(1 - sampler.Get1D()) / sigma_t[channel];
    // 决定是散射还是击中表面
    float t = std::min(dist / ray.d.Length(), ray.tMax);
    bool sampledMedium = t < ray.tMax;
    if (sampledMedium)
        *mi = MediumInteraction(ray(t), -ray.d, ray.time, this,
                                new HenyeyGreenstein(g));

    // 如果在介质中散射，创建交点
    Spectrum Tr = Exp(-sigma_t * std::min(t, MaxFloat) * ray.d.Length());

    // 计算到该点(t)的透射率
    Spectrum density = sampledMedium ? (sigma_t * Tr) : Tr;
    
    // 计算采样密度(PDF)
    float pdf = 0;
    for (int i = 0; i < Spectrum::nSamples; ++i) pdf += density[i];
    pdf *= 1 / (float)Spectrum::nSamples;
    if (pdf == 0) {
        //CHECK(Tr.IsBlack());
        pdf = 1;
    }

    // 返回蒙特卡洛权重
    return sampledMedium ? (Tr * sigma_s / pdf) : (Tr / pdf);
}


}









