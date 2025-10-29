#pragma once
#ifndef _MATERIAL_H__
#define _MATERIAL_H__

#include "Core/PBR.h"

namespace PBR {
enum class TransportMode { Radiance, Importance };
class Material {
  public:
    //º∆À„…¢…‰∫Ø ˝
    virtual void ComputeScatteringFunctions(SurfaceInteraction *si,
                                            TransportMode mode,
                                            bool allowMultipleLobes) const = 0;
    virtual ~Material() {}
};
}





#endif








