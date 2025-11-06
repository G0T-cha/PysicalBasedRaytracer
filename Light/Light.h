#pragma once
#ifndef __Light_h__
#define __Light_h__

#include "Core\PBR.h"
#include "Core\Transform.h"
#include "Core\Interaction.h"
#include "Core\Spectrum.h"

namespace PBR{

// 封装两点可见性测试所需的信息
class VisibilityTester {
public:
	VisibilityTester() {}
	VisibilityTester(const Interaction& p0, const Interaction& p1)
		: p0(p0), p1(p1) {}
	const Interaction& P0() const { return p0; }
	const Interaction& P1() const { return p1; }
    //核心方法
	bool Unoccluded(const Scene& scene) const;
    //计算透射率，用于体积渲染
	Spectrum Tr(const Scene& scene, Sampler& sampler) const; 

private:
	Interaction p0, p1;
};

// 位掩码：点光源、理想方向光、面光源、无限光源（天空盒、环境光等）
enum class LightFlags : int {
    DeltaPosition = 1,
    DeltaDirection = 2,
    Area = 4,
    Infinite = 8
};

// Delta光源测试，不能被BSDF碰巧击中，需使用光源采样
inline bool IsDeltaLight(int flags) {
    return flags & (int)LightFlags::DeltaPosition ||
        flags & (int)LightFlags::DeltaDirection;
}

class Light {
public:
    virtual ~Light() {}
    // 光源类型、光源空间到世界空间、采样数
    Light(int flags, const Transform& LightToWorld, const MediumInterface& mediumInterface, int nSamples = 1);
    // 执行光源采样
    // 给定被着色点ref和2d采样样本u，生成入射光方向wi、立体角pdf和可见性测试器vis，返回光谱：该方向的入射辐照度Li
    virtual Spectrum Sample_Li(const Interaction& ref, const Point2f& u,
        Vector3f* wi, float* pdf,
        VisibilityTester* vis) const = 0;
    // 总功率，路径追踪时需要根据场景中所有光源的功率，按比例随机选择一个光源进行采样，优先采样能量更强的光源。
    virtual Spectrum Power() const = 0;
    // 可选预处理
    virtual void Preprocess(const Scene& scene) {}
    // 查询自发光，光线未击中任何物体或者击中了光源，返回自发光
    virtual Spectrum Le(const RayDifferential& r) const { return Spectrum(0.8f); }
    // 查询光源采样PDF，用于MIS
    // 给定 ref 和 wi，返回 Sample_Li 本应采样到 wi 的 pdf
    virtual float Pdf_Li(const Interaction& ref, const Vector3f& wi) const = 0;
    // 采样自发光
    // 从光源自身出发，随机采样一条出射光线，输出出射光线、起点法线、选择起点概率（面积）、选择方向概率（立体角）
    // 双向路径追踪、光子映射需要
    virtual Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, float time,
        Ray* ray, Normal3f* nLight, float* pdfPos,
        float* pdfDir) const = 0;
    // 查询自发光 PDF
    // 给定一条从光源发出的光线和起点法线，返回Sample_Le本应生成这条光线的概率（位置和方向）
    virtual void Pdf_Le(const Ray& ray, const Normal3f& nLight, float* pdfPos,
        float* pdfDir) const = 0;


    const int flags;
    // 采样数：对光源采样时需要，如直接光照积分器
    const int nSamples;
    const MediumInterface mediumInterface;
protected:
    const Transform LightToWorld, WorldToLight;
};

// 子类：有表面积的光源
class AreaLight : public Light {
public:
    AreaLight(const Transform& LightToWorld, const MediumInterface& medium, int nSamples);
    // 查询面光源辐照度，返回光谱
    virtual Spectrum L(const Interaction& intr, const Vector3f& w) const = 0;
};

}


#endif