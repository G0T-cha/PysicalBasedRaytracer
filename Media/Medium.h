#pragma once
#ifndef __Medium_h__
#define __Medium_h__

#include "Core\PBR.h"
#include "Core\Spectrum.h"

namespace PBR {

// 相位函数抽象基类
// 描述了光线在介质中的一个点上是如何散射的
class PhaseFunction {
public:
	virtual ~PhaseFunction() {}
	// 给定出射方向 wo 和入射方向 wi，返回光线从 wi 散射到 wo 的概率密度
	virtual float p(const Vector3f &wo, const Vector3f &wi) const = 0;
	// 采样,给定出射方向和随机数，生成一个新的入射方向并返回选择该方向的PDF
	virtual float Sample_p(const Vector3f &wo, Vector3f *wi,
		const Point2f &u) const = 0;
};

// 相位函数数学公式
// 不对称参数g:各向同性/前向散射/后向散射
inline float PhaseHG(float cosTheta, float g) {
	float denom = 1 + g * g + 2 * g * cosTheta;
	return Inv4Pi * (1 - g * g) / (denom * std::sqrt(denom));
}

// HG 相位函数实现
class HenyeyGreenstein : public PhaseFunction {
public:
	// 通过传入 g 参数来创建一个HG相位函数
	HenyeyGreenstein(float g) : g(g) {}
	float p(const Vector3f &wo, const Vector3f &wi) const;
	float Sample_p(const Vector3f &wo, Vector3f *wi,
		const Point2f &sample) const;
private:
	const float g;
};

// 参与介质抽象基类
class Medium {
public:
	virtual ~Medium() {}
	// 透射率，返回一个Spectrum
	virtual Spectrum Tr(const Ray &ray, Sampler &sampler) const = 0;
	// 采样介质：沿光线路径随机采样一个点，决定是否在介质内部发生了散射事件
	// 发生了散射，会填充 mi 结构体，包含散射点的位置、相位函数，并返回一个权重
	virtual Spectrum Sample(const Ray &ray, Sampler &sampler,
		MediumInteraction *mi) const = 0;
};

// 介质接口，管理一个表面两侧的介质信息
struct MediumInterface {
	MediumInterface() : inside(nullptr), outside(nullptr) {}
	MediumInterface(const Medium *medium) : inside(medium), outside(medium) {}
	MediumInterface(const Medium *inside, const Medium *outside)
		: inside(inside), outside(outside) {}
	bool IsMediumTransition() const { return inside != outside; }
	const Medium *inside, *outside; 
};



}



#endif








