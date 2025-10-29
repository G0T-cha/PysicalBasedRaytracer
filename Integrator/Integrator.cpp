#pragma once

#include "Integrator\Integrator.h"
#include "Sampler\Sampler.h"
#include "Core\Spectrum.h"
#include "Core\interaction.h"
#include "Core\Scene.h"
#include "Core\frameBuffer.h"
#include "Material\Reflection.h"
#include "Light/Light.h"
#include <omp.h>
#include <iomanip>

namespace PBR{

    Spectrum SamplerIntegrator::SpecularReflect(
        const Ray& ray, const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const {
        // ��ȡ���䷽��wo��ָ��۲��ߣ�
        Vector3f wo = isect.wo, wi;
        float pdf;
        BxDFType type = BxDFType(BSDF_REFLECTION | BSDF_SPECULAR);
        // ���ݳ��䷽��������淴��
        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, type);

        // ��ȡ��ɫ����
        const Normal3f& ns = isect.shading.n;

        if (wi.HasNaNs()) return 0.0f;

        // ��������Ч
        if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f) {
            // ������ȫ�������
            Ray rd = isect.SpawnRay(wi);
            // �ݹ���� Li() ���㷴����ߵĹ���
            // �������չ���
            return  f * Li(rd, scene, sampler, depth + 1) * AbsDot(wi, ns) / pdf;
        }
        else
            return Spectrum(0.f);
    }

	void SamplerIntegrator::Render(const Scene& scene, double& timeConsume) {
        //�����̵߳ĸ���
		omp_set_num_threads(10); 
        //��ȡ��ʼʱ�� 
		double start = omp_get_wtime(); 

#pragma omp parallel for
        for (int j = 0; j < pixelBounds.pMax.x; j++) {
            for (int i = 0; i < pixelBounds.pMax.y; i++) {
                // Ϊ��ǰ���ش���һ������������
                int offset = (pixelBounds.pMax.x * j + i);
                std::unique_ptr<Sampler> pixelSampler = sampler->Clone(offset);
                Point2i pixel(i, j);
                pixelSampler->StartPixel(pixel);
                // ��ʼ��������ɫΪ��ɫ
                Spectrum colObj(0.0f);             
                // ��ʼ�Ե������ؽ��ж�β���
                do {
                    // �Ӳ�������ȡ�������
                    CameraSample cameraSample = pixelSampler->GetCameraSample(pixel);
                    // ���������������һ������
                    Ray r;
                    camera->GenerateRay(cameraSample, &r);
                    SurfaceInteraction isect;
                    //��������ʵ�ֵ� Li() ���������ɫ
                    colObj += Li(r, scene, *pixelSampler, 0);                   
                } while (pixelSampler->StartNextSample()); // �ƶ�����ǰ���ص���һ������

                // �����������Ľ��ȡƽ��ֵ
                colObj /= (float)pixelSampler->samplesPerPixel;

                // �����ռ������ƽ����ɫ���µ�֡������
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 0, (unsigned char)(colObj[0] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 1, (unsigned char)(colObj[1] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 2, (unsigned char)(colObj[2] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 3, 255);
            }
            // ��һ��������
            if (omp_get_thread_num() == 0) {
                float progress = 100.0f * (j + 1) / pixelBounds.pMax.y;
                std::cout << "\rRendering progress: " << std::fixed << std::setprecision(2) << progress << "%" << std::flush;
            }
        }

		// ���㲢��ʾʱ��
		double end = omp_get_wtime();
		timeConsume = end - start;
	}
};
