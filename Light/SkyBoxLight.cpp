#include "Light\SkyBoxLight.h"

#include <cmath>

#define STB_IMAGE_IMPLEMENTATION
#include "include\stb_image.h"
#include "core\Spectrum.h"
#include <Sampler/Sampling.h>

namespace PBR {
// ��һ����һ���� 3D �������� p ת��Ϊ������ͼ�ϵ� 2D UV �������� (u, v)��
void get_sphere_uv(const Vector3f&p, float &u, float &v) {
	float phi = atan2(p.z, p.x);
	float theta = asin(p.y);
	u = 1 - (phi + Pi) * Inv2Pi;
	v = (theta + PiOver2) * InvPi;
}
// ���� HDR ������ͼ�ļ�
bool SkyBoxLight::loadImage(const char* imageFile) {
	stbi_set_flip_vertically_on_load(true);
	data = stbi_loadf(imageFile, &imageWidth, &imageHeight, &nrComponents, 0);
	if (data)return true;
	else return false;
}
// ���ݸ����� UV ������Ѽ��ص� HDR ͼ������ data �в��Ҳ����ض�Ӧ����ɫֵ��
// ����ֱ��ת������LDR
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

// ���������Դ����
Spectrum SkyBoxLight::Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi,
	float* pdf, VisibilityTester* vis) const {
	// 1. ʹ�þ���������������� wi
	*wi = UniformSampleSphere(u);
	*pdf = 1.f / (4 * Pi);
	// 3. ���ÿɼ���
	*vis = VisibilityTester(ref, Interaction(ref.p + *wi * (2 * worldRadius), ref.time));
	// 4. �������ɵķ��� *wi* ת���� (u,v) ����
	float u_lookup, v_lookup;
	get_sphere_uv(Normalize(*wi), u_lookup, v_lookup);
	// 5. �� (u,v) �����ȡ��ȷ����ɫ
	if (!data) return Spectrum(0.f); // ���û����HDRI���Ͳ�����
	return getLightValue(u_lookup, v_lookup);
}

//��ѯ�Է���
Spectrum SkyBoxLight::Le(const Ray& ray) const {
	// 1. ���޹�Դֻ���ġ�����
	Vector3f dir_normalized = Normalize(ray.d);
	// 2. ���������ת��Ϊ (u,v) ����
	float u, v;
	get_sphere_uv(dir_normalized, u, v); // <--- ʹ�����ĸ�������
	Spectrum Col;
	if (data) {
		// 3. ��� HDRI ͼ���Ѽ��أ��ʹ�ͼ���л�ȡ��ɫ
		Col = getLightValue(u, v);
	}
	else {
		// 4. ���û�� HDR ���ݣ�ʹ�÷���->��ɫ�Ĳ����߼�
		Col[0] = (dir_normalized.x + 1.f) * 0.5f;
		Col[1] = (dir_normalized.y + 1.f) * 0.5f;
		Col[2] = (dir_normalized.z + 1.f) * 0.5f;
	}
	return Col;
}
}




















