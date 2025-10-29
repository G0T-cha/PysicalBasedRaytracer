#pragma once
#ifndef __ConstantTexture_h__
#define __ConstantTexture_h__

#include "Texture\Texture.h"

namespace PBR {

template <typename T>
class ConstantTexture : public Texture<T> {
  public:
    ConstantTexture(const T &value) : value(value) {}
    //·µ»Ø³£Á¿
    T Evaluate(const SurfaceInteraction &) const { return value; }

  private:
    T value;
};

}







#endif







