#pragma once
#ifndef CAMERA_H
#define CAMERA_H

//#include "Sampler\TimeClockRandom.h"
#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\Transform.h"

namespace PBR {
	//相机采样器
	struct CameraSample {
		//像素内部的采样点
		Point2f pFilm;
		//光圈：镜头圆盘的采样点
		Point2f pLens;
		//运动模糊
		float time;
	};

	class Camera {
	public:
		Camera(const Transform& CameraToWorld, const Medium* medium = nullptr) :CameraToWorld(CameraToWorld), medium(medium) {}
		virtual ~Camera() {}
		virtual float GenerateRay(const CameraSample& sample, Ray* ray) const { return 1; };
		virtual float GenerateRayDifferential(const CameraSample& sample, RayDifferential* rd) const;
		// 相机空间到世界空间
		Transform CameraToWorld;
		const Medium* medium;
	};

	class ProjectiveCamera : public Camera {
	public:
		ProjectiveCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
			const Transform& CameraToScreen,
			const Bounds2f& screenWindow, float lensr, float focald, const Medium* medium)
			: Camera(CameraToWorld, medium),
			CameraToScreen(CameraToScreen) {
			// 景深
			lensRadius = lensr;
			focalDistance = focald;
			// 从屏幕空间（标准2D平面相机胶片）->光栅空间（像素坐标）
			// 在初始化屏幕空间时，会保证胶片的宽高比与最终图像的宽高比一致，并且将胶片较短的边长度固定为 2.0（从 -1.0 到 1.0）
			//将 [pMin.x, pMax.x] x [pMin.y, pMax.y] 映射到 [0, RasterWidth] x [0, RasterHeight]
			ScreenToRaster =
				Scale(RasterWidth, RasterHeight, 1) *
				Scale(1 / (screenWindow.pMax.x - screenWindow.pMin.x),
					1 / (screenWindow.pMin.y - screenWindow.pMax.y), 1) *
				Translate(Vector3f(-screenWindow.pMin.x, -screenWindow.pMax.y, 0));
			//逆矩阵
			RasterToScreen = Inverse(ScreenToRaster);
			//光栅到相机坐标系的3D点
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