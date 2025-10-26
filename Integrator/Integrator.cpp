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
        // Compute specular reflection direction _wi_ and BSDF value
        Vector3f wo = isect.wo, wi;
        float pdf;
        BxDFType type = BxDFType(BSDF_REFLECTION | BSDF_SPECULAR);
        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, type);

        // Return contribution of specular reflection
        const Normal3f& ns = isect.shading.n;

        if (wi.HasNaNs()) return 1.0f;

        if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f) {
            // Compute ray differential _rd_ for specular reflection
            Ray rd = isect.SpawnRay(wi);
            return  f * Li(rd, scene, sampler, depth + 1) * AbsDot(wi, ns) / pdf;
        }
        else
            return Spectrum(0.f);
    }

	void SamplerIntegrator::Render(const Scene& scene, double& timeConsume) {

		omp_set_num_threads(10); //�����̵߳ĸ���
		double start = omp_get_wtime();//��ȡ��ʼʱ��  

#pragma omp parallel for
        for (int j = 0; j < pixelBounds.pMax.x; j++) {
            for (int i = 0; i < pixelBounds.pMax.y; i++) {
                // Ϊ��ǰ���ش���һ�������ġ�ȷ���ԵĲ���������
                int offset = (pixelBounds.pMax.x * j + i);
                std::unique_ptr<Sampler> pixelSampler = sampler->Clone(offset);

                Point2i pixel(i, j);
                pixelSampler->StartPixel(pixel);

                Spectrum colObj(0.0f); // ��ʼ��������ɫΪ��ɫ
               

                // ��ʼ�Ե������ؽ��ж�β��� (SPP)
                do {
                    // �Ӳ�������ȡһ����������������ߵ�����
                    CameraSample cameraSample = pixelSampler->GetCameraSample(pixel);
                    // ���������������һ������ (ʹ�����cam)
                    Ray r;
                    camera->GenerateRay(cameraSample, &r);
                    SurfaceInteraction isect;
                    colObj += Li(r, scene, *pixelSampler, 0);                   

                } while (pixelSampler->StartNextSample()); // �ƶ�����ǰ���ص���һ������

                // �����������Ľ��ȡƽ��ֵ
                colObj /= (float)pixelSampler->samplesPerPixel;

                // 8. �����ռ������ƽ����ɫ���µ�֡������
                //    ����Ҫ������� FrameBuffer API ���е���
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 0, (unsigned char)(colObj[0] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 1, (unsigned char)(colObj[1] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 2, (unsigned char)(colObj[2] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 3, 255);
            }
            // ���Լ�һ��������
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
