#pragma once
#include "Camera\Camera.h"


namespace PBR {
    class OrthographicCamera : public ProjectiveCamera {
    public:
        // 使用Orthographic正交投影变换（Z_new = (z - zNear) / (zFar - zNear)）
        // 将近裁剪平面 (Near Clip Plane) 从 z = zNear 处平移到原点 z = 0 处。
        // 将上一步得到的 [0, zFar - zNear] 范围归一化到 [0, 1] 的标准范围。
        OrthographicCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
            const Bounds2f& screenWindow, float lensRadius,
            float focalDistance)
            : ProjectiveCamera(RasterWidth, RasterHeight, CameraToWorld, Orthographic(0, 10), screenWindow, lensRadius, focalDistance) {
        }
        float GenerateRay(const CameraSample& sample, Ray*) const;
    };
}