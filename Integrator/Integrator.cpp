#pragma once

#include "Integrator\Integrator.h"
#include "Sampler\Sampler.h"
#include "Core\Spectrum.h"
#include "Core\interaction.h"
#include "Core\Scene.h"
#include "Core\frameBuffer.h"
#include "Material\Reflection.h"
#include "Light\Light.h"
#include "Sampler/Sampling.h"
#include <omp.h>
#include <iomanip>

namespace PBR{

    Spectrum UniformSampleAllLights(const Interaction& it, const Scene& scene, Sampler& sampler,
        const std::vector<int>& nLightSamples, bool handleMedia) {
        Spectrum L(0.f);
        // 遍历每个光源
        for (size_t j = 0; j < scene.lights.size(); ++j) {
            const std::shared_ptr<Light>& light = scene.lights[j];
            int nSamples = nLightSamples[j];
            // 采样光源面积
            const Point2f* uLightArray = sampler.Get2DArray(nSamples);
            // 采样表面BSDF
            const Point2f* uScatteringArray = sampler.Get2DArray(nSamples);
            // 如果没有成功申请到数组，只使用一个随机样本
            if (!uLightArray || !uScatteringArray) {
                Point2f uLight = sampler.Get2D();
                Point2f uScattering = sampler.Get2D();
                L += EstimateDirect(it, uScattering, *light, uLight, scene, sampler, handleMedia);
            }
            // 采样nSample次
            else {
                Spectrum Ld(0.f);
                for (int k = 0; k < nSamples; ++k)
                    Ld += EstimateDirect(it, uScatteringArray[k], *light,
                        uLightArray[k], scene, sampler, handleMedia);
                L += Ld / nSamples;
            }
        }
        return L;
    }

    Spectrum UniformSampleOneLight(const Interaction& it, const Scene& scene, Sampler& sampler,
        bool handleMedia, const Distribution1D* lightDistrib) {
        // 随机选择一个灯光
        int nLights = int(scene.lights.size());
        if (nLights == 0) return Spectrum(0.f);
        int lightNum;
        float lightPdf;
        // 如果存在加权轮盘
        if (lightDistrib) {
            lightNum = lightDistrib->SampleDiscrete(sampler.Get1D(), &lightPdf);
            if (lightPdf == 0) return Spectrum(0.f);
        }
        // 否则，均匀随机
        else {
            lightNum = std::min((int)(sampler.Get1D() * nLights), nLights - 1);
            lightPdf = float(1) / nLights;
        }
        const std::shared_ptr<Light>& light = scene.lights[lightNum];
        Point2f uLight = sampler.Get2D();
        Point2f uScattering = sampler.Get2D();
        // 蒙特卡洛
        return EstimateDirect(it, uScattering, *light, uLight,
            scene, sampler, handleMedia) / lightPdf;
    }

    Spectrum EstimateDirect(const Interaction& it, const Point2f& uScattering, const Light& light,
        const Point2f& uLight, const Scene& scene, Sampler& sampler, bool handleMedia, bool specular) {
        BxDFType bsdfFlags =
            specular ? BSDF_ALL : BxDFType(BSDF_ALL & ~BSDF_SPECULAR);
        Spectrum Ld(0.f);
        Vector3f wi;
        float lightPdf = 0, scatteringPdf = 0;
        VisibilityTester visibility;


        // 策略1：使用uLight随机数进行光源采样
        // 返回对应光的亮度、光的方向wi、采样到该点的概率，和可见性测试对象
        Spectrum Li = light.Sample_Li(it, uLight, &wi, &lightPdf, &visibility);

        // 光源采样有效的话
        if (lightPdf > 0 && !Li.IsBlack()) {
            Spectrum f;
            if (it.IsSurfaceInteraction()) {
                const SurfaceInteraction& isect = (const SurfaceInteraction&)it;
                // 按光源采样计算 bsdf
                f = isect.bsdf->f(isect.wo, wi, bsdfFlags) *
                    AbsDot(wi, isect.shading.n);
                // BSDF采样（策略2）刚好选择wi方向的概率
                scatteringPdf = isect.bsdf->Pdf(isect.wo, wi, bsdfFlags);
            }
            else {
                // Evaluate phase function for light sampling strategy
                const MediumInteraction& mi = (const MediumInteraction&)it;
                float p = mi.phase->p(mi.wo, wi);
                f = Spectrum(p);
            }
            if (!f.IsBlack()) {
                if (handleMedia) {
                    Li *= visibility.Tr(scene, sampler);
                }
                else {
                    if (!visibility.Unoccluded(scene)) {
                        Li = Spectrum(0.f);
                    }
                }
                if (!Li.IsBlack()) {
                    // 点光源，直接蒙特卡洛计算光谱
                    if (IsDeltaLight(light.flags))
                        Ld += f * Li / lightPdf;
                    // 否则，使用幂启发法，计算策略1权重
                    else {
                        float weight = PowerHeuristic(1, lightPdf, 1, scatteringPdf);
                        // 把通过策略1得到的亮度*权重加入总亮度中
                        Ld += f * Li * weight / lightPdf;
                    }
                }
            }
        }

        // 采样 BSDF
        if (!IsDeltaLight(light.flags)) {
            Spectrum f;
            bool sampledSpecular = false;
            if (it.IsSurfaceInteraction()) {
                BxDFType sampledType;
                // 创建一个交点副本
                const SurfaceInteraction& isect = (const SurfaceInteraction&)it;
                // 在这个交点副本上做方法B：BSDF采样，得到一个全新的光照方向 wi，得到材质颜色 f
                f = isect.bsdf->Sample_f(isect.wo, &wi, uScattering, &scatteringPdf,
                    bsdfFlags, &sampledType);
                f *= AbsDot(wi, isect.shading.n);
                sampledSpecular = (sampledType & BSDF_SPECULAR) != 0;
            }
            else {           
                const MediumInteraction& mi = (const MediumInteraction&)it;
                float p = mi.phase->Sample_p(mi.wo, &wi, uScattering);
                f = Spectrum(p);
                scatteringPdf = p;
            }
            if (!f.IsBlack() && scatteringPdf > 0) {
                // 完美镜面，只能通过策略2被采样，所以权重为1
                float weight = 1;
                // 如果不是完美镜面，应用MIS
                if (!sampledSpecular) {
                    // 光源采样（策略1）刚好选择wi方向的概率
                    lightPdf = light.Pdf_Li(it, wi);
                    if (lightPdf == 0) return Ld;
                    // 计算策略2权重
                    weight = PowerHeuristic(1, scatteringPdf, 1, lightPdf);
                }
                // 沿着材质给的 wi 方向发射一条光线
                SurfaceInteraction lightIsect;
                Ray ray = it.SpawnRay(wi);
                bool foundSurfaceInteraction = scene.Intersect(ray, &lightIsect);

                Spectrum Li(0.f);
                // 若击中了场景，检查是否刚好是在计算的light，若是，Li为自发光
                if (foundSurfaceInteraction) {
                    if (lightIsect.primitive->GetAreaLight() == &light)
                        Li = lightIsect.Le(-wi);
                }
                // 如果没击中，返回无限远光源
                else
                    Li = light.Le(ray);

                // 将策略 2 的贡献以 MIS 权重加入总亮度
                if (!Li.IsBlack()) Ld += f * Li * weight / scatteringPdf;
            }
        }

        return Ld;
    }

    Spectrum SamplerIntegrator::SpecularReflect(
        const RayDifferential& ray, const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const
    {
        // 计算反射方向和颜色
        Vector3f wo = isect.wo, wi;
        float pdf;
        BxDFType type = BxDFType(BSDF_REFLECTION | BSDF_SPECULAR);
        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, type);
        const Normal3f& ns = isect.shading.n;

        // 反射有效
        if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f)
        {
            // 携带了微分
            RayDifferential rd = isect.SpawnRay(wi);
            if (ray.hasDifferentials)
            {
                // 新起点：击中点 p 加上 dpdx, dpdy
                rd.hasDifferentials = true;
                rd.rxOrigin = isect.p + isect.dpdx;
                rd.ryOrigin = isect.p + isect.dpdy;
                // 法线 n 是如何随着屏幕 x 和 y 变化的
                Normal3f dndx = isect.shading.dndu * isect.dudx +
                    isect.shading.dndv * isect.dvdx;
                Normal3f dndy = isect.shading.dndu * isect.dudy +
                    isect.shading.dndv * isect.dvdy;
                // 入射方向 wo 是如何随着屏幕 x 和 y 变化的
                Vector3f dwodx = -ray.rxDirection - wo,
                    dwody = -ray.ryDirection - wo;
                // 点积 是如何随着屏幕 x 和 y 变化的
                float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);
                // 偏导数代入完整的反射方程导数中
                rd.rxDirection =
                    wi - dwodx + 2.f * Vector3f(Dot(wo, ns) * dndx + dDNdx * ns);
                rd.ryDirection =
                    wi - dwody + 2.f * Vector3f(Dot(wo, ns) * dndy + dDNdy * ns);
            }
            return f * Li(rd, scene, sampler, depth + 1) * AbsDot(wi, ns) / pdf;
        }
        else
            return Spectrum(0.f);
    }

    Spectrum SamplerIntegrator::SpecularTransmit(
        const RayDifferential& ray, const SurfaceInteraction& isect,
        const Scene& scene, Sampler& sampler, int depth) const
    {
        // 计算折射方向
        Vector3f wo = isect.wo, wi;
        float pdf;
        const Point3f& p = isect.p;
        const BSDF& bsdf = *isect.bsdf;
        Spectrum f = bsdf.Sample_f(wo, &wi, sampler.Get2D(), &pdf,
            BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR));
        Spectrum L = Spectrum(0.f);
        Normal3f ns = isect.shading.n;
        // 折射有效
        if (pdf > 0.f && !f.IsBlack() && AbsDot(wi, ns) != 0.f)
        {           
            RayDifferential rd = isect.SpawnRay(wi);
            if (ray.hasDifferentials)
            {
                rd.hasDifferentials = true;
                rd.rxOrigin = p + isect.dpdx;
                rd.ryOrigin = p + isect.dpdy;
                Normal3f dndx = isect.shading.dndu * isect.dudx +
                    isect.shading.dndv * isect.dvdx;
                Normal3f dndy = isect.shading.dndu * isect.dudy +
                    isect.shading.dndv * isect.dvdy;
                // 是否需要反转
                float eta = 1 / bsdf.eta;
                if (Dot(wo, ns) < 0)
                {                   
                    eta = 1 / eta;
                    ns = -ns;
                    dndx = -dndx;
                    dndy = -dndy;
                }
                Vector3f dwodx = -ray.rxDirection - wo,
                    dwody = -ray.ryDirection - wo;
                float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
                float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);
                // 折射方程
                float mu = eta * Dot(wo, ns) - AbsDot(wi, ns);
                float dmudx =
                    (eta - (eta * eta * Dot(wo, ns)) / AbsDot(wi, ns)) * dDNdx;
                float dmudy =
                    (eta - (eta * eta * Dot(wo, ns)) / AbsDot(wi, ns)) * dDNdy;

                rd.rxDirection =
                    wi - eta * dwodx + Vector3f(mu * dndx + dmudx * ns);
                rd.ryDirection =
                    wi - eta * dwody + Vector3f(mu * dndy + dmudy * ns);
            }
            L = f * Li(rd, scene, sampler, depth + 1) * AbsDot(wi, ns) / pdf;
        }
        return L;
    }

	void SamplerIntegrator::Render(const Scene& scene, double& timeConsume) {
        //设置线程的个数
		omp_set_num_threads(4); 
        //获取起始时间 
		double start = omp_get_wtime();
        Preprocess(scene, *sampler);

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
                    RayDifferential r;
                    float rayWeight =
                        camera->GenerateRayDifferential(cameraSample, &r);
                    r.ScaleDifferentials(
                        1 / std::sqrt((float)pixelSampler->samplesPerPixel));

                    //调用子类实现的 Li() 计算光线颜色
                    colObj += Li(r, scene, *pixelSampler, 0);                   
                } while (pixelSampler->StartNextSample()); // 移动到当前像素的下一个样本

                // 对所有样本的结果取平均值
                colObj /= (float)pixelSampler->samplesPerPixel;

                /*colObj = PBR::RGBSpectrum::HDRtoLDR(colObj, 0.25f);

                float r = PBR::Clamp(colObj[0], 0.0f, 1.0f);
                float g = PBR::Clamp(colObj[1], 0.0f, 1.0f);
                float b = PBR::Clamp(colObj[2], 0.0f, 1.0f);


                // 将最终计算出的平均颜色更新到帧缓冲区
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 0, (unsigned char)(colObj[0] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 1, (unsigned char)(colObj[1] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 2, (unsigned char)(colObj[2] * 255.999f));
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 3, 255);*/
                float xyz[3];
                colObj.ToXYZ(xyz); // <-- 这将调用您的 RGBSpectrum::ToXYZ

                // 2. 将 XYZ 转换回 sRGB 色彩空间
                float rgb[3];
                XYZToRGB(xyz, rgb); // <-- 这将调用您的全局 XYZToRGB

                // 3. 执行 Gamma 矫正和 8-bit 转换
                //    (假设 PBR::Clamp 已在别处定义)
                unsigned char r_byte = (unsigned char)PBR::Clamp(255.f * GammaCorrect(rgb[0]) + 0.5f, 0.f, 255.f);
                unsigned char g_byte = (unsigned char)PBR::Clamp(255.f * GammaCorrect(rgb[1]) + 0.5f, 0.f, 255.f);
                unsigned char b_byte = (unsigned char)PBR::Clamp(255.f * GammaCorrect(rgb[2]) + 0.5f, 0.f, 255.f);

                // 4. 将最终计算出的平均颜色更新到帧缓冲区
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 0, r_byte);
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 1, g_byte);
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 2, b_byte);
                m_FrameBuffer->set_uc(i, pixelBounds.pMax.y - j - 1, 3, 255); // Alpha
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

    Spectrum SamplerIntegrator::Li(const RayDifferential& ray, const Scene& scene,
        Sampler& sampler, int depth) const {
        PBR::SurfaceInteraction isect;

        PBR::Spectrum colObj;
        if (scene.Intersect(ray, &isect)) {
            for (int count = 0; count < scene.lights.size(); count++) {
                VisibilityTester vist;
                Vector3f wi;
                Interaction p1;
                float pdf_light;
                Spectrum Li = scene.lights[count]->Sample_Li(isect, sampler.Get2D(), &wi, &pdf_light, &vist);

                if (vist.Unoccluded(scene)) {
                    //计算散射
                    isect.ComputeScatteringFunctions(ray);
                    // 对于漫反射材质来说，wo不会影响后面的结果
                    Vector3f wo = isect.wo;
                    Spectrum f = isect.bsdf->f(wo, wi);
                    float pdf_scattering = isect.bsdf->Pdf(wo, wi);
                    colObj += Li * pdf_scattering * f * 3.0f / pdf_light;
                }
            }
        }

        return colObj;
    }
};
