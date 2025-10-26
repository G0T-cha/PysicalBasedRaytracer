#pragma once

#ifndef __Halton_h__
#define __Halton_h__

#include "Core\PBR.h"
#include "Sampler\Sampler.h"
#include "Sampler\LowDiscrepancy.h"

namespace PBR {

// HaltonSampler Declarations
class HaltonSampler : public GlobalSampler {
  public:
    // HaltonSampler Public Methods
    HaltonSampler(int nsamp, const Bounds2i &sampleBounds,
                  bool sampleAtCenter = false);
    int64_t GetIndexForSample(int64_t sampleNum) const;
    float SampleDimension(int64_t index, int dimension) const;
    std::unique_ptr<Sampler> Clone(int seed);

  private:
    // HaltonSampler Private Data
    static std::vector<uint16_t> radicalInversePermutations;
    Point2i baseScales, baseExponents;
    int sampleStride;
    int multInverse[2];
    mutable Point2i pixelForOffset = Point2i(std::numeric_limits<int>::max(),
                                             std::numeric_limits<int>::max());
    mutable int64_t offsetForCurrentPixel;
    // Added after book publication: force all image samples to be at the
    // center of the pixel area.
    bool sampleAtPixelCenter;

    // HaltonSampler Private Methods
    const uint16_t *PermutationForDimension(int dim) const {
        return &radicalInversePermutations[PrimeSums[dim]];
    }
};

HaltonSampler *CreateHaltonSampler(const Bounds2i &sampleBounds);


}





#endif



