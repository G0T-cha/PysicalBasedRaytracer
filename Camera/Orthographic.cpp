#include "Camera\orthographic.h"
#include "Sampler\Sampling.h"


namespace PBR {

	float OrthographicCamera::GenerateRay(const CameraSample& sample,
		Ray* ray) const {
		
		Point3f pFilm = Point3f(sample.pFilm.x, sample.pFilm.y, 0);
		Point3f pCamera = RasterToCamera(pFilm);
		//光线不再全从原点出发
		*ray = Ray(pCamera, Vector3f(0, 0, 1));
		
		if (lensRadius > 0) {
			Point2f pLens = lensRadius * ConcentricSampleDisk(sample.pLens);
			float ft = focalDistance / ray->d.z;
			Point3f pFocus = (*ray)(ft);
			ray->o = Point3f(pLens.x, pLens.y, 0);
			ray->d = Normalize(pFocus - ray->o);
		}
		*ray = CameraToWorld(*ray);
		return 1;
	}

	OrthographicCamera* CreateOrthographicCamera(const int RasterWidth, const int RasterHeight, const Transform& cam2world) {

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

		float ScreenScale = 2.0f;

		{
			screen.pMin.x *= ScreenScale;
			screen.pMax.x *= ScreenScale;
			screen.pMin.y *= ScreenScale;
			screen.pMax.y *= ScreenScale;
		}




		float lensradius = 0.0f;
		float focaldistance = 0.0f;
		return new OrthographicCamera(RasterWidth, RasterHeight, cam2world, screen
			, lensradius, focaldistance);
	}




}













