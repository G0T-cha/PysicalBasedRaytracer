#include "Shape\Triangle.h"
#include "Core\interaction.h"
#include "Sampler\Sampling.h"

namespace PBR {

	static long long nTris = 0;
	static long long nMeshes = 0;
	static long long nHits = 0;
	static long long nTests = 0;

	TriangleMesh::TriangleMesh(
		const Transform& ObjectToWorld, int nTriangles, const int* vertexIndices,
		int nVertices, const Point3f* P, const Vector3f* S, const Normal3f* N,
		const Point2f* UV, const int* fIndices)
		: nTriangles(nTriangles),
		nVertices(nVertices),
		vertexIndices(vertexIndices, vertexIndices + 3 * nTriangles) {
		++nMeshes;
		nTris += nTriangles;
		triMeshBytes += sizeof(*this) + this->vertexIndices.size() * sizeof(int) +
			nVertices * (sizeof(*P) + (N ? sizeof(*N) : 0) +
				(S ? sizeof(*S) : 0) + (UV ? sizeof(*UV) : 0) +
				(fIndices ? sizeof(*fIndices) : 0));

		// Transform mesh vertices to world space
		p.reset(new Point3f[nVertices]);
		for (int i = 0; i < nVertices; ++i) p[i] = ObjectToWorld(P[i]);
		// Copy _UV_, _N_, and _S_ vertex data, if present
		if (UV) {
			uv.reset(new Point2f[nVertices]);
			memcpy(uv.get(), UV, nVertices * sizeof(Point2f));
		}
		if (N) {
			n.reset(new Normal3f[nVertices]);
			for (int i = 0; i < nVertices; ++i) n[i] = ObjectToWorld(N[i]);
		}
		if (S) {
			s.reset(new Vector3f[nVertices]);
			for (int i = 0; i < nVertices; ++i) s[i] = ObjectToWorld(S[i]);
		}
		if (fIndices)
			faceIndices = std::vector<int>(fIndices, fIndices + nTriangles);
	}


	Bounds3f Triangle::ObjectBound() const {
		// Get triangle vertices in _p0_, _p1_, and _p2_
		const Point3f& p0 = mesh->p[v[0]];
		const Point3f& p1 = mesh->p[v[1]];
		const Point3f& p2 = mesh->p[v[2]];
		return Union(Bounds3f((*WorldToObject)(p0), (*WorldToObject)(p1)),
			(*WorldToObject)(p2));
	}
	Bounds3f Triangle::WorldBound() const {
		// Get triangle vertices in _p0_, _p1_, and _p2_
		const Point3f& p0 = mesh->p[v[0]];
		const Point3f& p1 = mesh->p[v[1]];
		const Point3f& p2 = mesh->p[v[2]];
		return Union(Bounds3f(p0, p1), p2);
	}
	bool Triangle::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect,
		bool testAlphaTexture) const {

		++nTests;
		// Get triangle vertices in _p0_, _p1_, and _p2_
		const Point3f& p0 = mesh->p[v[0]];
		const Point3f& p1 = mesh->p[v[1]];
		const Point3f& p2 = mesh->p[v[2]];

		// Perform ray--triangle intersection test

		// Transform triangle vertices to ray coordinate space

		// Translate vertices based on ray origin
		Point3f p0t = p0 - Vector3f(ray.o);
		Point3f p1t = p1 - Vector3f(ray.o);
		Point3f p2t = p2 - Vector3f(ray.o);
		// Permute components of triangle vertices and ray direction
		int kz = MaxDimension(Abs(ray.d));
		int kx = kz + 1;
		if (kx == 3) kx = 0;
		int ky = kx + 1;
		if (ky == 3) ky = 0;
		Vector3f d = Permute(ray.d, kx, ky, kz);
		p0t = Permute(p0t, kx, ky, kz);
		p1t = Permute(p1t, kx, ky, kz);
		p2t = Permute(p2t, kx, ky, kz);
		// Apply shear transformation to translated vertex positions
		float Sx = -d.x / d.z;
		float Sy = -d.y / d.z;
		float Sz = 1.f / d.z;
		p0t.x += Sx * p0t.z;
		p0t.y += Sy * p0t.z;
		p1t.x += Sx * p1t.z;
		p1t.y += Sy * p1t.z;
		p2t.x += Sx * p2t.z;
		p2t.y += Sy * p2t.z;
		// Compute edge function coefficients _e0_, _e1_, and _e2_
		float e0 = p1t.x * p2t.y - p1t.y * p2t.x;
		float e1 = p2t.x * p0t.y - p2t.y * p0t.x;
		float e2 = p0t.x * p1t.y - p0t.y * p1t.x;
		// Fall back to double precision test at triangle edges
		if (sizeof(float) == sizeof(float) &&
			(e0 == 0.0f || e1 == 0.0f || e2 == 0.0f)) {
			double p2txp1ty = (double)p2t.x * (double)p1t.y;
			double p2typ1tx = (double)p2t.y * (double)p1t.x;
			e0 = (float)(p2typ1tx - p2txp1ty);
			double p0txp2ty = (double)p0t.x * (double)p2t.y;
			double p0typ2tx = (double)p0t.y * (double)p2t.x;
			e1 = (float)(p0typ2tx - p0txp2ty);
			double p1txp0ty = (double)p1t.x * (double)p0t.y;
			double p1typ0tx = (double)p1t.y * (double)p0t.x;
			e2 = (float)(p1typ0tx - p1txp0ty);
		}
		// Perform triangle edge and determinant tests
		if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0))
			return false;
		float det = e0 + e1 + e2;
		if (det == 0) return false;
		// Compute scaled hit distance to triangle and test against ray $t$ range
		p0t.z *= Sz;
		p1t.z *= Sz;
		p2t.z *= Sz;
		float tScaled = e0 * p0t.z + e1 * p1t.z + e2 * p2t.z;
		if (det < 0 && (tScaled >= 0 || tScaled < ray.tMax * det))
			return false;
		else if (det > 0 && (tScaled <= 0 || tScaled > ray.tMax * det))
			return false;

		// Compute barycentric coordinates and $t$ value for triangle intersection
		float invDet = 1 / det;
		float b0 = e0 * invDet;
		float b1 = e1 * invDet;
		float b2 = e2 * invDet;
		float t = tScaled * invDet;

		// Ensure that computed triangle $t$ is conservatively greater than zero

		// Compute $\delta_z$ term for triangle $t$ error bounds
		float maxZt = MaxComponent(Abs(Vector3f(p0t.z, p1t.z, p2t.z)));
		float deltaZ = gamma(3) * maxZt;
		// Compute $\delta_x$ and $\delta_y$ terms for triangle $t$ error bounds
		float maxXt = MaxComponent(Abs(Vector3f(p0t.x, p1t.x, p2t.x)));
		float maxYt = MaxComponent(Abs(Vector3f(p0t.y, p1t.y, p2t.y)));
		float deltaX = gamma(5) * (maxXt + maxZt);
		float deltaY = gamma(5) * (maxYt + maxZt);
		// Compute $\delta_e$ term for triangle $t$ error bounds
		float deltaE =
			2 * (gamma(2) * maxXt * maxYt + deltaY * maxXt + deltaX * maxYt);
		// Compute $\delta_t$ term for triangle $t$ error bounds and check _t_
		float maxE = MaxComponent(Abs(Vector3f(e0, e1, e2)));
		float deltaT = 3 *
			(gamma(3) * maxE * maxZt + deltaE * maxZt + deltaZ * maxE) *
			std::abs(invDet);
		if (t <= deltaT) return false;

		// Compute triangle partial derivatives
		Vector3f dpdu, dpdv;
		Point2f uv[3];
		GetUVs(uv);

		// Compute deltas for triangle partial derivatives
		Vector2f duv02 = uv[0] - uv[2], duv12 = uv[1] - uv[2];
		Vector3f dp02 = p0 - p2, dp12 = p1 - p2;
		float determinant = duv02[0] * duv12[1] - duv02[1] * duv12[0];
		bool degenerateUV = std::abs(determinant) < 1e-8;
		if (!degenerateUV) {
			float invdet = 1 / determinant;
			dpdu = (duv12[1] * dp02 - duv02[1] * dp12) * invdet;
			dpdv = (-duv12[0] * dp02 + duv02[0] * dp12) * invdet;
		}

		// Compute error bounds for triangle intersection
		float xAbsSum =
			(std::abs(b0 * p0.x) + std::abs(b1 * p1.x) + std::abs(b2 * p2.x));
		float yAbsSum =
			(std::abs(b0 * p0.y) + std::abs(b1 * p1.y) + std::abs(b2 * p2.y));
		float zAbsSum =
			(std::abs(b0 * p0.z) + std::abs(b1 * p1.z) + std::abs(b2 * p2.z));
		Vector3f pError = gamma(7) * Vector3f(xAbsSum, yAbsSum, zAbsSum);

		Point2f uvHit = b0 * uv[0] + b1 * uv[1] + b2 * uv[2];
		Point3f pHit = b0 * p0 + b1 * p1 + b2 * p2;


		// Fill in _SurfaceInteraction_ from triangle hit
		*isect = SurfaceInteraction(pHit, pError, uvHit, -ray.d, dpdu, dpdv,
			Normal3f(0, 0, 0), Normal3f(0, 0, 0), ray.time,
			this, faceIndex);

		// Override surface normal in _isect_ for triangle
		isect->n = isect->shading.n = Normal3f(Normalize(Cross(dp02, dp12)));
		if (reverseOrientation ^ transformSwapsHandedness) 
			isect->n = isect->shading.n = -isect->n;




		*tHit = t;
		++nHits;
		return true;


	}

	bool Triangle::IntersectP(const Ray& ray, bool testAlphaTexture) const {
		++nTests;
		// Get triangle vertices in _p0_, _p1_, and _p2_
		const Point3f& p0 = mesh->p[v[0]];
		const Point3f& p1 = mesh->p[v[1]];
		const Point3f& p2 = mesh->p[v[2]];

		// Perform ray--triangle intersection test

		// Transform triangle vertices to ray coordinate space

		// Translate vertices based on ray origin
		Point3f p0t = p0 - Vector3f(ray.o);
		Point3f p1t = p1 - Vector3f(ray.o);
		Point3f p2t = p2 - Vector3f(ray.o);
		// Permute components of triangle vertices and ray direction
		int kz = MaxDimension(Abs(ray.d));
		int kx = kz + 1;
		if (kx == 3) kx = 0;
		int ky = kx + 1;
		if (ky == 3) ky = 0;
		Vector3f d = Permute(ray.d, kx, ky, kz);
		p0t = Permute(p0t, kx, ky, kz);
		p1t = Permute(p1t, kx, ky, kz);
		p2t = Permute(p2t, kx, ky, kz);
		// Apply shear transformation to translated vertex positions
		float Sx = -d.x / d.z;
		float Sy = -d.y / d.z;
		float Sz = 1.f / d.z;
		p0t.x += Sx * p0t.z;
		p0t.y += Sy * p0t.z;
		p1t.x += Sx * p1t.z;
		p1t.y += Sy * p1t.z;
		p2t.x += Sx * p2t.z;
		p2t.y += Sy * p2t.z;
		// Compute edge function coefficients _e0_, _e1_, and _e2_
		float e0 = p1t.x * p2t.y - p1t.y * p2t.x;
		float e1 = p2t.x * p0t.y - p2t.y * p0t.x;
		float e2 = p0t.x * p1t.y - p0t.y * p1t.x;
		// Fall back to double precision test at triangle edges
		if (sizeof(float) == sizeof(float) &&
			(e0 == 0.0f || e1 == 0.0f || e2 == 0.0f)) {
			double p2txp1ty = (double)p2t.x * (double)p1t.y;
			double p2typ1tx = (double)p2t.y * (double)p1t.x;
			e0 = (float)(p2typ1tx - p2txp1ty);
			double p0txp2ty = (double)p0t.x * (double)p2t.y;
			double p0typ2tx = (double)p0t.y * (double)p2t.x;
			e1 = (float)(p0typ2tx - p0txp2ty);
			double p1txp0ty = (double)p1t.x * (double)p0t.y;
			double p1typ0tx = (double)p1t.y * (double)p0t.x;
			e2 = (float)(p1typ0tx - p1txp0ty);
		}
		// Perform triangle edge and determinant tests
		if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0))
			return false;
		float det = e0 + e1 + e2;
		if (det == 0) return false;
		// Compute scaled hit distance to triangle and test against ray $t$ range
		p0t.z *= Sz;
		p1t.z *= Sz;
		p2t.z *= Sz;
		float tScaled = e0 * p0t.z + e1 * p1t.z + e2 * p2t.z;
		if (det < 0 && (tScaled >= 0 || tScaled < ray.tMax * det))
			return false;
		else if (det > 0 && (tScaled <= 0 || tScaled > ray.tMax * det))
			return false;

		// Compute barycentric coordinates and $t$ value for triangle intersection
		float invDet = 1 / det;
		float b0 = e0 * invDet;
		float b1 = e1 * invDet;
		float b2 = e2 * invDet;
		float t = tScaled * invDet;

		// Ensure that computed triangle $t$ is conservatively greater than zero

		// Compute $\delta_z$ term for triangle $t$ error bounds
		float maxZt = MaxComponent(Abs(Vector3f(p0t.z, p1t.z, p2t.z)));
		float deltaZ = gamma(3) * maxZt;
		// Compute $\delta_x$ and $\delta_y$ terms for triangle $t$ error bounds
		float maxXt = MaxComponent(Abs(Vector3f(p0t.x, p1t.x, p2t.x)));
		float maxYt = MaxComponent(Abs(Vector3f(p0t.y, p1t.y, p2t.y)));
		float deltaX = gamma(5) * (maxXt + maxZt);
		float deltaY = gamma(5) * (maxYt + maxZt);
		// Compute $\delta_e$ term for triangle $t$ error bounds
		float deltaE =
			2 * (gamma(2) * maxXt * maxYt + deltaY * maxXt + deltaX * maxYt);
		// Compute $\delta_t$ term for triangle $t$ error bounds and check _t_
		float maxE = MaxComponent(Abs(Vector3f(e0, e1, e2)));
		float deltaT = 3 *
			(gamma(3) * maxE * maxZt + deltaE * maxZt + deltaZ * maxE) *
			std::abs(invDet);
		if (t <= deltaT) return false;



		++nHits;
		return true;
	}

	float Triangle::Area() const {
		// Get triangle vertices in _p0_, _p1_, and _p2_
		const Point3f& p0 = mesh->p[v[0]];
		const Point3f& p1 = mesh->p[v[1]];
		const Point3f& p2 = mesh->p[v[2]];
		return 0.5 * Cross(p1 - p0, p2 - p0).Length();
	}

	Interaction Triangle::Sample(const Point2f& u, float* pdf) const {
		Point2f b = UniformSampleTriangle(u);
		// Get triangle vertices in _p0_, _p1_, and _p2_
		const Point3f& p0 = mesh->p[v[0]];
		const Point3f& p1 = mesh->p[v[1]];
		const Point3f& p2 = mesh->p[v[2]];
		Interaction it;
		it.p = b[0] * p0 + b[1] * p1 + (1 - b[0] - b[1]) * p2;
		// Compute surface normal for sampled point on triangle
		it.n = Normalize(Normal3f(Cross(p1 - p0, p2 - p0)));
		// Ensure correct orientation of the geometric normal; follow the same
		// approach as was used in Triangle::Intersect().
		if (mesh->n) {
			Normal3f ns(b[0] * mesh->n[v[0]] + b[1] * mesh->n[v[1]] +
				(1 - b[0] - b[1]) * mesh->n[v[2]]);
			it.n = Faceforward(it.n, ns);
		}
		//else if (reverseOrientation ^ transformSwapsHandedness)
		//	it.n *= -1;

		// Compute error bounds for sampled point on triangle
		Point3f pAbsSum =
			Abs(b[0] * p0) + Abs(b[1] * p1) + Abs((1 - b[0] - b[1]) * p2);
		it.pError = gamma(6) * Vector3f(pAbsSum.x, pAbsSum.y, pAbsSum.z);
		*pdf = 1 / Area();
		return it;
	}
}




