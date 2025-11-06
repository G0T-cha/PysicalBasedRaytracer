#include "Media\GridDensityMedium.h"
#include "Sampler\Sampler.h"
#include "Core\interaction.h"

namespace PBR {

// GridDensityMedium Method Definitions
float GridDensityMedium::Density(const Point3f &p) const {
	// Compute voxel coordinates and offsets for _p_
	Point3f pSamples(p.x * nx - .5f, p.y * ny - .5f, p.z * nz - .5f);
	Point3i pi = (Point3i)Floor(pSamples);
	Vector3f d = pSamples - (Point3f)pi;

	// Trilinearly interpolate density values to compute local density
	float d00 = Lerp(d.x, D(pi), D(pi + Vector3i(1, 0, 0)));
	float d10 = Lerp(d.x, D(pi + Vector3i(0, 1, 0)), D(pi + Vector3i(1, 1, 0)));
	float d01 = Lerp(d.x, D(pi + Vector3i(0, 0, 1)), D(pi + Vector3i(1, 0, 1)));
	float d11 = Lerp(d.x, D(pi + Vector3i(0, 1, 1)), D(pi + Vector3i(1, 1, 1)));
	float d0 = Lerp(d.y, d00, d10);
	float d1 = Lerp(d.y, d01, d11);
	return Lerp(d.z, d0, d1);
}

Spectrum GridDensityMedium::Sample(const Ray &rWorld, Sampler &sampler, MediumInteraction *mi) const {
	Ray ray = WorldToMedium(
		Ray(rWorld.o, Normalize(rWorld.d), rWorld.tMax * rWorld.d.Length()));
	// Compute $[\tmin, \tmax]$ interval of _ray_'s overlap with medium bounds
	const Bounds3f b(Point3f(0, 0, 0), Point3f(1, 1, 1));
	float tMin, tMax;
	if (!b.IntersectP(ray, &tMin, &tMax)) return Spectrum(1.f);

	// Run delta-tracking iterations to sample a medium interaction
	float t = tMin;
	while (true) {
		t -= std::log(1 - sampler.Get1D()) * invMaxDensity / sigma_t;
		if (t >= tMax) break;
		if (Density(ray(t)) * invMaxDensity > sampler.Get1D()) {
			// Populate _mi_ with medium interaction information and return
			PhaseFunction *phase = new HenyeyGreenstein(g);
			*mi = MediumInteraction(rWorld(t), -rWorld.d, rWorld.time, this,
				phase);
			return sigma_s / sigma_t;
		}
	}
	return Spectrum(1.f);
}

Spectrum GridDensityMedium::Tr(const Ray &rWorld, Sampler &sampler) const {
	Ray ray = WorldToMedium(
		Ray(rWorld.o, Normalize(rWorld.d), rWorld.tMax * rWorld.d.Length()));
	// Compute $[\tmin, \tmax]$ interval of _ray_'s overlap with medium bounds
	const Bounds3f b(Point3f(0, 0, 0), Point3f(1, 1, 1));
	float tMin, tMax;
	if (!b.IntersectP(ray, &tMin, &tMax)) return Spectrum(1.f);

	// Perform ratio tracking to estimate the transmittance value
	float Tr = 1, t = tMin;

	while (true) {
		t -= std::log(1 - sampler.Get1D()) * invMaxDensity / sigma_t;
		if (t >= tMax) break;
		float density = Density(ray(t));
		Tr *= 1 - std::max((float)0, density * invMaxDensity);
		// Added after book publication: when transmittance gets low,
		// start applying Russian roulette to terminate sampling.
		const float rrThreshold = .1;
		if (Tr < rrThreshold) {
			float q = std::max((float).05, 1 - Tr);
			if (sampler.Get1D() < q) return 0;
			Tr /= 1 - q;
		}
	}
	return Spectrum(Tr);
}




}




