#pragma once
#ifndef CAMERA_H
#define CAMERA_H

//#include "Sampler\TimeClockRandom.h"
#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\Transform.h"

namespace PBR {
	struct CameraSample {
		Point2f pFilm;
		Point2f pLens;
		float time;
	};

	class Camera {
	public:
		// Camera Interface
		Camera(const Transform& CameraToWorld) :CameraToWorld(CameraToWorld) {}
		virtual ~Camera() {}
		virtual float GenerateRay(const CameraSample& sample, Ray* ray) const { return 1; };

		// Camera Public Data
		Transform CameraToWorld;
	};

	class ProjectiveCamera : public Camera {
	public:
		// ProjectiveCamera Public Methods
		ProjectiveCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
			const Transform& CameraToScreen,
			const Bounds2f& screenWindow, float lensr, float focald)
			: Camera(CameraToWorld),
			CameraToScreen(CameraToScreen) {
			// Initialize depth of field parameters
			lensRadius = lensr;
			focalDistance = focald;
			// Compute projective camera screen transformations
			ScreenToRaster =
				Scale(RasterWidth, RasterHeight, 1) *
				Scale(1 / (screenWindow.pMax.x - screenWindow.pMin.x),
					1 / (screenWindow.pMin.y - screenWindow.pMax.y), 1) *
				Translate(Vector3f(-screenWindow.pMin.x, -screenWindow.pMax.y, 0));
			RasterToScreen = Inverse(ScreenToRaster);
			RasterToCamera = Inverse(CameraToScreen) * RasterToScreen;
		}

	protected:
		// ProjectiveCamera Protected Data
		Transform CameraToScreen, RasterToCamera;
		Transform ScreenToRaster, RasterToScreen;
		float lensRadius, focalDistance;
	};
}

#endif