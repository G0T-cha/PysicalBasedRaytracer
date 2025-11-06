#include "Texture\Texture.h"
#include "Core\interaction.h"



namespace PBR {

	Point2f UVMapping2D::Map(const SurfaceInteraction& si, Vector2f* dstdx,
		Vector2f* dstdy) const {
		// 计算纹理导数，并返回最终纹理坐标
		*dstdx = Vector2f(su * si.dudx, sv * si.dvdx);
		*dstdy = Vector2f(su * si.dudy, sv * si.dvdy);
		return Point2f(su * si.uv[0] + du, sv * si.uv[1] + dv);
	}

	float Lanczos(float x, float tau) {
		x = std::abs(x);
		if (x < 1e-5f) return 1;
		if (x > 1.f) return 0;
		x *= Pi;
		float s = std::sin(x * tau) / (x * tau);
		float lanczos = std::sin(x) / x;
		return s * lanczos;
	}
}



