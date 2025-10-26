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

		omp_set_num_threads(10); //设置线程的个数
		double start = omp_get_wtime();//获取起始时间  

#pragma omp parallel for
        for (int j = 0; j < pixelBounds.pMax.x; j++) {
            for (int i = 0; i < pixelBounds.pMax.y; i++) {
                // 为当前像素创建一个独立的、确定性的采样器副本
                int offset = (pixelBounds.pMax.x * j + i);
                std::unique_ptr<Sampler> pixelSampler = sampler->Clone(offset);

                Point2i pixel(i, j);
                pixelSampler->StartPixel(pixel);

                Spectrum colObj(0.0f); // 初始化像素颜色为黑色
               

                // 开始对单个像素进行多次采样 (SPP)
                do {
                    // 从采样器获取一个用于生成相机光线的样本
                    CameraSample cameraSample = pixelSampler->GetCameraSample(pixel);
                    // 根据相机样本生成一条光线 (使用你的cam)
                    Ray r;
                    camera->GenerateRay(cameraSample, &r);
                    SurfaceInteraction isect;
                    colObj += Li(r, scene, *pixelSampler, 0);                   

                } while (pixelSampler->StartNextSample()); // 移动到当前像素的下一个样本

                // 对所有样本的结果取平均值
                colObj /= (float)pixelSampler->samplesPerPixel;

                // 8. 将最终计算出的平均颜色更新到帧缓冲区
                //    你需要根据你的 FrameBuffer API 进行调整
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 0, (unsigned char)(colObj[0] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 1, (unsigned char)(colObj[1] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 2, (unsigned char)(colObj[2] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 3, 255);
            }
            // 可以加一个进度条
            if (omp_get_thread_num() == 0) {
                float progress = 100.0f * (j + 1) / pixelBounds.pMax.y;
                std::cout << "\rRendering progress: " << std::fixed << std::setprecision(2) << progress << "%" << std::flush;
            }
        }

		// 计算并显示时间
		double end = omp_get_wtime();
		timeConsume = end - start;

	}


};
