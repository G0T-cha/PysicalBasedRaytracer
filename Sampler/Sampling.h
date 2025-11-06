#pragma once
#ifndef SAMPLING_H
#define SAMPLING_H_

#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\Transform.h"
#include "Sampler\RNG.h"

#include <vector>
#include <iostream>

namespace PBR {

	void StratifiedSample1D(float* samples, int nsamples, RNG& rng,
		bool jitter = true);

	void StratifiedSample2D(Point2f* samples, int nx, int ny, RNG& rng,
		bool jitter = true);

	void LatinHypercube(float* samples, int nSamples, int nDim, RNG& rng);

	Point2f RejectionSampleDisk(RNG& rng);

	Vector3f UniformSampleHemisphere(const Point2f& u);

	float UniformHemispherePdf();

	Vector3f UniformSampleSphere(const Point2f& u);

	float UniformSpherePdf();

	Vector3f UniformSampleCone(const Point2f& u, float thetamax);

	Vector3f UniformSampleCone(const Point2f& u, float thetamax, const Vector3f& x,
		const Vector3f& y, const Vector3f& z);

	float UniformConePdf(float thetamax);

	Point2f UniformSampleDisk(const Point2f& u);

	Point2f ConcentricSampleDisk(const Point2f& u);

	Point2f UniformSampleTriangle(const Point2f& u);

	// Sampling Inline Functions
	template <typename T>
	void Shuffle(T* samp, int count, int nDimensions, RNG& rng) {
		for (int i = 0; i < count; ++i) {
			int other = i + rng.UniformUInt32(count - i);
			for (int j = 0; j < nDimensions; ++j)
				std::swap(samp[nDimensions * i + j], samp[nDimensions * other + j]);
		}
	}
	//余弦加权半球采样-理想漫反射进行重要性采样
	//普通的 UniformSampleHemisphere 会浪费大量样本在 cos(theta) 接近 0 的方向上，这些方向对最终颜色的贡献很小。
	inline Vector3f CosineSampleHemisphere(const Point2f& u) {
		Point2f d = ConcentricSampleDisk(u);
		float z = std::sqrt(std::max((float)0, 1 - d.x * d.x - d.y * d.y));
		return Vector3f(d.x, d.y, z);
	}

	inline float CosineHemispherePdf(float cosTheta) { return cosTheta * InvPi; }

	//MIS多重重要性采样
	//平衡启发式
	inline float BalanceHeuristic(int nf, float fPdf, int ng, float gPdf) {
		return (nf * fPdf) / (nf * fPdf + ng * gPdf);
	}

	//幂启发式
	inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
		float f = nf * fPdf, g = ng * gPdf;
		return (f * f) / (f * f + g * g);
	}

	struct Distribution1D {
		// 接受包含n个权重的数组f，初始化func和cdf
		Distribution1D(const float* f, int n) : func(f, f + n), cdf(n + 1){
			cdf[0] = 0;
			// 宽度x高度
			for (int i = 1; i < n + 1; ++i) cdf[i] = cdf[i - 1] + func[i - 1] / n;
			funcInt = cdf[n];
			if (funcInt == 0) {
				for (int i = 1; i < n + 1; ++i) cdf[i] = float(i) / float(n);
			}
			else {
				for (int i = 1; i < n + 1; ++i) cdf[i] /= funcInt;
			}
		}

		// 返回区间数量
		int Count() const { return (int)func.size(); }

		// 根据随机数 u，离散采样
		int SampleDiscrete(float u, float* pdf = nullptr,
			float* uRemapped = nullptr) const {
			// 找出 u 所在的区间索引
			int offset = FindInterval((int)cdf.size(),
				[&](int index) { return cdf[index] <= u; });
			// 计算并返回选中这个离散区间的概率
			if (pdf) *pdf = (funcInt > 0) ? func[offset] / (funcInt * Count()) : 0;
			// 计算 u 在 [cdf[offset], cdf[offset+1]]的相对位置，用于 2D 采样
			if (uRemapped)
				*uRemapped = (u - cdf[offset]) / (cdf[offset + 1] - cdf[offset]);
			return offset;
		}


		// 选中第 i 个区间的概率是多少，和上面一致
		float DiscretePDF(int index) const {
			//CHECK(index >= 0 && index < Count());
			return func[index] / (funcInt * Count());
		}

		// 根据随机数 u，执行连续采样
		// 不仅返回选中的区间，还返回区间内的具体位置
		float SampleContinuous(float u, float* pdf, int* off = nullptr) const {
			int offset = FindInterval((int)cdf.size(),
				[&](int index) { return cdf[index] <= u; });
			if (off) *off = offset;
			float du = u - cdf[offset];
			if ((cdf[offset + 1] - cdf[offset]) > 0) {
				du /= (cdf[offset + 1] - cdf[offset]);
			}
			DCHECK(!std::isnan(du));

			if (pdf) *pdf = (funcInt > 0) ? func[offset] / funcInt : 0;

			return (offset + du) / Count();
		}



		// func 原始权重，func[i]代表第 i 个区间的权重（如能量）
		// cdf 累积概率密度，cdf[i] 存储的是前 i 个区间的累积概率。
		std::vector<float> func, cdf;
		// 权重总和
		float funcInt;
	};

	class Distribution2D {
	public:
		Distribution2D(const float* data, int nu, int nv);
		// 根据 2D 均匀随机数 u，返回一个2D坐标
		Point2f SampleContinuous(const Point2f& u, float* pdf) const {
			float pdfs[2];
			int v;
			// u[1]采样边缘分布
			float d1 = pMarginal->SampleContinuous(u[1], &pdfs[1], &v);
			// u[0]采样该行对应的条件分布
			float d0 = pConditionalV[v]->SampleContinuous(u[0], &pdfs[0]);
			*pdf = pdfs[0] * pdfs[1];
			return Point2f(d0, d1);
		}
		float Pdf(const Point2f& p) const {
			int iu = Clamp(int(p[0] * pConditionalV[0]->Count()), 0,
				pConditionalV[0]->Count() - 1);
			int iv =
				Clamp(int(p[1] * pMarginal->Count()), 0, pMarginal->Count() - 1);
			return pConditionalV[iv]->func[iu] / pMarginal->funcInt;
		}

	private:
		// 边缘分布：单一的 Distribution1D 对象
		// 选择行 (坐标V), 这里保存了每一行所有权重的总和
		std::unique_ptr<Distribution1D> pMarginal;
		// 条件分布：Distribution1D 对象的向量
		std::vector<std::unique_ptr<Distribution1D>> pConditionalV;
		
	};
}



#endif