#pragma once
#include "Camera\Camera.h"


namespace PBR {
    class OrthographicCamera : public ProjectiveCamera {
    public:
        // ʹ��Orthographic����ͶӰ�任��Z_new = (z - zNear) / (zFar - zNear)��
        // �����ü�ƽ�� (Near Clip Plane) �� z = zNear ��ƽ�Ƶ�ԭ�� z = 0 ����
        // ����һ���õ��� [0, zFar - zNear] ��Χ��һ���� [0, 1] �ı�׼��Χ��
        OrthographicCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
            const Bounds2f& screenWindow, float lensRadius,
            float focalDistance)
            : ProjectiveCamera(RasterWidth, RasterHeight, CameraToWorld, Orthographic(0, 10), screenWindow, lensRadius, focalDistance) {
        }
        float GenerateRay(const CameraSample& sample, Ray*) const;
    };
}