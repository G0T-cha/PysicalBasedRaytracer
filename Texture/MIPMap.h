#pragma once
#ifndef __MIPMap_h__
#define __MIPMap_h__

#include "Core\PBR.h"
#include "Core\Spectrum.h"
#include "Texture\Texture.h"
#include "Core\Memory.h"

#include <vector>
#include <string>

namespace PBR {

	// 椭圆滤波调用次数、三线性插值调用次数、MIPMAP内存
	static int64_t nEWALookups = 0;
	static int64_t nTrilerpLookups = 0;
	static int64_t mipMapMemory = 0;

// 纹理坐标超限的策略
enum class ImageWrap { Repeat, Black, Clamp };
// 缩放图像
struct ResampleWeight {
    int firstTexel;
    float weight[4];
};


template <typename T>
class MIPMap {
  public:
    // 构造MIPMAP
	  MIPMap(const Point2i &resolution, const T *data, bool doTri = false,
		  float maxAniso = 8.f, ImageWrap wrapMode = ImageWrap::Repeat);
    int Width() const { return resolution[0]; }
    int Height() const { return resolution[1]; }
	// 返回层数
    int Levels() const { return pyramid.size(); }
	// 从第level层，取出坐标 (s, t) 的像素值
	const T &Texel(int level, int s, int t) const;
	// 三线性插值查询：给定一个浮点坐标st和模糊宽度width，根据width插值合适的两层
	T Lookup(const Point2f &st, float width = 0.f) const;
	// 椭圆加权平均：dstdx 和 dstdy 定义了一个像素在纹理上覆盖的椭圆区域
	// 函数会计算这个椭圆区域内所有像素的加权平均颜色
	T Lookup(const Point2f &st, Vector2f dstdx, Vector2f dstdy) const;

  private:
    // 原始图像分辨率不是 2 的 N 次方时，高质量地将图像缩放到 2 的 N 次方
    std::unique_ptr<ResampleWeight[]> resampleWeights(int oldRes, int newRes) {
        std::unique_ptr<ResampleWeight[]> wt(new ResampleWeight[newRes]);
        float filterwidth = 2.f;
        for (int i = 0; i < newRes; ++i) {
            float center = (i + .5f) * oldRes / newRes;
            wt[i].firstTexel = std::floor((center - filterwidth) + 0.5f);
            for (int j = 0; j < 4; ++j) {
                float pos = wt[i].firstTexel + j + .5f;
                wt[i].weight[j] = Lanczos((pos - center) / filterwidth);
            }
            float invSumWts = 1 / (wt[i].weight[0] + wt[i].weight[1] +
                                   wt[i].weight[2] + wt[i].weight[3]);
            for (int j = 0; j < 4; ++j) wt[i].weight[j] *= invSumWts;
        }
        return wt;
    }
    float clamp(float v) { return Clamp(v, 0.f, Infinity); }
    Spectrum clamp(const Spectrum &v) { return v.Clamp(0.f, Infinity); }
	// 双线性插值：在金字塔的某一层内部，根据浮点坐标 st，对周围 4 个像素进行加权平均
    T triangle(int level, const Point2f &st) const;
	// 计算一个椭圆区域的加权平均
    T EWA(int level, Point2f st, Vector2f dst0, Vector2f dst1) const;


    const bool doTrilinear;
    const float maxAnisotropy;
    const ImageWrap wrapMode;
    Point2i resolution;
	// 核心：pyramid[0] 存储最高分辨率的图，pyramid[1] 存储 1/2 分辨率的图...
    std::vector<std::unique_ptr<BlockedArray<T>>> pyramid;
    static constexpr int WeightLUTSize = 128;
    static float weightLut[WeightLUTSize];
};


// MIPMap Method Definitions
template <typename T>
MIPMap<T>::MIPMap(const Point2i &res, const T *img, bool doTrilinear,
	float maxAnisotropy, ImageWrap wrapMode)
	: doTrilinear(doTrilinear),
	maxAnisotropy(maxAnisotropy),
	wrapMode(wrapMode),
	resolution(res) {
	
	// 如果不是 2 的 N 次方，重新采样到 2 的 N 次方
	// 先循环遍历所有像素，将图像横向缩放，再纵向缩放
	std::unique_ptr<T[]> resampledImage = nullptr;
	if (!IsPowerOf2(resolution[0]) || !IsPowerOf2(resolution[1])) {
		Point2i resPow2(RoundUpPow2(resolution[0]), RoundUpPow2(resolution[1]));
		std::unique_ptr<ResampleWeight[]> sWeights =
			resampleWeights(resolution[0], resPow2[0]);
		resampledImage.reset(new T[resPow2[0] * resPow2[1]]);

		for (int64_t t = 0; t < resolution[1]; ++t) {
			for (int s = 0; s < resPow2[0]; ++s) {
				resampledImage[t * resPow2[0] + s] = 0.f;
				for (int j = 0; j < 4; ++j) {
					int origS = sWeights[s].firstTexel + j;
					if (wrapMode == ImageWrap::Repeat)
						origS = Mod(origS, resolution[0]);
					else if (wrapMode == ImageWrap::Clamp)
						origS = Clamp(origS, 0, resolution[0] - 1);
					if (origS >= 0 && origS < (int)resolution[0])
						resampledImage[t * resPow2[0] + s] +=
						sWeights[s].weight[j] *
						img[t * resolution[0] + origS];
				}
			}
		}
		std::unique_ptr<ResampleWeight[]> tWeights =
			resampleWeights(resolution[1], resPow2[1]);
		std::vector<T *> resampleBufs;

		T *workData = new T[resPow2[1]];
		for (int64_t s = 0; s < resPow2[0]; ++s) {	
			for (int t = 0; t < resPow2[1]; ++t) {
				workData[t] = 0.f;
				for (int j = 0; j < 4; ++j) {
					int offset = tWeights[t].firstTexel + j;
					if (wrapMode == ImageWrap::Repeat)
						offset = Mod(offset, resolution[1]);
					else if (wrapMode == ImageWrap::Clamp)
						offset = Clamp(offset, 0, (int)resolution[1] - 1);
					if (offset >= 0 && offset < (int)resolution[1])
						workData[t] += tWeights[t].weight[j] *
						resampledImage[offset * resPow2[0] + s];
				}
			}
			for (int t = 0; t < resPow2[1]; ++t)
				resampledImage[t * resPow2[0] + s] = clamp(workData[t]);

		}
		delete[]workData;
		for (auto ptr : resampleBufs) delete[] ptr;
		resolution = resPow2;
	}
	// 计算总层数
	int nLevels = 1 + Log2Int(std::max(resolution[0], resolution[1]));
	pyramid.resize(nLevels);

	// 原始图像放入第 0 层
	pyramid[0].reset(
		new BlockedArray<T>(resolution[0], resolution[1],
			resampledImage ? resampledImage.get() : img));

	// 循环生成金字塔
	for (int i = 1; i < nLevels; ++i) {
		int sRes = std::max(1, pyramid[i - 1]->uSize() / 2);
		int tRes = std::max(1, pyramid[i - 1]->vSize() / 2);
		pyramid[i].reset(new BlockedArray<T>(sRes, tRes));

		// 取上一层（i-1）的“2x2”方块（4个像素），计算它们的平均值（* 0.25f）生成当前层（i）的一个像素
		for (int64_t t = 0; t < tRes; ++t) {
			for (int s = 0; s < sRes; ++s)
				(*pyramid[i])(s, t) =
				.25f * (Texel(i - 1, 2 * s, 2 * t) +
					Texel(i - 1, 2 * s + 1, 2 * t) +
					Texel(i - 1, 2 * s, 2 * t + 1) +
					Texel(i - 1, 2 * s + 1, 2 * t + 1));
		}
	}
	// 计算 EWA 滤波要用到的 exp() 权重
	if (weightLut[0] == 0.) {
		for (int i = 0; i < WeightLUTSize; ++i) {
			float alpha = 2;
			float r2 = float(i) / float(WeightLUTSize - 1);
			weightLut[i] = std::exp(-alpha * r2) - std::exp(-alpha);
		}
	}
	mipMapMemory += (4 * resolution[0] * resolution[1] * sizeof(T)) / 3;
}

// 取像素
template <typename T>
const T &MIPMap<T>::Texel(int level, int s, int t) const {
	// 拿到本层的数据
	const BlockedArray<T> &l = *pyramid[level];
	// 处理边界情况
	switch (wrapMode) {
	case ImageWrap::Repeat:
		s = Mod(s, l.uSize());
		t = Mod(t, l.vSize());
		break;
	case ImageWrap::Clamp:
		s = Clamp(s, 0, l.uSize() - 1);
		t = Clamp(t, 0, l.vSize() - 1);
		break;
	case ImageWrap::Black: {
		static const T black = 0.f;
		if (s < 0 || s >= (int)l.uSize() || t < 0 || t >= (int)l.vSize())
			return black;
		break;
	}
	}
	return l(s, t);
}

template <typename T>
T MIPMap<T>::Lookup(const Point2f &st, float width) const {
	++nTrilerpLookups;
	// 把 width 通过对数转换为金字塔层级
	float level = Levels() - 1 + Log2(std::max(width, (float)1e-8));
	// 如果 width 极其模糊，直接采样最高层
	if (level < 0)
		return triangle(0, st);
	// 如果 width 极其清晰，就直接用 Texel 采样最底层
	else if (level >= Levels() - 1)
		return Texel(Levels() - 1, 0, 0);
	// 否则在i层做一次双线性插值，i+1做一次双线性插值，再插值 —— 三线性插值
	else {
		int iLevel = std::floor(level);
		float delta = level - iLevel;
		return Lerp(delta, triangle(iLevel, st), triangle(iLevel + 1, st));
	}
}

// EWA 椭圆滤波
template <typename T>
T MIPMap<T>::Lookup(const Point2f &st, Vector2f dst0, Vector2f dst1) const {
	if (doTrilinear) {
		float width = std::max(std::max(std::abs(dst0[0]), std::abs(dst0[1])),
			std::max(std::abs(dst1[0]), std::abs(dst1[1])));
		return Lookup(st, width);
	}
	++nEWALookups;
	// Compute ellipse minor and major axes
	if (dst0.LengthSquared() < dst1.LengthSquared()) std::swap(dst0, dst1);
	float majorLength = dst0.Length();
	float minorLength = dst1.Length();

	// Clamp ellipse eccentricity if too large
	if (minorLength * maxAnisotropy < majorLength && minorLength > 0) {
		float scale = majorLength / (minorLength * maxAnisotropy);
		dst1 *= scale;
		minorLength *= scale;
	}
	if (minorLength == 0) return triangle(0, st);

	// Choose level of detail for EWA lookup and perform EWA filtering
	float lod = std::max((float)0, Levels() - (float)1 + Log2(minorLength));
	int ilod = std::floor(lod);
	return Lerp(lod - ilod, EWA(ilod, st, dst0, dst1),
		EWA(ilod + 1, st, dst0, dst1));
}

//  双线性插值
template <typename T>
T MIPMap<T>::triangle(int level, const Point2f &st) const {
	level = Clamp(level, 0, Levels() - 1);
	float s = st[0] * pyramid[level]->uSize() - 0.5f;
	float t = st[1] * pyramid[level]->vSize() - 0.5f;
	int s0 = std::floor(s), t0 = std::floor(t);
	float ds = s - s0, dt = t - t0;
	return (1 - ds) * (1 - dt) * Texel(level, s0, t0) +
		(1 - ds) * dt * Texel(level, s0, t0 + 1) +
		ds * (1 - dt) * Texel(level, s0 + 1, t0) +
		ds * dt * Texel(level, s0 + 1, t0 + 1);
}

// 椭圆内的加权平均
template <typename T>
T MIPMap<T>::EWA(int level, Point2f st, Vector2f dst0, Vector2f dst1) const {
	if (level >= Levels()) return Texel(Levels() - 1, 0, 0);
	// Convert EWA coordinates to appropriate scale for level
	st[0] = st[0] * pyramid[level]->uSize() - 0.5f;
	st[1] = st[1] * pyramid[level]->vSize() - 0.5f;
	dst0[0] *= pyramid[level]->uSize();
	dst0[1] *= pyramid[level]->vSize();
	dst1[0] *= pyramid[level]->uSize();
	dst1[1] *= pyramid[level]->vSize();

	// Compute ellipse coefficients to bound EWA filter region
	float A = dst0[1] * dst0[1] + dst1[1] * dst1[1] + 1;
	float B = -2 * (dst0[0] * dst0[1] + dst1[0] * dst1[1]);
	float C = dst0[0] * dst0[0] + dst1[0] * dst1[0] + 1;
	float invF = 1 / (A * C - B * B * 0.25f);
	A *= invF;
	B *= invF;
	C *= invF;

	// Compute the ellipse's $(s,t)$ bounding box in texture space
	float det = -B * B + 4 * A * C;
	float invDet = 1 / det;
	float uSqrt = std::sqrt(det * C), vSqrt = std::sqrt(A * det);
	int s0 = std::ceil(st[0] - 2 * invDet * uSqrt);
	int s1 = std::floor(st[0] + 2 * invDet * uSqrt);
	int t0 = std::ceil(st[1] - 2 * invDet * vSqrt);
	int t1 = std::floor(st[1] + 2 * invDet * vSqrt);

	// Scan over ellipse bound and compute quadratic equation
	T sum(0.f);
	float sumWts = 0;
	for (int it = t0; it <= t1; ++it) {
		float tt = it - st[1];
		for (int is = s0; is <= s1; ++is) {
			float ss = is - st[0];
			// Compute squared radius and filter texel if inside ellipse
			float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
			if (r2 < 1) {
				int index =
					std::min((int)(r2 * WeightLUTSize), WeightLUTSize - 1);
				float weight = weightLut[index];
				sum += Texel(level, is, it) * weight;
				sumWts += weight;
			}
		}
	}
	return sum / sumWts;
}


	template <typename T>
	float MIPMap<T>::weightLut[WeightLUTSize];


}










#endif






