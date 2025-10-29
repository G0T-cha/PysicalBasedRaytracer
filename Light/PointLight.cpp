#include "Light\PointLight.h"
#include "Sampler\Sampling.h"

namespace PBR {
	Spectrum PointLight::Sample_Li(const Interaction& ref, const Point2f& u,
		Vector3f* wi, float* pdf,
		VisibilityTester* vis) const {
		// ����Ψһ�ķ���pdf��֮��Ϊ1
		*wi = Normalize(pLight - ref.p);
		*pdf = 1.f;
		*vis =
			VisibilityTester(ref, Interaction(pLight, ref.time));
		// ����ƽ�����ȵĹ���ǿ��˥��
		return I / DistanceSquared(pLight, ref.p);
	}

	Spectrum PointLight::Sample_Le(const Point2f& u1, const Point2f& u2, float time,
		Ray* ray, Normal3f* nLight, float* pdfPos, float* pdfDir) const {
		// ������ǵ��Դ�������������
		*ray = Ray(pLight, UniformSampleSphere(u1), Infinity, time);
		*nLight = (Normal3f)ray->d;
		*pdfPos = 1;
		*pdfDir = UniformSpherePdf();
		return I;
	}

	void PointLight::Pdf_Le(const Ray&, const Normal3f&, float* pdfPos,
		float* pdfDir) const {
		*pdfPos = 0;
		*pdfDir = UniformSpherePdf();
	}
}