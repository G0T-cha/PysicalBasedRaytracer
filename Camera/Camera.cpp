#include "Camera\Camera.h"

namespace PBR {
	float Camera::GenerateRayDifferential(const CameraSample& sample,
		RayDifferential* rd) const {
		// 主光线
		float wt = GenerateRay(sample, rd);
		if (wt == 0) return 0;

		// 计算 X 方向微分：rd->rxOrigin 和 rd->rxDirection
		float wtx;
		// 尝试在x方向偏移百分之五
		for (float eps : { .05, -.05 }) {
			CameraSample sshift = sample;
			sshift.pFilm.x += eps;
			Ray rx;
			// 采样一个偏移
			wtx = GenerateRay(sshift, &rx);
			// 一阶泰勒计算偏移光线
			rd->rxOrigin = rd->o + (rx.o - rd->o) / eps;
			rd->rxDirection = rd->d + (rx.d - rd->d) / eps;
			// 如果有效，就是一条辅助光线
			if (wtx != 0)
				break;
		}
		if (wtx == 0)
			return 0;

		// 计算 Y 方向微分
		float wty;
		for (float eps : { .05, -.05 }) {
			CameraSample sshift = sample;
			sshift.pFilm.y += eps;
			Ray ry;
			wty = GenerateRay(sshift, &ry);
			rd->ryOrigin = rd->o + (ry.o - rd->o) / eps;
			rd->ryDirection = rd->d + (ry.d - rd->d) / eps;
			if (wty != 0)
				break;
		}
		if (wty == 0)
			return 0;

		rd->hasDifferentials = true;
		return wt;
	}
}
