#include "Material\Reflection.h"
#include "Sampler\Sampling.h"

namespace PBR{
	// �������䷽�� wo��ָ�����������һ�� 2d ���������������������ԣ��������䷽�� wi
	// Ĭ����Ϊ�����Ҽ�Ȩ��������������ط��佫�̳������Ϊ
	Spectrum BxDF::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample, float* pdf, BxDFType* sampledType) const
	{
		*wi = CosineSampleHemisphere(sample);
		if (wo.z < 0) wi->z *= -1;
		*pdf = Pdf(wo, *wi);
		return f(wo,*wi);
	}
	// �����ߴӰ������п��ܵ����䷽���յ�����ʱ�����ٹ���(��ɫ)�����䵽���ض��ĳ��䷽�� w
	// Ĭ����Ϊ�����ؿ�����ֹ��㣬ͨ�����ϵ���Sample_f���ۼ����ؿ�������ӣ����������������׵�ƽ��ֵ
	// �����ط������ڴ��ڽ����⣬��д���������
	Spectrum BxDF::rho(const Vector3f& w, int nSamples, const Point2f* u)const {
		Spectrum r(0.);
		for (int i = 0; i < nSamples; ++i) {
			Vector3f wi;
			float pdf = 0;
			Spectrum f = Sample_f(w, &wi, u[i], &pdf);
			if (pdf > 0) r += f * AbsCosTheta(wi) / pdf;
		}
		return r / nSamples;
	}

	// �Ӹ����������Ĺ⣬�ж���ɢ�䵽�����з���
	// ���ؿ���˫�ػ���
	// �����ط������ڴ��ڽ����⣬��д���������
	Spectrum BxDF::rho(int nSamples, const Point2f* samples1, const Point2f* samples2) const
	{
		Spectrum r(0.f);
		for (int i = 0; i < nSamples; ++i) {
			Vector3f wo, wi;
			//���Ȳ������䷽��
			wo = UniformSampleHemisphere(samples1[i]);
			float pdfo = UniformHemispherePdf(), pdfi = 0;
			//��Ҫ�Բ������䷽��
			Spectrum f = Sample_f(wo, &wi, samples2[i], &pdfi);
			if (pdfi > 0) {
				r += f * AbsCosTheta(wi) * AbsCosTheta(wo) / (pdfo * pdfi);
			}
		}
		return r / (Pi * nSamples);
	}
	// ˫��ֲ�������Ĭ�� PDF ��ѯ������
	// ����Ϊ�Ҽ�Ȩ��������� PDF�����Ƿ�����Ϊ0
	// ���淴����д����Ϊ0
	float BxDF::Pdf(const Vector3f& wo, const Vector3f& wi) const
	{
		return SameHemisphere(wo, wi) ? AbsCosTheta(wi) * InvPi : 0;
	}

	// �����ܵ� BSDF (˫��ɢ��ֲ�����) ֵ������һ������
	Spectrum BSDF::f(const Vector3f& woW, const Vector3f& wiW, BxDFType flags) const
	{
		//����ռ�ת���ؿռ�
		Vector3f wi = WorldToLocal(wiW), wo = WorldToLocal(woW);
		if (wo.z == 0) return 0.;
		// �����Ƿ��ڷ���ͬ���ж��Ƿ��仹��͸��
		bool reflect = Dot(wiW, ng) * Dot(woW, ng) > 0;
		Spectrum f(0.f);
		// �ۼ�����ƥ���BxDF���������͸��
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(flags)&&
				((reflect && (bxdfs[i]->type & BSDF_REFLECTION))||
					(!reflect && (bxdfs[i]->type & BSDF_TRANSMISSION))))
				f += bxdfs[i]->f(wo, wi);
		return f;
	}
	//����ί�и�������� BxDF (˫��ֲ�����) ��Ա���������ǵ��ܷ����ʼ�����
	Spectrum BSDF::rho(int nSamples, const Point2f* samples1, const Point2f* samples2, BxDFType flags) const
	{
		Spectrum ret(0.f);
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(flags))
				ret += bxdfs[i]->rho(nSamples, samples1, samples2);
		return ret;
	}
	Spectrum BSDF::rho(const Vector3f& woWorld, int nSamples, const Point2f* samples,
		BxDFType flags) const {
		Vector3f wo = WorldToLocal(woWorld);
		Spectrum ret(0.f);
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(flags))
				ret += bxdfs[i]->rho(wo, nSamples, samples);
		return ret;
	}
	// �������wo��wi����ԣ�Sample_f��Ӧ������wi���ܸ����ܶȡ�
	// Ϊ������Ҫ�Բ��� (MIS) ����
	float BSDF::Pdf(const Vector3f& woWorld, const Vector3f& wiWorld,
		BxDFType flags) const {
		if (nBxDFs == 0.f) return 0.f;
		Vector3f wo = WorldToLocal(woWorld), wi = WorldToLocal(wiWorld);
		if (wo.z == 0) return 0.;
		float pdf = 0.f;
		int matchingComps = 0;
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(flags)) {
				++matchingComps;
				pdf += bxdfs[i]->Pdf(wo, wi);
			}
		float v = matchingComps > 0 ? pdf / matchingComps : 0.f;
		return v;
	}
	// ���ѡ��һ�� BxDF (˫��ֲ�����) �����ɷ��� wi���������Ӧ����ƽ�� PDF ���ܺ� f ֵ
	Spectrum BSDF::Sample_f(const Vector3f& woWorld, Vector3f* wiWorld,
		const Point2f& u, float* pdf, BxDFType type,
		BxDFType* sampledType) const {
		// ���˷���Ҫ��� BxDF
		int matchingComps = NumComponents(type);
		if (matchingComps == 0) {
			*pdf = 0;
			if (sampledType) *sampledType = BxDFType(0);
			return Spectrum(0);
		}
		//���ѡ��һ��BxDF
		int comp =
			std::min((int)std::floor(u[0] * matchingComps), matchingComps - 1);
		// ��ȡ��ָ��
		BxDF* bxdf = nullptr;
		int count = comp;
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(type) && count-- == 0) {
				bxdf = bxdfs[i];
				break;
			}
		// ��ӳ������u
		Point2f uRemapped(std::min(u[0] * matchingComps - comp, OneMinusEpsilon),
			u[1]);

		// ����ת�� ����ѡ�е�BxDF
		Vector3f wi, wo = WorldToLocal(woWorld);
		if (wo.z == 0) return 0.;
		*pdf = 0;
		if (sampledType) *sampledType = bxdf->type;
		Spectrum f = bxdf->Sample_f(wo, &wi, uRemapped, pdf, sampledType);
		if (*pdf == 0) {
			if (sampledType) *sampledType = BxDFType(0);
			return 0;
		}
		*wiWorld = LocalToWorld(wi);

		// ������Ǿ��棬�ۼӣ��������յ�PDF
		if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1)
			for (int i = 0; i < nBxDFs; ++i)
				if (bxdfs[i] != bxdf && bxdfs[i]->MatchesFlags(type))
					*pdf += bxdfs[i]->Pdf(wo, wi);
		if (matchingComps > 1) *pdf /= matchingComps;

		// ������Ǿ��棬�ۼӣ��������յ�f
		if (!(bxdf->type & BSDF_SPECULAR)) {
			bool reflect = Dot(*wiWorld, ng) * Dot(woWorld, ng) > 0;
			f = 0.;
			for (int i = 0; i < nBxDFs; ++i)
				if (bxdfs[i]->MatchesFlags(type) &&
					((reflect && (bxdfs[i]->type & BSDF_REFLECTION)) ||
						(!reflect && (bxdfs[i]->type & BSDF_TRANSMISSION))))
					f += bxdfs[i]->f(wo, wi);
		}
		// ����f
		return f;
	}

	BSDF::~BSDF() {
		for (int i = 0; i < nBxDFs; i++)
			bxdfs[i]->~BxDF();
	}
	// �������䷽������䷽��ɢ������������ף�
	// ������Ϊ R / PI
	Spectrum LambertianReflection::f(const Vector3f& wo, const Vector3f& wi) const {
		return R * InvPi;
	}

	// �������䷽�� wo��ָ�����������һ�� 2d ���������������������ԣ��������䷽�� wi
	// �ھ��淴���У����wi��pdfΪ1�����ط�������������ʵ�ʱ������ı���
	// ���� cos(theta) ����Ϊ������Ҫ�ˣ�����Լ��
	Spectrum SpecularReflection::Sample_f(const Vector3f& wo, Vector3f* wi,
		const Point2f& sample, float* pdf, BxDFType* sampledType) const {
		*wi = Vector3f(-wo.x, -wo.y, wo.z);
		*pdf = 1;
		return fresnel->Evaluate(CosTheta(*wi)) * R / AbsCosTheta(*wi);
	}
}