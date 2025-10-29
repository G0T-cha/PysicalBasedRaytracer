#include "Core\Interaction.h"
#include "Core\Primitive.h"
#include "Material\Reflection.h"
#include "Light\DiffuseLight.h"

namespace PBR {
	SurfaceInteraction::SurfaceInteraction(
		const Point3f& p, const Vector3f& pError, const Point2f& uv,
		const Vector3f& wo, const Vector3f& dpdu, const Vector3f& dpdv,
		const Normal3f& dndu, const Normal3f& dndv, float time, const Shape* shape,
		int faceIndex)
		: Interaction(p, Normal3f(Normalize(Cross(dpdu, dpdv))), pError, wo, time),
		uv(uv),
		dpdu(dpdu),
		dpdv(dpdv),
		dndu(dndu),
		dndv(dndv),
		shape(shape) {
		shading.n = n;
		shading.dpdu = dpdu;
		shading.dpdv = dpdv;
		shading.dndu = dndu;
		shading.dndv = dndv;

		// 判断是否需要反转法向量
		if (shape &&
		(shape->reverseOrientation ^ shape->transformSwapsHandedness)) {
		n *= -1;
		shading.n *= -1;
		}
	}

	SurfaceInteraction::~SurfaceInteraction() {
		if (bsdf)
			bsdf->~BSDF();
	}
	//计算散射
	void SurfaceInteraction::ComputeScatteringFunctions(const Ray& ray,
		bool allowMultipleLobes,
		TransportMode mode) {
		primitive->ComputeScatteringFunctions(this, mode,
			allowMultipleLobes);
	}
	//判断是否需要累加自发光
	Spectrum SurfaceInteraction::Le(const Vector3f& w) const {
		const AreaLight* area = primitive->GetAreaLight();
		return area ? area->L(*this, w) : Spectrum(0.f);
	}

}
