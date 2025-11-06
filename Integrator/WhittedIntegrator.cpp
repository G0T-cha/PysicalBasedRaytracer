#include "Integrator\WhittedIntegrator.h"
#include "Core\Spectrum.h"
#include "Core\Interaction.h"
#include "Core\Scene.h"
#include "Light\Light.h"
#include "Sampler\Sampler.h"
#include "Material\Reflection.h"


namespace PBR {
Spectrum WhittedIntegrator::Li(const RayDifferential&ray, const Scene &scene,
                               Sampler &sampler, int depth) const {
    Spectrum L(0.);
    // 查找光线与场景的最近交点
    SurfaceInteraction isect;
    if (!scene.Intersect(ray, &isect)) {
        // 若未击中任何物体，遍历光源，累加无穷远光源自发光
        for (const auto& light : scene.lights) L += light->Le(ray);
        return L;
    }

	// 击中物体
    // 获取着色法线和出射方向
    const Normal3f &n = isect.shading.n;
    Vector3f wo = isect.wo;

    // 构建BSDF
    // 内部调用 primitive->material->ComputeScatteringFunctions()，创建 BSDF 并设置 isect.bsdf 指针
    isect.ComputeScatteringFunctions(ray);

    // 若没有BSDF，则沿原方向继续追踪
	if (!isect.bsdf) return Li(isect.SpawnRay(ray.d), scene, sampler, depth);

    // 若击中面光源，累计自发光
    L += isect.Le(wo);

    // 计算直接光照
    // 遍历场景全部光源
    for (const auto &light : scene.lights) {
        Vector3f wi;
        float pdf;
        VisibilityTester visibility;
        // 光源采样
        Spectrum Li = light->Sample_Li(isect, sampler.Get2D(), &wi, &pdf, &visibility);
        // 光源无贡献或采样失败，跳过
        if (Li.IsBlack() || pdf == 0) continue;

        // 求值BSDF
        Spectrum f = isect.bsdf->f(wo, wi);
        // 若阴影测试通过
        if (!f.IsBlack() && visibility.Unoccluded(scene))
            // 累加光照贡献
            L += f * Li * AbsDot(wi, n) / pdf;
    }

    // 计算镜面反射带来的间接光照
    if (depth + 1 < maxDepth) { //若未到达递归最大深度
        // 处理镜面反射
        // 这里内部利用了BSDF类型参数隐式地判断当前材质是否包含镜面反射组件
        // 如果包含，成功并返回非零值，触发递归
        // 如果不包含，递归被跳过
		L += SpecularReflect(ray, isect, scene, sampler, depth);
    }
    return L;
}
}



















