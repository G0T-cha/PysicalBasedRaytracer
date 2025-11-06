#pragma once
#ifndef PERSPECTIVE_H
#define PERSPECTIVE_H

#include "Camera\Camera.h"


namespace PBR {
    class PerspectiveCamera : public ProjectiveCamera {
    public:
        // PerspectiveCamera Public Methods
        PerspectiveCamera(const int RasterWidth, const int RasterHeight, const Transform& CameraToWorld,
            const Bounds2f& screenWindow, float lensRadius, float focalDistance,
            float fov, const Medium* medium);
        float GenerateRay(const CameraSample& sample, Ray*) const;
        virtual float GenerateRayDifferential(const CameraSample& sample,
            RayDifferential* rd) const;
    private:
        Vector3f dxCamera, dyCamera;
    };

    PerspectiveCamera* CreatePerspectiveCamera(const int RasterWidth, const int RasterHeight, const Transform& cam2world, Medium* media);
}

#endif



