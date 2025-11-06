#include "Sampler\Sampling.h"


namespace PBR {
	// 分层采样
	void StratifiedSample1D(float* samp, int nSamples, RNG& rng, bool jitter) {
		float invNSamples = (float)1 / nSamples;
		for (int i = 0; i < nSamples; ++i) {
			float delta = jitter ? rng.UniformFloat() : 0.5f;
			samp[i] = std::min((i + delta) * invNSamples, OneMinusEpsilon);
		}
	}
	
	void StratifiedSample2D(Point2f* samp, int nx, int ny, RNG& rng, bool jitter) {
		float dx = (float)1 / nx, dy = (float)1 / ny;
		for (int y = 0; y < ny; ++y)
			for (int x = 0; x < nx; ++x) {
				float jx = jitter ? rng.UniformFloat() : 0.5f;
				float jy = jitter ? rng.UniformFloat() : 0.5f;
				samp->x = std::min((x + jx) * dx, OneMinusEpsilon);
				samp->y = std::min((y + jy) * dy, OneMinusEpsilon);
				++samp;
			}
	}
	// 拉丁超立方采样
	void LatinHypercube(float* samples, int nSamples, int nDim, RNG& rng) {
		float invNSamples = (float)1 / nSamples;
		for (int i = 0; i < nSamples; ++i)
			for (int j = 0; j < nDim; ++j) {
				float sj = (i + (rng.UniformFloat())) * invNSamples;
				samples[nDim * i + j] = std::min(sj, OneMinusEpsilon);
			}
		for (int i = 0; i < nDim; ++i) {
			for (int j = 0; j < nSamples; ++j) {
				int other = j + rng.UniformUInt32(nSamples - j);
				std::swap(samples[nDim * j + i], samples[nDim * other + i]);
			}
		}
	}
	//圆盘：拒绝采样-效率低
	Point2f RejectionSampleDisk(RNG& rng) {
		Point2f p;
		do {
			p.x = 1 - 2 * rng.UniformFloat();
			p.y = 1 - 2 * rng.UniformFloat();
		} while (p.x * p.x + p.y * p.y > 1);
		return p;
	}
	//均匀半球（立体角
	Vector3f UniformSampleHemisphere(const Point2f& u) {
		float z = u[0];
		float r = std::sqrt(std::max((float)0, (float)1. - z * z));
		float phi = 2 * Pi * u[1];
		return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
	}

	float UniformHemispherePdf() { return Inv2Pi; }
	//均匀球体
	Vector3f UniformSampleSphere(const Point2f& u) {
		float z = 1 - 2 * u[0];
		float r = std::sqrt(std::max((float)0, (float)1 - z * z));
		float phi = 2 * Pi * u[1];
		return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
	}

	float UniformSpherePdf() { return Inv4Pi; }
	//圆盘：均匀采样
	Point2f UniformSampleDisk(const Point2f& u) {
		float r = std::sqrt(u[0]);
		float theta = 2 * Pi * u[1];
		return Point2f(r * std::cos(theta), r * std::sin(theta));
	}
	//同心圆盘映射
	Point2f ConcentricSampleDisk(const Point2f& u) {
		// Map uniform random numbers to $[-1,1]^2$
		Point2f uOffset = 2.f * u - Vector2f(1, 1);

		// Handle degeneracy at the origin
		if (uOffset.x == 0 && uOffset.y == 0) return Point2f(0, 0);

		// Apply concentric mapping to point
		float theta, r;
		if (std::abs(uOffset.x) > std::abs(uOffset.y)) {
			r = uOffset.x;
			theta = PiOver4 * (uOffset.y / uOffset.x);
		}
		else {
			r = uOffset.y;
			theta = PiOver2 - PiOver4 * (uOffset.x / uOffset.y);
		}
		return r * Point2f(std::cos(theta), std::sin(theta));
	}

	float UniformConePdf(float cosThetaMax) {
		return 1 / (2 * Pi * (1 - cosThetaMax));
	}
	//圆锥体
	Vector3f UniformSampleCone(const Point2f& u, float cosThetaMax) {
		float cosTheta = ((float)1 - u[0]) + u[0] * cosThetaMax;
		float sinTheta = std::sqrt((float)1 - cosTheta * cosTheta);
		float phi = u[1] * 2 * Pi;
		return Vector3f(std::cos(phi) * sinTheta, std::sin(phi) * sinTheta,
			cosTheta);
	}

	Vector3f UniformSampleCone(const Point2f& u, float cosThetaMax,
		const Vector3f& x, const Vector3f& y,
		const Vector3f& z) {
		float cosTheta = Lerp(u[0], cosThetaMax, 1.f);
		float sinTheta = std::sqrt((float)1. - cosTheta * cosTheta);
		float phi = u[1] * 2 * Pi;
		return std::cos(phi) * sinTheta * x + std::sin(phi) * sinTheta * y +
			cosTheta * z;
	}
	//三角形采样
	Point2f UniformSampleTriangle(const Point2f& u) {
		float su0 = std::sqrt(u[0]);
		return Point2f(1 - su0, u[1] * su0);
	}

	Distribution2D::Distribution2D(const float* func, int nu, int nv) {
		pConditionalV.reserve(nv);
		for (int v = 0; v < nv; ++v) {
			// 构造条件分布
			pConditionalV.emplace_back(new Distribution1D(&func[v * nu], nu));
		}
		// 构建边缘分布
		std::vector<float> marginalFunc;
		marginalFunc.reserve(nv);
		for (int v = 0; v < nv; ++v)
			marginalFunc.push_back(pConditionalV[v]->funcInt);
		pMarginal.reset(new Distribution1D(&marginalFunc[0], nv));
	}







}