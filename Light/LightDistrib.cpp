#include "Light\LightDistrib.h"
#include "Sampler\Sampling.h"
#include <vector>
#include "Core\Scene.h"

namespace PBR {

    std::unique_ptr<LightDistribution> CreateLightSampleDistribution(
        const std::string& name, const Scene& scene) {
        if (name == "uniform" || scene.lights.size() == 1)
            return std::unique_ptr<LightDistribution>{
            new UniformLightDistribution(scene)};
        else if (name == "power")
            return std::unique_ptr<LightDistribution>{
            new PowerLightDistribution(scene)};
        /*else if (name == "spatial")
            return std::unique_ptr<LightDistribution>{
            new SpatialLightDistribution(scene)};*/
        else {
            return std::unique_ptr<LightDistribution>{
                new UniformLightDistribution(scene)};
        }
    }

    UniformLightDistribution::UniformLightDistribution(const Scene& scene) {
        // 光源数量等长的权重列表
        std::vector<float> prob(scene.lights.size(), float(1));
        distrib.reset(new Distribution1D(&prob[0], int(prob.size())));
    }

    const Distribution1D* UniformLightDistribution::Lookup(const Point3f& p) const {
        return distrib.get();
    }

    PowerLightDistribution::PowerLightDistribution(const Scene& scene) {
        std::vector<float> lightPower;
        // 将光源亮度作为权重列表
        for (const auto& light : scene.lights)
            lightPower.push_back(light->Power().y());
        distrib.reset(new Distribution1D(&lightPower[0], lightPower.size()));
    }

    const Distribution1D* PowerLightDistribution::Lookup(const Point3f& p) const {
        return distrib.get();
    }




}