#pragma once

#ifndef __PointLight_h__
#define __PointLight_h__
#include "Light\Light.h"


namespace PBR {
	// PointLight Declarations
	class PointLight : public Light {
	public:
		// PointLight Public Methods
		PointLight(const Transform& LightToWorld, const Spectrum& I)
			: Light((int)LightFlags::DeltaPosition, LightToWorld),
			pLight(LightToWorld(Point3f(0, 0, 0))),
			I(I) {}
		Spectrum Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi,
			float* pdf, VisibilityTester* vis) const;
		Spectrum Power() const { return 4 * Pi * I; }
		float Pdf_Li(const Interaction&, const Vector3f&) const { return 0; }
		Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, float time,
			Ray* ray, Normal3f* nLight, float* pdfPos,
			float* pdfDir) const;
		void Pdf_Le(const Ray&, const Normal3f&, float* pdfPos,
			float* pdfDir) const;

	private:
		// PointLight Private Data
		const Point3f pLight;
		const Spectrum I;
	};
}

#endif




