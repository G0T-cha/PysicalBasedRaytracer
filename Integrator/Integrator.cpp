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
        // 获取出射方向wo（指向观察者）
        Vector3f wo = isect.wo, wi;
        float pdf;
        BxDFType type = BxDFType(BSDF_REFLECTION | BSDF_SPECULAR);
        // 根据出射方向采样镜面反射
        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, type);

        // 获取着色法线
        const Normal3f& ns = isect.shading.n;

        if (wi.HasNaNs()) return 0.0f;

        // 若采样有效
        if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f) {
            // 创建安全反射光线
            Ray rd = isect.SpawnRay(wi);
            // 递归调用 Li() 计算反射光线的贡献
            // 计算最终贡献
            return  f * Li(rd, scene, sampler, depth + 1) * AbsDot(wi, ns) / pdf;
        }
        else
            return Spectrum(0.f);
    }

	void SamplerIntegrator::Render(const Scene& scene, double& timeConsume) {
        //设置线程的个数
		omp_set_num_threads(10); 
        //获取起始时间 
		double start = omp_get_wtime(); 

#pragma omp parallel for
        for (int j = 0; j < pixelBounds.pMax.x; j++) {
            for (int i = 0; i < pixelBounds.pMax.y; i++) {
                // 为当前像素创建一个采样器副本
                int offset = (pixelBounds.pMax.x * j + i);
                std::unique_ptr<Sampler> pixelSampler = sampler->Clone(offset);
                Point2i pixel(i, j);
                pixelSampler->StartPixel(pixel);
                // 初始化像素颜色为黑色
                Spectrum colObj(0.0f);             
                // 开始对单个像素进行多次采样
                do {
                    // 从采样器获取相机样本
                    CameraSample cameraSample = pixelSampler->GetCameraSample(pixel);
                    // 根据相机样本生成一条光线
                    Ray r;
                    camera->GenerateRay(cameraSample, &r);
                    SurfaceInteraction isect;
                    //调用子类实现的 Li() 计算光线颜色
                    colObj += Li(r, scene, *pixelSampler, 0);                   
                } while (pixelSampler->StartNextSample()); // 移动到当前像素的下一个样本

                // 对所有样本的结果取平均值
                colObj /= (float)pixelSampler->samplesPerPixel;

                // 将最终计算出的平均颜色更新到帧缓冲区
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 0, (unsigned char)(colObj[0] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 1, (unsigned char)(colObj[1] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 2, (unsigned char)(colObj[2] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 3, 255);
            }
            // 加一个进度条
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
