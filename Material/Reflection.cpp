#include "Material\Reflection.h"
#include "Sampler\Sampling.h"

namespace PBR{
	// 给定出射方向 wo（指向相机），和一个 2d 采样样本，根据物理特性，生成入射方向 wi
	// 默认行为是余弦加权半球采样，兰伯特反射将继承这个行为
	Spectrum BxDF::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& sample, float* pdf, BxDFType* sampledType) const
	{
		*wi = CosineSampleHemisphere(sample);
		if (wo.z < 0) wi->z *= -1;
		*pdf = Pdf(wo, *wi);
		return f(wo,*wi);
	}
	// 当光线从半球所有可能的入射方向照到表面时，多少光线(颜色)被反射到了特定的出射方向 w
	// 默认行为是蒙特卡洛积分估算，通过不断调用Sample_f，累加蒙特卡洛估算子，返回所有样本贡献的平均值
	// 兰伯特反射由于存在解析解，重写了这个方法
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

	// 从各个方向来的光，有多少散射到了所有方向
	// 蒙特卡洛双重积分
	// 兰伯特反射由于存在解析解，重写了这个方法
	Spectrum BxDF::rho(int nSamples, const Point2f* samples1, const Point2f* samples2) const
	{
		Spectrum r(0.f);
		for (int i = 0; i < nSamples; ++i) {
			Vector3f wo, wi;
			//均匀采样出射方向
			wo = UniformSampleHemisphere(samples1[i]);
			float pdfo = UniformHemispherePdf(), pdfi = 0;
			//重要性采样入射方向
			Spectrum f = Sample_f(wo, &wi, samples2[i], &pdfi);
			if (pdfi > 0) {
				r += f * AbsCosTheta(wi) * AbsCosTheta(wo) / (pdfo * pdfi);
			}
		}
		return r / (Pi * nSamples);
	}
	// 双向分布函数的默认 PDF 查询函数。
	// 反射为弦加权半球采样的 PDF，不是反射则为0
	// 镜面反射重写了其为0
	float BxDF::Pdf(const Vector3f& wo, const Vector3f& wi) const
	{
		return SameHemisphere(wo, wi) ? AbsCosTheta(wi) * InvPi : 0;
	}

	// 计算总的 BSDF (双向散射分布函数) 值，返回一个光谱
	Spectrum BSDF::f(const Vector3f& woW, const Vector3f& wiW, BxDFType flags) const
	{
		//世界空间转本地空间
		Vector3f wi = WorldToLocal(wiW), wo = WorldToLocal(woW);
		if (wo.z == 0) return 0.;
		// 根据是否在法线同侧判断是反射还是透射
		bool reflect = Dot(wiW, ng) * Dot(woW, ng) > 0;
		Spectrum f(0.f);
		// 累加所有匹配的BxDF，即反射或透射
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(flags)&&
				((reflect && (bxdfs[i]->type & BSDF_REFLECTION))||
					(!reflect && (bxdfs[i]->type & BSDF_TRANSMISSION))))
				f += bxdfs[i]->f(wo, wi);
		return f;
	}
	//计算委托给其包含的 BxDF (双向分布函数) 成员，并将它们的总反射率加起来
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
	// 计算给定wo和wi方向对，Sample_f本应采样到wi的总概率密度。
	// 为多重重要性采样 (MIS) 服务
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
	// 随机选择一个 BxDF (双向分布函数) 来生成方向 wi，并计算对应的总平均 PDF 和总和 f 值
	Spectrum BSDF::Sample_f(const Vector3f& woWorld, Vector3f* wiWorld,
		const Point2f& u, float* pdf, BxDFType type,
		BxDFType* sampledType) const {
		// 过滤符合要求的 BxDF
		int matchingComps = NumComponents(type);
		if (matchingComps == 0) {
			*pdf = 0;
			if (sampledType) *sampledType = BxDFType(0);
			return Spectrum(0);
		}
		//随机选择一个BxDF
		int comp =
			std::min((int)std::floor(u[0] * matchingComps), matchingComps - 1);
		// 获取其指针
		std::shared_ptr<BxDF>bxdf = nullptr;
		int count = comp;
		for (int i = 0; i < nBxDFs; ++i)
			if (bxdfs[i]->MatchesFlags(type) && count-- == 0) {
				bxdf = bxdfs[i];
				break;
			}
		// 重映射样本u
		Point2f uRemapped(std::min(u[0] * matchingComps - comp, OneMinusEpsilon),
			u[1]);

		// 坐标转换 调用选中的BxDF
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

		// 如果不是镜面，累加，计算最终的PDF
		if (!(bxdf->type & BSDF_SPECULAR) && matchingComps > 1)
			for (int i = 0; i < nBxDFs; ++i)
				if (bxdfs[i] != bxdf && bxdfs[i]->MatchesFlags(type))
					*pdf += bxdfs[i]->Pdf(wo, wi);
		if (matchingComps > 1) *pdf /= matchingComps;

		// 如果不是镜面，累加，计算最终的f
		if (!(bxdf->type & BSDF_SPECULAR)) {
			bool reflect = Dot(*wiWorld, ng) * Dot(woWorld, ng) > 0;
			f = 0.;
			for (int i = 0; i < nBxDFs; ++i)
				if (bxdfs[i]->MatchesFlags(type) &&
					((reflect && (bxdfs[i]->type & BSDF_REFLECTION)) ||
						(!reflect && (bxdfs[i]->type & BSDF_TRANSMISSION))))
					f += bxdfs[i]->f(wo, wi);
		}
		// 返回f
		return f;
	}

	BSDF::~BSDF() {
	}

	// 给定出射方向和入射方向，散射的能量（光谱）
	// 理想漫反射为 R / PI
	Spectrum LambertianReflection::f(const Vector3f& wo, const Vector3f& wi) const {
		return R * InvPi;
	}

	// 粗糙漫反射，依赖粗糙度
	Spectrum OrenNayar::f(const Vector3f& wo, const Vector3f& wi) const {
		float sinThetaI = SinTheta(wi);
		float sinThetaO = SinTheta(wo);
		// Compute cosine term of Oren-Nayar model
		float maxCos = 0;
		if (sinThetaI > 1e-4 && sinThetaO > 1e-4) {
			float sinPhiI = SinPhi(wi), cosPhiI = CosPhi(wi);
			float sinPhiO = SinPhi(wo), cosPhiO = CosPhi(wo);
			float dCos = cosPhiI * cosPhiO + sinPhiI * sinPhiO;
			maxCos = std::max((float)0, dCos);
		}

		// Compute sine and tangent terms of Oren-Nayar model
		float sinAlpha, tanBeta;
		if (AbsCosTheta(wi) > AbsCosTheta(wo)) {
			sinAlpha = sinThetaO;
			tanBeta = sinThetaI / AbsCosTheta(wi);
		}
		else {
			sinAlpha = sinThetaI;
			tanBeta = sinThetaO / AbsCosTheta(wo);
		}
		return R * InvPi * (A + B * maxCos * sinAlpha * tanBeta);
	}

	// 给定出射方向 wo（指向相机），和一个 2d 采样样本，根据物理特性，生成入射方向 wi
	// 在镜面反射中，求得wi，pdf为1，返回菲涅尔对象计算的实际被反射光的比例
	// 除以 cos(theta) 是因为后续需要乘，这里约掉
	Spectrum SpecularReflection::Sample_f(const Vector3f& wo, Vector3f* wi,
		const Point2f& sample, float* pdf, BxDFType* sampledType) const {
		*wi = Vector3f(-wo.x, -wo.y, wo.z);
		*pdf = 1;
		return fresnel->Evaluate(CosTheta(*wi)) * R / AbsCosTheta(*wi);
	}

	// 斯涅尔定律计算折射方向、检查全内反射、菲涅尔计算透射能量
	Spectrum SpecularTransmission::Sample_f(const Vector3f& wo, Vector3f* wi,
		const Point2f& sample, float* pdf, BxDFType* sampledType) const {
		bool entering = CosTheta(wo) > 0;
		float etaI = entering ? etaA : etaB;
		float etaT = entering ? etaB : etaA;
		if (!Refract(wo, Faceforward(Normal3f(0, 0, 1), wo), etaI / etaT, wi)) {
			/**wi = Vector3f(-wo.x, -wo.y, wo.z);
			*pdf = 1;
			return Spectrum(1.) * T / AbsCosTheta(*wi);*/
			return 0;
		}
		else {
			*pdf = 1;
			Spectrum ft = T * (Spectrum(1.) - fresnel.Evaluate(CosTheta(*wi)));
			if (mode == TransportMode::Radiance) ft *= (etaI * etaI) / (etaT * etaT);
			return ft / AbsCosTheta(*wi);
		}
	}

	// 菲涅尔计算反射概率，根据采样的随机数判断反射或折射
	Spectrum FresnelSpecular::Sample_f(const Vector3f& wo, Vector3f* wi,
		const Point2f& u, float* pdf, BxDFType* sampledType) const {
		float F = FrDielectric(CosTheta(wo), etaA, etaB);
		if (u[0] < F) {
			*wi = Vector3f(-wo.x, -wo.y, wo.z);
			if (sampledType)
				*sampledType = BxDFType(BSDF_SPECULAR | BSDF_REFLECTION);
			*pdf = F;
			return F * R / AbsCosTheta(*wi);
		}
		else {
			bool entering = CosTheta(wo) > 0;
			float etaI = entering ? etaA : etaB;
			float etaT = entering ? etaB : etaA;
			if (!Refract(wo, Faceforward(Normal3f(0, 0, 1), wo), etaI / etaT, wi))
				return 0;
			Spectrum ft = T * (1 - F);
			if (mode == TransportMode::Radiance)
				ft *= (etaI * etaI) / (etaT * etaT);
			if (sampledType)
				*sampledType = BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION);
			*pdf = 1 - F;
			return ft / AbsCosTheta(*wi);
		}
	}

	// DGF乘在一起：朝向的微表面*是否被遮挡*反射了多少能量
	Spectrum MicrofacetReflection::f(const Vector3f& wo, const Vector3f& wi) const {
		float cosThetaO = AbsCosTheta(wo), cosThetaI = AbsCosTheta(wi);
		Vector3f wh = wi + wo;
		// Handle degenerate cases for microfacet reflection
		if (cosThetaI == 0 || cosThetaO == 0) return Spectrum(0.);
		if (wh.x == 0 && wh.y == 0 && wh.z == 0) return Spectrum(0.);
		wh = Normalize(wh);
		// For the Fresnel call, make sure that wh is in the same hemisphere
		// as the surface normal, so that TIR is handled correctly.
		Spectrum F = fresnel->Evaluate(Dot(wi, Faceforward(wh, Vector3f(0, 0, 1))));
		return R * distribution->D(wh) * distribution->G(wo, wi) * F /
			(4 * cosThetaI * cosThetaO);
	}

	// 按概率采样高光，使用f计算颜色
	Spectrum MicrofacetReflection::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
		float* pdf, BxDFType* sampledType) const {
		if (wo.z == 0) return 0.;
		Vector3f wh = distribution->Sample_wh(wo, u);
		if (Dot(wo, wh) < 0) return 0.;   
		*wi = Reflect(wo, wh);
		if (!SameHemisphere(wo, *wi)) return Spectrum(0.f);

		*pdf = distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
		return f(wo, *wi);
	}

	// Sample_f得到的概率
	float MicrofacetReflection::Pdf(const Vector3f& wo, const Vector3f& wi) const {
		if (!SameHemisphere(wo, wi)) return 0;
		Vector3f wh = Normalize(wo + wi);
		return distribution->Pdf(wo, wh) / (4 * Dot(wo, wh));
	}

	// 折射版本的T-S公式
	Spectrum MicrofacetTransmission::f(const Vector3f& wo, const Vector3f& wi) const {
		if (SameHemisphere(wo, wi)) return 0;  // transmission only

		float cosThetaO = CosTheta(wo);
		float cosThetaI = CosTheta(wi);
		if (cosThetaI == 0 || cosThetaO == 0) return Spectrum(0);

		// Compute $\wh$ from $\wo$ and $\wi$ for microfacet transmission
		float eta = CosTheta(wo) > 0 ? (etaB / etaA) : (etaA / etaB);
		Vector3f wh = Normalize(wo + wi * eta);
		if (wh.z < 0) wh = -wh;

		// Same side?
		if (Dot(wo, wh) * Dot(wi, wh) > 0) return Spectrum(0);

		Spectrum F = fresnel.Evaluate(Dot(wo, wh));

		float sqrtDenom = Dot(wo, wh) + eta * Dot(wi, wh);
		float factor = (mode == TransportMode::Radiance) ? (1 / eta) : 1;

		return (Spectrum(1.f) - F) * T *
			std::abs(distribution->D(wh) * distribution->G(wo, wi) * eta * eta *
				AbsDot(wi, wh) * AbsDot(wo, wh) * factor * factor /
				(cosThetaI * cosThetaO * sqrtDenom * sqrtDenom));
	}

	// 采样折射光
	Spectrum MicrofacetTransmission::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& u,
		float* pdf, BxDFType* sampledType) const {
		if (wo.z == 0) return 0.;
		Vector3f wh = distribution->Sample_wh(wo, u);
		if (Dot(wo, wh) < 0) return 0.;  // Should be rare

		float eta = CosTheta(wo) > 0 ? (etaA / etaB) : (etaB / etaA);
		if (!Refract(wo, (Normal3f)wh, eta, wi)) return 0;
		*pdf = Pdf(wo, *wi);
		return f(wo, *wi);
	}

	// 折射的PDF
	float MicrofacetTransmission::Pdf(const Vector3f& wo, const Vector3f& wi) const {
		if (SameHemisphere(wo, wi)) return 0;
		// Compute $\wh$ from $\wo$ and $\wi$ for microfacet transmission
		float eta = CosTheta(wo) > 0 ? (etaB / etaA) : (etaA / etaB);
		Vector3f wh = Normalize(wo + wi * eta);

		if (Dot(wo, wh) * Dot(wi, wh) > 0) return 0;

		// Compute change of variables _dwh\_dwi_ for microfacet transmission
		float sqrtDenom = Dot(wo, wh) + eta * Dot(wi, wh);
		float dwh_dwi =
			std::abs((eta * eta * Dot(wi, wh)) / (sqrtDenom * sqrtDenom));
		return distribution->Pdf(wo, wh) * dwh_dwi;
	}

	FresnelBlend::FresnelBlend(const Spectrum& Rd, const Spectrum& Rs,
		MicrofacetDistribution* distribution)
		: BxDF(BxDFType(BSDF_REFLECTION | BSDF_GLOSSY)),
		Rd(Rd),
		Rs(Rs),
		distribution(distribution) {}

	// 将颜色分为两部分
	Spectrum FresnelBlend::f(const Vector3f& wo, const Vector3f& wi) const {
		auto pow5 = [](float v) { return (v * v) * (v * v) * v; };
		Spectrum diffuse = (28.f / (23.f * Pi)) * Rd * (Spectrum(1.f) - Rs) *
			(1 - pow5(1 - .5f * AbsCosTheta(wi))) *
			(1 - pow5(1 - .5f * AbsCosTheta(wo)));
		Vector3f wh = wi + wo;
		if (wh.x == 0 && wh.y == 0 && wh.z == 0) return Spectrum(0);
		wh = Normalize(wh);
		Spectrum specular =
			distribution->D(wh) /
			(4 * AbsDot(wi, wh) * std::max(AbsCosTheta(wi), AbsCosTheta(wo))) *
			SchlickFresnel(Dot(wi, wh));
		return diffuse + specular;
	}

	// 50%概率漫反射
	Spectrum FresnelBlend::Sample_f(const Vector3f& wo, Vector3f* wi, const Point2f& uOrig,
		float* pdf, BxDFType* sampledType) const {
		Point2f u = uOrig;
		if (u[0] < .5) {
			u[0] = std::min(2 * u[0], OneMinusEpsilon);
			// Cosine-sample the hemisphere, flipping the direction if necessary
			*wi = CosineSampleHemisphere(u);
			if (wo.z < 0) wi->z *= -1;
		}
		else {
			u[0] = std::min(2 * (u[0] - .5f), OneMinusEpsilon);
			// Sample microfacet orientation $\wh$ and reflected direction $\wi$
			Vector3f wh = distribution->Sample_wh(wo, u);
			*wi = Reflect(wo, wh);
			if (!SameHemisphere(wo, *wi)) return Spectrum(0.f);
		}
		*pdf = Pdf(wo, *wi);
		return f(wo, *wi);
	}
	float FresnelBlend::Pdf(const Vector3f& wo, const Vector3f& wi) const {
		if (!SameHemisphere(wo, wi)) return 0;
		Vector3f wh = Normalize(wo + wi);
		float pdf_wh = distribution->Pdf(wo, wh);
		return .5f * (AbsCosTheta(wi) * InvPi + pdf_wh / (4 * Dot(wo, wh)));
	}
}