#pragma once

#ifndef __PointLight_h__
#define __PointLight_h__
#include "Light\Light.h"


namespace PBR {
	//点光源
	class PointLight : public Light {
	public:
		// 点光源本地0，0，0，通过变换转换为世界坐标
		PointLight(const Transform& LightToWorld, const Spectrum& I)
			: Light((int)LightFlags::DeltaPosition, LightToWorld),
			pLight(LightToWorld(Point3f(0, 0, 0))),
			I(I) {}
		// 执行光源采样
		Spectrum Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi,
			float* pdf, VisibilityTester* vis) const;
		// 各向同性点光源，总功率为强度乘总立体角
		Spectrum Power() const { return 4 * Pi * I; }
		// 查询 PDF, 用于MIS ，由于只有采样到一个点才能采样到，所以点光源此项为0
		float Pdf_Li(const Interaction&, const Vector3f&) const { return 0; }
		// 自发光采样
		Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, float time,
			Ray* ray, Normal3f* nLight, float* pdfPos,
			float* pdfDir) const;
		// 自发光查询 PDF
		void Pdf_Le(const Ray&, const Normal3f&, float* pdfPos,
			float* pdfDir) const;

	private:
		const Point3f pLight;
		const Spectrum I;
	};
}

#endif




