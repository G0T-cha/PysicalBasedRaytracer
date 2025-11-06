#pragma once
#ifndef __InfiniteAreaLight_H__
#define __InfiniteAreaLight_H__

#include "Texture\MIPMap.h"
#include "Light\Light.h"
#include "Core\Scene.h"


namespace PBR {
	class InfiniteAreaLight : public Light {
	public:
		InfiniteAreaLight(const Transform &LightToWorld, const Spectrum &power,
			int nSamples, const std::string &texmap);
		void Preprocess(const Scene &scene) {
			scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
		}
		Spectrum Power() const;
		Spectrum Le(const RayDifferential &ray) const;
		Spectrum Sample_Li(const Interaction &ref, const Point2f &u, Vector3f *wi,
			float *pdf, VisibilityTester *vis) const;
		float Pdf_Li(const Interaction &, const Vector3f &) const;
		Spectrum Sample_Le(const Point2f &u1, const Point2f &u2, float time,
			Ray *ray, Normal3f *nLight, float *pdfPos,
			float *pdfDir) const {
			return Spectrum(0.f);
		}
		void Pdf_Le(const Ray &, const Normal3f &, float *pdfPos,
			float *pdfDir) const {}

	private:
		// ÃÏø’∫–Ã˘Õº£®MIPMAP£©
		std::unique_ptr<MIPMap<RGBSpectrum>> Lmap;
		Point3f worldCenter;
		float worldRadius;
		// º”»®¬÷≈Ã
		std::unique_ptr<Distribution2D> distribution;
	};







}













#endif







