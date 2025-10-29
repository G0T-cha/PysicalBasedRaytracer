#include "Light\DiffuseLight.h"

namespace PBR
{
	DiffuseAreaLight::DiffuseAreaLight(const Transform& LightToWorld,
		const Spectrum& Lemit, int nSamples,
		const std::shared_ptr<Shape>& shape,
		bool twoSided)
		: AreaLight(LightToWorld, nSamples),
		Lemit(Lemit),
		shape(shape),
		twoSided(twoSided),
		area(shape->Area())
	{
		// Warn if light has transformation with non-uniform scale, though not
		// for Triangles, since this doesn't matter for them.
		// if (WorldToLight.HasScale() && dynamic_cast<const Triangle *>(shape.get()) == nullptr);
	}
	//（Φ = L * A * \pi)。
	Spectrum DiffuseAreaLight::Power() const
	{
		return (twoSided ? 2 : 1) * Lemit * area * Pi;
	}
	// 光源采样
	Spectrum DiffuseAreaLight::Sample_Li(const Interaction& ref, const Point2f& u,
		Vector3f* wi, float* pdf,
		VisibilityTester* vis) const
	{
		//按立体角进行形状采样
		Interaction pShape = shape->Sample(ref, u, pdf);
		if (*pdf == 0 || (pShape.p - ref.p).LengthSquared() == 0)
		{
			*pdf = 0;
			return 0.f;
		}
		*wi = Normalize(pShape.p - ref.p);
		*vis = VisibilityTester(ref, pShape);
		//返回自发光辐照度Radiance（不同于点光源强度衰减）
		return L(pShape, -*wi);
	}
	//委托给形状，计算光源采样 PDF
	float DiffuseAreaLight::Pdf_Li(const Interaction& ref,
		const Vector3f& wi) const
	{
		return shape->Pdf(ref, wi);
	}

	Spectrum DiffuseAreaLight::Sample_Le(const Point2f& u1, const Point2f& u2,
		float time, Ray* ray, Normal3f* nLight,
		float* pdfPos, float* pdfDir) const
	{
		/*
		// Sample a point on the area light's _Shape_, _pShape_
		Interaction pShape = shape->Sample(u1, pdfPos);
		pShape.mediumInterface = mediumInterface;
		*nLight = pShape.n;

		// Sample a cosine-weighted outgoing direction _w_ for area light
		Vector3f w;
		if (twoSided) {
			Point2f u = u2;
			// Choose a side to sample and then remap u[0] to [0,1] before
			// applying cosine-weighted hemisphere sampling for the chosen side.
			if (u[0] < .5) {
				u[0] = std::min(u[0] * 2, OneMinusEpsilon);
				w = CosineSampleHemisphere(u);
			}
			else {
				u[0] = std::min((u[0] - .5f) * 2, OneMinusEpsilon);
				w = CosineSampleHemisphere(u);
				w.z *= -1;
			}
			*pdfDir = 0.5f * CosineHemispherePdf(std::abs(w.z));
		}
		else {
			w = CosineSampleHemisphere(u2);
			*pdfDir = CosineHemispherePdf(w.z);
		}

		Vector3f v1, v2, n(pShape.n);
		CoordinateSystem(n, &v1, &v2);
		w = w.x * v1 + w.y * v2 + w.z * n;
		*ray = pShape.SpawnRay(w);
		return L(pShape, w);*/
		return Spectrum(0.f);
	}

	void DiffuseAreaLight::Pdf_Le(const Ray& ray, const Normal3f& n, float* pdfPos,
		float* pdfDir) const
	{
		/*
		Interaction it(ray.o, n, Vector3f(), Vector3f(n), ray.time,
			mediumInterface);
		*pdfPos = shape->Pdf(it);
		*pdfDir = twoSided ? (.5 * CosineHemispherePdf(AbsDot(n, ray.d)))
			: CosineHemispherePdf(Dot(n, ray.d));*/
	}

}
