#include "Media\Medium.h"
#include "Core\Geometry.h"
#include "Core\Transform.h"


namespace PBR {

// 实现采样函数
float HenyeyGreenstein::Sample_p(const Vector3f &wo, Vector3f *wi,
	const Point2f &u) const {
	// Compute $\cos \theta$ for Henyey--Greenstein sample
	float cosTheta;
	if (std::abs(g) < 1e-3)
		cosTheta = 1 - 2 * u[0];
	else {
		float sqrTerm = (1 - g * g) / (1 + g - 2 * g * u[0]);
		cosTheta = -(1 + g * g - sqrTerm * sqrTerm) / (2 * g);
	}

	float sinTheta = std::sqrt(std::max((float)0, 1 - cosTheta * cosTheta));
	float phi = 2 * Pi * u[1];
	Vector3f v1, v2;
	CoordinateSystem(wo, &v1, &v2);
	*wi = SphericalDirection(sinTheta, cosTheta, phi, v1, v2, wo);
	return PhaseHG(cosTheta, g);
}

// 实现概率密度计算
float HenyeyGreenstein::p(const Vector3f &wo, const Vector3f &wi) const {
	return PhaseHG(Dot(wo, wi), g);
}


}











