#pragma once
#ifndef __DiffuseLight_h__
#define __DiffuseLight_h__
#include "Light\Light.h"

namespace PBR {

	// 漫反射
	class DiffuseAreaLight : public AreaLight {
	public:
		// Le是单位面积单位立体角的能量强度（Radiance）
		// shape为形状，twoSided为是否两面发光
		DiffuseAreaLight(const Transform& LightToWorld, const Spectrum& Le,
			int nSamples, const std::shared_ptr<Shape>& shape,
			bool twoSided = false);
		// 自发光辐照度：正面看为Lemit
		Spectrum L(const Interaction& intr, const Vector3f& w) const {
			return (twoSided || Dot(intr.n, w) > 0) ? Lemit : Spectrum(0.f);
		}
		Spectrum Power() const;
		Spectrum Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wo,
			float* pdf, VisibilityTester* vis) const;
		float Pdf_Li(const Interaction&, const Vector3f&) const;
		Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, float time,
			Ray* ray, Normal3f* nLight, float* pdfPos,
			float* pdfDir) const;
		void Pdf_Le(const Ray&, const Normal3f&, float* pdfPos,
			float* pdfDir) const;

	protected:
		const Spectrum Lemit;
		std::shared_ptr<Shape> shape;
		const bool twoSided;
		const float area;
	};

}


#endif