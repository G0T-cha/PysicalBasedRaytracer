#include "Core\Interaction.h"
#include "Core\Transform.h"
#include "Core\Primitive.h"
#include "Material\Reflection.h"
#include "Light\DiffuseLight.h"

namespace PBR {
	SurfaceInteraction::SurfaceInteraction(
		const Point3f& p, const Vector3f& pError, const Point2f& uv,
		const Vector3f& wo, const Vector3f& dpdu, const Vector3f& dpdv,
		const Normal3f& dndu, const Normal3f& dndv, float time, const Shape* shape,
		int faceIndex)
		: Interaction(p, Normal3f(Normalize(Cross(dpdu, dpdv))), pError, wo, time, nullptr),
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
		ComputeDifferentials(ray);
		primitive->ComputeScatteringFunctions(this, mode,
			allowMultipleLobes);
	}

	// 计算出 dudx, dvdx, dudy, dvdy 四个值
	// dudx：x 方向移动一个单位时， u 纹理坐标会变化的值
	void SurfaceInteraction::ComputeDifferentials(const RayDifferential& ray) const {
		if (ray.hasDifferentials) {
			// 计算dpdx，dpdy
			// 如果屏幕坐标 x 移动一个像素，3D 世界交点 p 会沿着这个向量 dpdx 移动
			float d = Dot(n, Vector3f(p.x, p.y, p.z));
			float tx =
				-(Dot(n, Vector3f(ray.rxOrigin)) - d) / Dot(n, ray.rxDirection);
			if (std::isinf(tx) || std::isnan(tx)) goto fail;
			Point3f px = ray.rxOrigin + tx * ray.rxDirection;
			float ty =
				-(Dot(n, Vector3f(ray.ryOrigin)) - d) / Dot(n, ray.ryDirection);
			if (std::isinf(ty) || std::isnan(ty)) goto fail;
			Point3f py = ray.ryOrigin + ty * ray.ryDirection;
			dpdx = px - p;
			dpdy = py - p;

			// 已知dpdx-3D位置随屏幕x的变化；dpdu-3D位置随纹理u的变化。dpdv
			// 链式法则 dpdx = (dpdu * dudx) + (dpdv * dvdx)
			int dim[2];
			if (std::abs(n.x) > std::abs(n.y) && std::abs(n.x) > std::abs(n.z)) {
				dim[0] = 1;
				dim[1] = 2;
			}
			else if (std::abs(n.y) > std::abs(n.z)) {
				dim[0] = 0;
				dim[1] = 2;
			}
			else {
				dim[0] = 0;
				dim[1] = 1;
			}

			float A[2][2] = { { dpdu[dim[0]], dpdv[dim[0]] },
			{ dpdu[dim[1]], dpdv[dim[1]] } };
			float Bx[2] = { px[dim[0]] - p[dim[0]], px[dim[1]] - p[dim[1]] };
			float By[2] = { py[dim[0]] - p[dim[0]], py[dim[1]] - p[dim[1]] };
			if (!SolveLinearSystem2x2(A, Bx, &dudx, &dvdx)) dudx = dvdx = 0;
			if (!SolveLinearSystem2x2(A, By, &dudy, &dvdy)) dudy = dvdy = 0;
			testd = true;
		}
		else {
		fail:
			dudx = dvdx = 0;
			dudy = dvdy = 0;
			dpdx = dpdy = Vector3f(0, 0, 0);
			testd = false;
		}
	}

	void SurfaceInteraction::SetShadingGeometry(const Vector3f& dpdus,
		const Vector3f& dpdvs,
		const Normal3f& dndus,
		const Normal3f& dndvs,
		bool orientationIsAuthoritative) {
		shading.n = Normalize((Normal3f)Cross(dpdus, dpdvs));
		if (orientationIsAuthoritative)
			n = Faceforward(n, shading.n);
		else
			shading.n = Faceforward(shading.n, n);

		shading.dpdu = dpdus;
		shading.dpdv = dpdvs;
		shading.dndu = dndus;
		shading.dndv = dndvs;
	}

	//判断是否需要累加自发光
	Spectrum SurfaceInteraction::Le(const Vector3f& w) const {
		const AreaLight* area = primitive->GetAreaLight();
		return area ? area->L(*this, w) : Spectrum(0.f);
	}

}
