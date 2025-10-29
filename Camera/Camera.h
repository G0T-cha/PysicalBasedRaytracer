#pragma once
#ifndef CAMERA_H
#define CAMERA_H

//#include "Sampler\TimeClockRandom.h"
#include "Core\PBR.h"
#include "Core\Geometry.h"
#include "Core\Transform.h"

namespace PBR {
	//���������
	struct CameraSample {
		//�����ڲ��Ĳ�����
		Point2f pFilm;
		//��Ȧ����ͷԲ�̵Ĳ�����
		Point2f pLens;
		//�˶�ģ��
		float time;
	};

	class Camera {
	public:
		Camera(const Transform& CameraToWorld) :CameraToWorld(CameraToWorld) {}
		virtual ~Camera() {}
		virtual float GenerateRay(const CameraSample& sample, Ray* ray) const { return 1; };

		// ����ռ䵽����ռ�
		Transform CameraToWorld;
	};

	class ProjectiveCamera : public Camera {
	public:
		ProjectiveCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
			const Transform& CameraToScreen,
			const Bounds2f& screenWindow, float lensr, float focald)
			: Camera(CameraToWorld),
			CameraToScreen(CameraToScreen) {
			// ����
			lensRadius = lensr;
			focalDistance = focald;
			// ����Ļ�ռ䣨��׼2Dƽ�������Ƭ��->��դ�ռ䣨�������꣩
			// �ڳ�ʼ����Ļ�ռ�ʱ���ᱣ֤��Ƭ�Ŀ�߱�������ͼ��Ŀ�߱�һ�£����ҽ���Ƭ�϶̵ı߳��ȹ̶�Ϊ 2.0���� -1.0 �� 1.0��
			//�� [pMin.x, pMax.x] x [pMin.y, pMax.y] ӳ�䵽 [0, RasterWidth] x [0, RasterHeight]
			ScreenToRaster =
				Scale(RasterWidth, RasterHeight, 1) *
				Scale(1 / (screenWindow.pMax.x - screenWindow.pMin.x),
					1 / (screenWindow.pMin.y - screenWindow.pMax.y), 1) *
				Translate(Vector3f(-screenWindow.pMin.x, -screenWindow.pMax.y, 0));
			//�����
			RasterToScreen = Inverse(ScreenToRaster);
			//��դ���������ϵ��3D��
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