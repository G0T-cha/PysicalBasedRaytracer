#pragma once
#ifndef __SkyBoxLight_h__
#define __SkyBoxLight_h__

#include "Light\Light.h"

namespace PBR {
class SkyBoxLight : public Light {
  public:
	  // ��պУ�����Զ��Դ
	  // ��ʼ�����������ģ�����뾶
	SkyBoxLight(const Transform &LightToWorld, const Point3f& worldCenter, float worldRadius, const char * file,  int nSamples)
	: Light((int)LightFlags::Infinite, LightToWorld, nSamples),
		worldCenter(worldCenter),
		worldRadius(worldRadius)
	{
		imageWidth = 0;
		imageHeight = 0;
		nrComponents = 0;
		data = nullptr;
		loadImage(file);
	}
	~SkyBoxLight() {
		if (data) free(data);
	}
    void Preprocess(const Scene &scene) {}
	bool loadImage(const char* imageFile);
	Spectrum getLightValue(float u, float v) const;
    Spectrum Power() const { return Spectrum(0.f); }
	// ��ѯ�Է���
	Spectrum Le(const Ray &ray) const;
	// ��Դ����
	Spectrum Sample_Li(const Interaction &ref, const Point2f &u, Vector3f *wi,
		float *pdf, VisibilityTester *vis) const;
	float Pdf_Li(const Interaction &, const Vector3f &) const { return 0.f; }
    Spectrum Sample_Le(const Point2f &u1, const Point2f &u2, float time,
                       Ray *ray, Normal3f *nLight, float *pdfPos,
                       float *pdfDir) const { return Spectrum(0.f); }
    void Pdf_Le(const Ray &, const Normal3f &, float *pdfPos, float *pdfDir) const {}

  private:
    Point3f worldCenter;
    float worldRadius;
	int imageWidth, imageHeight, nrComponents;
	float *data;

};
}
#endif







