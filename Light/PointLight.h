#pragma once

#ifndef __PointLight_h__
#define __PointLight_h__
#include "Light\Light.h"


namespace PBR {
	//���Դ
	class PointLight : public Light {
	public:
		// ���Դ����0��0��0��ͨ���任ת��Ϊ��������
		PointLight(const Transform& LightToWorld, const Spectrum& I)
			: Light((int)LightFlags::DeltaPosition, LightToWorld),
			pLight(LightToWorld(Point3f(0, 0, 0))),
			I(I) {}
		// ִ�й�Դ����
		Spectrum Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi,
			float* pdf, VisibilityTester* vis) const;
		// ����ͬ�Ե��Դ���ܹ���Ϊǿ�ȳ��������
		Spectrum Power() const { return 4 * Pi * I; }
		// ��ѯ PDF, ����MIS ������ֻ�в�����һ������ܲ����������Ե��Դ����Ϊ0
		float Pdf_Li(const Interaction&, const Vector3f&) const { return 0; }
		// �Է������
		Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, float time,
			Ray* ray, Normal3f* nLight, float* pdfPos,
			float* pdfDir) const;
		// �Է����ѯ PDF
		void Pdf_Le(const Ray&, const Normal3f&, float* pdfPos,
			float* pdfDir) const;

	private:
		const Point3f pLight;
		const Spectrum I;
	};
}

#endif




