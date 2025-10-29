#include "Light\SkyBoxLight.h"

#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "include\stb_image.h"
#include "core\Spectrum.h"
#include <Sampler/Sampling.h>

namespace PBR {
// 将一个归一化的 3D 方向向量 p 转换为环境贴图上的 2D UV 纹理坐标 (u, v)。
void get_sphere_uv(const Vector3f&p, float &u, float &v) {
	float phi = atan2(p.z, p.x);
	float theta = asin(p.y);
	u = 1 - (phi + Pi) * Inv2Pi;
	v = (theta + PiOver2) * InvPi;
}
// 加载 HDR 环境贴图文件
bool SkyBoxLight::loadImage(const char* imageFile) {
	stbi_set_flip_vertically_on_load(true);
	data = stbi_loadf(imageFile, &imageWidth, &imageHeight, &nrComponents, 0);
	if (data)return true;
	else return false;
}
// 根据给定的 UV 坐标从已加载的 HDR 图像数据 data 中查找并返回对应的颜色值。
// 这里直接转化成了LDR
Spectrum SkyBoxLight::getLightValue(float u, float v) const{
	u = Clamp(u, 0.f, 1.f);
	v = Clamp(v, 0.f, 1.f);
	int w = u * imageWidth, h = v * imageHeight;
	w = Clamp(w, 0, imageWidth - 1);
	h = Clamp(h, 0, imageHeight - 1);
	int offset = (w + h*imageWidth) * nrComponents;
	Spectrum Lv;
	Lv[0] = data[offset + 0];
	Lv[1] = data[offset + 1];
	Lv[2] = data[offset + 2];
	Lv = RGBSpectrum::HDRtoLDR(Lv, 0.3);
	return Lv;
}

// 均匀球面光源采样
Spectrum SkyBoxLight::Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi,
	float* pdf, VisibilityTester* vis) const {
	// 1. 使用均匀球面采样来生成 wi
	*wi = UniformSampleSphere(u);
	*pdf = 1.f / (4 * Pi);
	// 3. 设置可见性
	*vis = VisibilityTester(ref, Interaction(ref.p + *wi * (2 * worldRadius), ref.time));
	// 4. 将新生成的方向 *wi* 转换回 (u,v) 坐标
	float u_lookup, v_lookup;
	get_sphere_uv(Normalize(*wi), u_lookup, v_lookup);
	// 5. 从 (u,v) 坐标获取正确的颜色
	if (!data) return Spectrum(0.f); // 如果没加载HDRI，就不发光
	return getLightValue(u_lookup, v_lookup);
}

//查询自发光
Spectrum SkyBoxLight::Le(const Ray& ray) const {
	// 1. 无限光源只关心“方向”
	Vector3f dir_normalized = Normalize(ray.d);
	// 2. 将这个方向转换为 (u,v) 坐标
	float u, v;
	get_sphere_uv(dir_normalized, u, v); // <--- 使用您的辅助函数
	Spectrum Col;
	if (data) {
		// 3. 如果 HDRI 图像已加载，就从图像中获取颜色
		Col = getLightValue(u, v);
	}
	else {
		// 4. 如果没有 HDR 数据，使用方向->颜色的测试逻辑
		Col[0] = (dir_normalized.x + 1.f) * 0.5f;
		Col[1] = (dir_normalized.y + 1.f) * 0.5f;
		Col[2] = (dir_normalized.z + 1.f) * 0.5f;
	}
	return Col;
}
}




















