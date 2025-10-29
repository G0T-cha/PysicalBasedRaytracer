#pragma once
#ifndef __TEXTURE_H__
#define __TEXTURE_H__
#include "Core\Geometry.h"
#include "Core\PBR.h"

namespace PBR{


template <typename T>
class Texture {
  public:
    // 返回光谱或浮点数等
    virtual T Evaluate(const SurfaceInteraction &) const = 0;
    virtual ~Texture() {}
};




}






#endif







