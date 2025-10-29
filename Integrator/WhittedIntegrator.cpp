#include "Integrator\WhittedIntegrator.h"
#include "Core\Spectrum.h"
#include "Core\Interaction.h"
#include "Core\Scene.h"
#include "Light\Light.h"
#include "Sampler\Sampler.h"
#include "Material\Reflection.h"


namespace PBR {
Spectrum WhittedIntegrator::Li(const Ray &ray, const Scene &scene,
                               Sampler &sampler, int depth) const {
    Spectrum L(0.);
    // ���ҹ����볡�����������
    SurfaceInteraction isect;
    if (!scene.Intersect(ray, &isect)) {
        // ��δ�����κ����壬������Դ���ۼ�����Զ��Դ�Է���
        for (const auto& light : scene.lights) L += light->Le(ray);
        return L;
    }

	// ��������
    // ��ȡ��ɫ���ߺͳ��䷽��
    const Normal3f &n = isect.shading.n;
    Vector3f wo = isect.wo;

    // ����BSDF
    // �ڲ����� primitive->material->ComputeScatteringFunctions()������ BSDF ������ isect.bsdf ָ��
    isect.ComputeScatteringFunctions(ray);

    // ��û��BSDF������ԭ�������׷��
	if (!isect.bsdf) return Li(isect.SpawnRay(ray.d), scene, sampler, depth);

    // ���������Դ���ۼ��Է���
    L += isect.Le(wo);

    // ����ֱ�ӹ���
    // ��������ȫ����Դ
    for (const auto &light : scene.lights) {
        Vector3f wi;
        float pdf;
        VisibilityTester visibility;
        // ��Դ����
        Spectrum Li = light->Sample_Li(isect, sampler.Get2D(), &wi, &pdf, &visibility);
        // ��Դ�޹��׻����ʧ�ܣ�����
        if (Li.IsBlack() || pdf == 0) continue;

        // ��ֵBSDF
        Spectrum f = isect.bsdf->f(wo, wi);
        // ����Ӱ����ͨ��
        if (!f.IsBlack() && visibility.Unoccluded(scene))
            // �ۼӹ��չ���
            L += f * Li * AbsDot(wi, n) / pdf;
    }

    // ���㾵�淴������ļ�ӹ���
    if (depth + 1 < maxDepth) { //��δ����ݹ�������
        // �����淴��
        // �����ڲ�������BSDF���Ͳ�����ʽ���жϵ�ǰ�����Ƿ�������淴�����
        // ����������ɹ������ط���ֵ�������ݹ�
        // ������������ݹ鱻����
		L += SpecularReflect(ray, isect, scene, sampler, depth);
    }
    return L;
}
}



















