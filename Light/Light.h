#pragma once
#ifndef __Light_h__
#define __Light_h__

#include "Core\PBR.h"
#include "Core\Transform.h"
#include "Core\Interaction.h"
#include "Core\Spectrum.h"

namespace PBR{

// ��װ����ɼ��Բ����������Ϣ
class VisibilityTester {
public:
	VisibilityTester() {}
	VisibilityTester(const Interaction& p0, const Interaction& p1)
		: p0(p0), p1(p1) {}
	const Interaction& P0() const { return p0; }
	const Interaction& P1() const { return p1; }
    //���ķ���
	bool Unoccluded(const Scene& scene) const;
    //����͸���ʣ����������Ⱦ
	Spectrum Tr(const Scene& scene, Sampler& sampler) const { return Spectrum(0.f); }

private:
	Interaction p0, p1;
};

// λ���룺���Դ�����뷽��⡢���Դ�����޹�Դ����պС�������ȣ�
enum class LightFlags : int {
    DeltaPosition = 1,
    DeltaDirection = 2,
    Area = 4,
    Infinite = 8
};

// Delta��Դ���ԣ����ܱ�BSDF���ɻ��У���ʹ�ù�Դ����
inline bool IsDeltaLight(int flags) {
    return flags & (int)LightFlags::DeltaPosition ||
        flags & (int)LightFlags::DeltaDirection;
}

class Light {
public:
    virtual ~Light() {}
    // ��Դ���͡���Դ�ռ䵽����ռ䡢������
    Light(int flags, const Transform& LightToWorld, int nSamples = 1);
    // ִ�й�Դ����
    // ��������ɫ��ref��2d��������u����������ⷽ��wi�������pdf�Ϳɼ��Բ�����vis�����ع��ף��÷����������ն�Li
    virtual Spectrum Sample_Li(const Interaction& ref, const Point2f& u,
        Vector3f* wi, float* pdf,
        VisibilityTester* vis) const = 0;
    // �ܹ��ʣ�·��׷��ʱ��Ҫ���ݳ��������й�Դ�Ĺ��ʣ����������ѡ��һ����Դ���в��������Ȳ���������ǿ�Ĺ�Դ��
    virtual Spectrum Power() const = 0;
    // ��ѡԤ����
    virtual void Preprocess(const Scene& scene) {}
    // ��ѯ�Է��⣬����δ�����κ�������߻����˹�Դ�������Է���
    virtual Spectrum Le(const Ray& r) const { return Spectrum(0.8f); }
    // ��ѯ��Դ����PDF������MIS
    // ���� ref �� wi������ Sample_Li ��Ӧ������ wi �� pdf
    virtual float Pdf_Li(const Interaction& ref, const Vector3f& wi) const = 0;
    // �����Է���
    // �ӹ�Դ����������������һ��������ߣ����������ߡ���㷨�ߡ�ѡ�������ʣ��������ѡ������ʣ�����ǣ�
    // ˫��·��׷�١�����ӳ����Ҫ
    virtual Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, float time,
        Ray* ray, Normal3f* nLight, float* pdfPos,
        float* pdfDir) const = 0;
    // ��ѯ�Է��� PDF
    // ����һ���ӹ�Դ�����Ĺ��ߺ���㷨�ߣ�����Sample_Le��Ӧ�����������ߵĸ��ʣ�λ�úͷ���
    virtual void Pdf_Le(const Ray& ray, const Normal3f& nLight, float* pdfPos,
        float* pdfDir) const = 0;


    const int flags;
    const int nSamples;
protected:
    const Transform LightToWorld, WorldToLight;
};

// ���ࣺ�б�����Ĺ�Դ
class AreaLight : public Light {
public:
    AreaLight(const Transform& LightToWorld, int nSamples);
    // ��ѯ���Դ���նȣ����ع���
    virtual Spectrum L(const Interaction& intr, const Vector3f& w) const = 0;
};

}


#endif