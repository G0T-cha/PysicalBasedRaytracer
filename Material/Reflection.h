#pragma once
#ifndef __Reflection_h__
#define __Reflection_h__

#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\Spectrum.h"
#include "Core\Interaction.h"

#include "Material\Fresnel.h"
#include "Material\Microfacet.h"

#include <algorithm>
#include <string>


namespace PBR {

    // 本地空间三角函数
    inline float CosTheta(const Vector3f& w) { return w.z; }
    inline float Cos2Theta(const Vector3f& w) { return w.z * w.z; }
    inline float AbsCosTheta(const Vector3f& w) { return std::abs(w.z); }
    inline float Sin2Theta(const Vector3f& w) { return std::max((float)0, (float)1 - Cos2Theta(w)); }
    inline float SinTheta(const Vector3f& w) { return std::sqrt(Sin2Theta(w)); }
    inline float TanTheta(const Vector3f& w) { return SinTheta(w) / CosTheta(w); }
    inline float Tan2Theta(const Vector3f& w) { return Sin2Theta(w) / Cos2Theta(w); }
    inline float CosPhi(const Vector3f& w) {
        float sinTheta = SinTheta(w);
        return (sinTheta == 0) ? 1 : Clamp(w.x / sinTheta, -1, 1);
    }
    inline float SinPhi(const Vector3f& w) {
        float sinTheta = SinTheta(w);
        return (sinTheta == 0) ? 0 : Clamp(w.y / sinTheta, -1, 1);
    }
    inline float Cos2Phi(const Vector3f& w) { return CosPhi(w) * CosPhi(w); }
    inline float Sin2Phi(const Vector3f& w) { return SinPhi(w) * SinPhi(w); }
    inline float CosDPhi(const Vector3f& wa, const Vector3f& wb) {
        float waxy = wa.x * wa.x + wa.y * wa.y;
        float wbxy = wb.x * wb.x + wb.y * wb.y;
        if (waxy == 0 || wbxy == 0)
            return 1;
        return Clamp((wa.x * wb.x + wa.y * wb.y) / std::sqrt(waxy * wbxy), -1, 1);
    }
    // 反射公式：计算理想镜像反射向量
    inline Vector3f Reflect(const Vector3f& wo, const Vector3f& n) { return -wo + 2 * Dot(wo, n) * n; }
    // 斯涅尔定律：根据入射向量、法线、折射率，计算理想折射向量
    inline bool Refract(const Vector3f& wi, const Normal3f& n, float eta,
        Vector3f* wt) {
        float cosThetaI = Dot(n, wi);
        float sin2ThetaI = std::max(float(0), float(1 - cosThetaI * cosThetaI));
        float sin2ThetaT = eta * eta * sin2ThetaI;
        //全反射
        if (sin2ThetaT >= 1) return false;
        float cosThetaT = std::sqrt(1 - sin2ThetaT);
        *wt = eta * -wi + (eta * cosThetaI - cosThetaT) * Vector3f(n);
        return true;
    }

    // 检查两个向量是否在法线同侧
    inline bool SameHemisphere(const Vector3f& w, const Vector3f& wp) { return w.z * wp.z > 0; }
    inline bool SameHemisphere(const Vector3f& w, const Normal3f& wp) { return w.z * wp.z > 0; }

    // 位掩码，用于给积分器使用该枚举来查询 BSDF (双向散射分布函数)
    enum BxDFType {
        BSDF_REFLECTION = 1 << 0,
        BSDF_TRANSMISSION = 1 << 1,
        BSDF_DIFFUSE = 1 << 2,
        BSDF_GLOSSY = 1 << 3,
        BSDF_SPECULAR = 1 << 4,
        BSDF_ALL = BSDF_DIFFUSE | BSDF_GLOSSY | BSDF_SPECULAR | BSDF_REFLECTION |
        BSDF_TRANSMISSION,
    };

    // 散射基类BxDF，所有具体的散射行为都将继承它，如漫反射、镜面反射等
    class BxDF {
    public:
        virtual ~BxDF() {}
        // 标记
        BxDF(BxDFType type) : type(type) {}
        // 过滤器
        bool MatchesFlags(BxDFType t) const { return (type & t) == type; }
        // 给定出射方向和入射方向，散射的能量（光谱）
        virtual Spectrum f(const Vector3f& wo, const Vector3f& wi) const = 0;
        // 给定出射方向 wo（指向相机），和一个 2d 采样样本，根据物理特性，生成入射方向 wi
        virtual Spectrum Sample_f(const Vector3f& wo, Vector3f* wi,
            const Point2f& sample, float* pdf, BxDFType* sampledType = nullptr) const;
        // 定向-半球反射率（俄罗斯轮盘赌）
        // 光线从一个给定的出射方向 wo 观察，有多少能量是从整个半球的所有入射方向散射到 wo 方向的
        virtual Spectrum rho(const Vector3f& wo, int nSamples, const Point2f* samples) const;
        // 双半球反射率
        // 光线从所有可能的入射方向 wi 均匀射入，有多少能量被散射到了所有可能的出射方向 wo
        virtual Spectrum rho(int nSamples, const Point2f* samples1, const Point2f* samples2) const;
        // 对于积分器持有的 wo，wi，Sample_f 本应采样到wi方向的概率密度
        virtual float Pdf(const Vector3f& wo, const Vector3f& wi) const;

        // 成员：类型
        const BxDFType type;
    };

    //将所有BxDF组合在一起
    class BSDF {
    public:
        //建立着色坐标系，将shading.n（ns）视为z轴，切线shading.dpdu（ss）视为x轴，
        //ns, 副切线（ss）的叉乘作为y轴，另外存储几何法线ng，用于偏移和判断反射/投射
        BSDF(const SurfaceInteraction& si, float eta = 1)
            : eta(eta),
            ns(si.shading.n),
            ng(si.n),
            ss(Normalize(si.shading.dpdu)),
            ts(Cross(ns, ss)) {}
        //注册BxDF
        void Add(BxDF* b) {
            //CHECK_LT(nBxDFs, MaxBxDFs);
            //bxdfs[nBxDFs++] = b;
            bxdfs.push_back(std::shared_ptr<BxDF>(b));
            nBxDFs++;
        }
        int NumComponents(BxDFType flags = BSDF_ALL) const;
        //将世界空间向量v，转换为本地空间向量
        Vector3f WorldToLocal(const Vector3f& v) const {
            return Vector3f(Dot(v, ss), Dot(v, ts), Dot(v, ns));
        }
        //逆向量
        Vector3f LocalToWorld(const Vector3f& v) const {
            return Vector3f(ss.x * v.x + ts.x * v.y + ns.x * v.z,
                ss.y * v.x + ts.y * v.y + ns.y * v.z,
                ss.z * v.x + ts.z * v.y + ns.z * v.z);
        }
        Spectrum f(const Vector3f& woW, const Vector3f& wiW,
            BxDFType flags = BSDF_ALL) const;
        Spectrum rho(int nSamples, const Point2f* samples1, const Point2f* samples2,
            BxDFType flags = BSDF_ALL) const;
        Spectrum rho(const Vector3f& wo, int nSamples, const Point2f* samples,
            BxDFType flags = BSDF_ALL) const;
        Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
            float* pdf, BxDFType type = BSDF_ALL,
            BxDFType* sampledType = nullptr) const;
        float Pdf(const Vector3f& wo, const Vector3f& wi,
            BxDFType flags = BSDF_ALL) const;
        const float eta;
        ~BSDF();
    private:
        const Normal3f ns, ng;
        const Vector3f ss, ts;
        int nBxDFs = 0;
        static constexpr int MaxBxDFs = 8;
        std::vector<std::shared_ptr<BxDF>> bxdfs;
        //BxDF* bxdfs[MaxBxDFs];
    };

    //兰伯特反射（理想漫反射）
    class LambertianReflection : public BxDF {
    public:
        // 构造函数，把BxDFType设为反射、漫反射，同时接受一个光谱类型漫反射颜色
        LambertianReflection(const Spectrum& R)
            : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(R) {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
        // 两个反照率均为 R，反射比率固定
        Spectrum rho(const Vector3f&, int, const Point2f*) const { return R; }
        Spectrum rho(int, const Point2f*, const Point2f*) const { return R; }
    private:
        const Spectrum R;
    };

    // 奥伦纳亚尔模型（粗糙漫反射）
    // 在掠射角度或者当光源和观察者的方向比较接近时，会显得更亮
    class OrenNayar : public BxDF {
    public:
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
        OrenNayar(const Spectrum& R, float sigma)
            : BxDF(BxDFType(BSDF_REFLECTION | BSDF_DIFFUSE)), R(R) {
            sigma = Radians(sigma);
            float sigma2 = sigma * sigma;
            A = 1.f - (sigma2 / (2.f * (sigma2 + 0.33f)));
            B = 0.45f * sigma2 / (sigma2 + 0.09f);
        }
    private:
        const Spectrum R;
        float A, B;
    };


    inline int BSDF::NumComponents(BxDFType flags) const {
        int num = 0;
        for (int i = 0; i < nBxDFs; ++i)
            if (bxdfs[i]->MatchesFlags(flags)) ++num;
        return num;
    }

    // 光滑镜面
    class SpecularReflection : public BxDF {
    public:
        // 构造函数，把BxDFType设为反射、镜面反射，接受反射光谱R（镜面为111）
        // 同时接受一个菲涅尔对象指针，负责根据入射角判断多少比例光被反射
        SpecularReflection(const Spectrum& R, Fresnel* fresnel)
            : BxDF(BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)),
            R(R),
            fresnel(fresnel) {}
        ~SpecularReflection() { fresnel->~Fresnel(); }
        // 给定出射方向和入射方向，散射的能量（光谱）永远为0
        virtual Spectrum f(const Vector3f& wo, const Vector3f& wi) const {
            return Spectrum(0.f);
        }
        // 重写采样函数，用它生成采样光线
        virtual Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample,
            float* pdf, BxDFType* sampledType) const;
        // 概率密度同样为零
        float Pdf(const Vector3f& wo, const Vector3f& wi) const { return 0; }

    private:
        const Spectrum R;
        const Fresnel* fresnel;
    };

    // 清水/玻璃（完美透射）
    class SpecularTransmission : public BxDF {
    public:
        SpecularTransmission(const Spectrum& T, float etaA, float etaB,
            TransportMode mode)
            : BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_SPECULAR)),
            T(T),
            etaA(etaA),
            etaB(etaB),
            fresnel(etaA, etaB),
            mode(mode) {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const {
            return Spectrum(0.f);
        }
        Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample,
            float* pdf, BxDFType* sampledType) const;
        float Pdf(const Vector3f& wo, const Vector3f& wi) const { return 0; }

    private:
        const Spectrum T;
        const float etaA, etaB;
        const FresnelDielectric fresnel;

        const TransportMode mode;
    };

    // 完美玻璃（完美反射+完美透射）
    class FresnelSpecular : public BxDF {
    public:
        FresnelSpecular(const Spectrum& R, const Spectrum& T, float etaA, float etaB, TransportMode mode)
            : BxDF(BxDFType(BSDF_REFLECTION | BSDF_TRANSMISSION | BSDF_SPECULAR)),
            R(R),
            T(T),
            etaA(etaA),
            etaB(etaB),
            mode(mode) {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const {
            return Spectrum(0.f);
        }
        // 根据样本执行透射/反射
        Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
            float* pdf, BxDFType* sampledType) const;
        float Pdf(const Vector3f& wo, const Vector3f& wi) const { return 0; }

    private:
        const Spectrum R, T;
        const float etaA, etaB;
        const TransportMode mode;
    };

    // 金属（微表面反射）
    class MicrofacetReflection : public BxDF {
    public:
        //接受GGX模型，决定高光形状D和自阴影G
        MicrofacetReflection(const Spectrum& R,
            MicrofacetDistribution* distribution, Fresnel* fresnel)
            : BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
            R(R),
            distribution(distribution),
            fresnel(fresnel) {}
        ~MicrofacetReflection() {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
        Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
            float* pdf, BxDFType* sampledType) const;
        float Pdf(const Vector3f& wo, const Vector3f& wi) const;
    private:
        // MicrofacetReflection Private Data
        const Spectrum R;
        const std::shared_ptr<MicrofacetDistribution> distribution;
        const std::shared_ptr<Fresnel> fresnel;
    };

    // 磨砂玻璃（微表面透射）
    class MicrofacetTransmission : public BxDF {
    public:
        MicrofacetTransmission(const Spectrum& T,
            MicrofacetDistribution* distribution, float etaA,
            float etaB, TransportMode mode)
            : BxDF(BxDFType(BSDF_TRANSMISSION | BSDF_GLOSSY)),
            T(T),
            distribution(distribution),
            etaA(etaA),
            etaB(etaB),
            fresnel(etaA, etaB),
            mode(mode) {}
        ~MicrofacetTransmission() {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
        Spectrum Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
            float* pdf, BxDFType* sampledType) const;
        float Pdf(const Vector3f& wo, const Vector3f& wi) const;

    private:
        const Spectrum T;
        const std::shared_ptr<MicrofacetDistribution> distribution;
        const float etaA, etaB;
        // 电介质菲涅尔
        const FresnelDielectric fresnel;
        const TransportMode mode;
    };

    //塑料、清漆（哑光+光泽）
    class FresnelBlend : public BxDF {
    public:
        // GGX：清漆的粗糙度
        FresnelBlend(const Spectrum& Rd, const Spectrum& Rs,
            MicrofacetDistribution* distrib);
        ~FresnelBlend() {}
        Spectrum f(const Vector3f& wo, const Vector3f& wi) const;
        // 施利克近似：返回在特定角度下光线被反射的百分比
        Spectrum SchlickFresnel(float cosTheta) const {
            auto pow5 = [](float v) { return (v * v) * (v * v) * v; };
            return Rs + pow5(1 - cosTheta) * (Spectrum(1.) - Rs);
        }
        Spectrum Sample_f(const Vector3f& wi, Vector3f* sampled_f, const Point2f& u,
            float* pdf, BxDFType* sampledType) const;
        float Pdf(const Vector3f& wo, const Vector3f& wi) const;

    private:
        // 漫反射颜色、镜面颜色
        const Spectrum Rd, Rs;
        std::shared_ptr<MicrofacetDistribution> distribution;
    };
}

#endif