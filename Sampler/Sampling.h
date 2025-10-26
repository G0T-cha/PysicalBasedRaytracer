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

	inline Vector3f CosineSampleHemisphere(const Point2f& u) {
		Point2f d = ConcentricSampleDisk(u);
		float z = std::sqrt(std::max((float)0, 1 - d.x * d.x - d.y * d.y));
		return Vector3f(d.x, d.y, z);
	}

	inline float CosineHemispherePdf(float cosTheta) { return cosTheta * InvPi; }

	inline float BalanceHeuristic(int nf, float fPdf, int ng, float gPdf) {
		return (nf * fPdf) / (nf * fPdf + ng * gPdf);
	}

	inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
		float f = nf * fPdf, g = ng * gPdf;
		return (f * f) / (f * f + g * g);
	}

}



#endif