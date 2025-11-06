#pragma once
#ifndef __TEXTURE_H__
#define __TEXTURE_H__
#include "Core\Geometry.h"
#include "Core\PBR.h"

namespace PBR{

class TextureMapping2D {
public:
	virtual ~TextureMapping2D() {}
	virtual Point2f Map(const SurfaceInteraction& si, Vector2f* dstdx,
		Vector2f* dstdy) const = 0;
};

class UVMapping2D : public TextureMapping2D {
public:
	// 使用模型在 SurfaceInteraction 中提供的 uv 坐标
	// 并对其应用一个简单的缩放和偏移
	UVMapping2D(float su = 1, float sv = 1, float du = 0, float dv = 0)
		: su(su), sv(sv), du(du), dv(dv) {}
	Point2f Map(const SurfaceInteraction& si, Vector2f* dstdx,
		Vector2f* dstdy) const;

private:
	const float su, sv, du, dv;
};


template <typename T>
class Texture {
  public:
    // 返回光谱或浮点数等
    virtual T Evaluate(const SurfaceInteraction &) const = 0;
    virtual ~Texture() {}
};

float Lanczos(float, float tau = 2);




}






#endif







