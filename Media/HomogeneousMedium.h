#pragma once
#ifndef __HomogeneousMedium_h__
#define __HomogeneousMedium_h__

#include "Media\Medium.h"
#include "Core\PBR.h"
#include "Core\Spectrum.h"


namespace PBR {

	// 均匀参与介质
	class HomogeneousMedium : public Medium {
	public:
		HomogeneousMedium(const Spectrum &sigma_a, const Spectrum &sigma_s, float g)
			: sigma_a(sigma_a),
			sigma_s(sigma_s),
			sigma_t(sigma_s + sigma_a),
			g(g) {}
		// 透射率
		Spectrum Tr(const Ray &ray, Sampler &sampler) const;
		// 在光线 ray 的路径上随机采样一个散射点
		Spectrum Sample(const Ray &ray, Sampler &sampler,
			MediumInteraction *mi) const;
	private:
		const Spectrum sigma_a, sigma_s, sigma_t;
		const float g;
	};


}



#endif


