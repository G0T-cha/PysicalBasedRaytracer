#pragma once
#include "Camera\Camera.h"


namespace PBR {
    // OrthographicCamera Declarations
    class OrthographicCamera : public ProjectiveCamera {
    public:
        // OrthographicCamera Public Methods
        OrthographicCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
            const Bounds2f& screenWindow, float lensRadius,
            float focalDistance)
            : ProjectiveCamera(RasterWidth, RasterHeight, CameraToWorld, Orthographic(0, 10), screenWindow, lensRadius, focalDistance) {
        }
        float GenerateRay(const CameraSample& sample, Ray*) const;
    };
}