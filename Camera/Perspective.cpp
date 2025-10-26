#include "Camera\Perspective.h"
#include "Sampler\Sampling.h"

namespace PBR {
	PerspectiveCamera::PerspectiveCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
		const Bounds2f& screenWindow, float lensRadius, float focalDistance, float fov)
		: ProjectiveCamera(RasterWidth, RasterHeight, CameraToWorld, Perspective(fov, 1e-2f, 1000.f),
			screenWindow, lensRadius, focalDistance) {

		// Compute image plane bounds at $z=1$ for _PerspectiveCamera_
		Point2i res = Point2i(RasterWidth, RasterHeight);
		Point3f pMin = RasterToCamera(Point3f(0, 0, 0));
		Point3f pMax = RasterToCamera(Point3f(res.x, res.y, 0));
		pMin /= pMin.z;
		pMax /= pMax.z;
	}

	float PerspectiveCamera::GenerateRay(const CameraSample& sample,
		Ray* ray) const {
		// Compute raster and camera sample positions
		Point3f pFilm = Point3f(sample.pFilm.x, sample.pFilm.y, 0);
		Point3f pCamera = RasterToCamera(pFilm);
		*ray = Ray(Point3f(0, 0, 0), Normalize(Vector3f(pCamera)));
		// Modify ray for depth of field
		if (lensRadius > 0) {
			// Sample point on lens
			Point2f pLens = lensRadius * ConcentricSampleDisk(sample.pLens);

			// Compute point on plane of focus
			float ft = focalDistance / ray->d.z;
			Point3f pFocus = (*ray)(ft);

			// Update ray for effect of lens
			ray->o = Point3f(pLens.x, pLens.y, 0);
			ray->d = Normalize(pFocus - ray->o);
		}
		*ray = CameraToWorld(*ray);
		return 1;
	}

	PerspectiveCamera* CreatePerspectiveCamera(const int RasterWidth, const int RasterHeight, const Transform& cam2world) {
		float lensradius = 0.0f;
		float focaldistance = 0.0f;
		float frame = (float)RasterWidth / (float)RasterHeight;
		Bounds2f screen;
		if (frame > 1.f) {
			screen.pMin.x = -frame;
			screen.pMax.x = frame;
			screen.pMin.y = -1.f;
			screen.pMax.y = 1.f;
		}
		else {
			screen.pMin.x = -1.f;
			screen.pMax.x = 1.f;
			screen.pMin.y = -1.f / frame;
			screen.pMax.y = 1.f / frame;
		}
		float fov = 90.0f;
		float halffov = 45.0f;
		return new PerspectiveCamera(RasterWidth, RasterHeight, cam2world, screen, lensradius, focaldistance, fov);
	}
}