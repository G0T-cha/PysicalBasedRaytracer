#pragma once
#ifndef __DiffuseLight_h__
#define __DiffuseLight_h__
#include "Light\Light.h"

namespace PBR {

	// ������
	class DiffuseAreaLight : public AreaLight {
	public:
		// Le�ǵ�λ�����λ����ǵ�����ǿ�ȣ�Radiance��
		// shapeΪ��״��twoSidedΪ�Ƿ����淢��
		DiffuseAreaLight(const Transform& LightToWorld, const Spectrum& Le,
			int nSamples, const std::shared_ptr<Shape>& shape,
			bool twoSided = false);
		// �Է�����նȣ����濴ΪLemit
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