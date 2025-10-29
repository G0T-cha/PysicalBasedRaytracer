#include "Core\PBR.h"
#include "Sampler\Sampler.h"

namespace PBR {

// Sampler Method Definitions
Sampler::~Sampler() {}
Sampler::Sampler(int64_t samplesPerPixel) : samplesPerPixel(samplesPerPixel) {}
//调用Get2D() 和 Get1D() 来填充 pFilm、pLens 和 time。
CameraSample Sampler::GetCameraSample(const Point2i &pRaster) {
    CameraSample cs;
    cs.pFilm = (Point2f)pRaster + Get2D();
    cs.time = Get1D();
    cs.pLens = Get2D();
	/*
	cs.pFilm = (Point2f)pRaster;
	cs.time = 0.0f;
	cs.pLens = Point2f(0.0f, 0.0f);
	*/
    return cs;
}
//开始渲染一个新像素时必须调用它，以重置采样器的内部状态。
void Sampler::StartPixel(const Point2i &p) {
    currentPixel = p;
    currentPixelSampleIndex = 0;
    array1DOffset = array2DOffset = 0;
}

//Integrator 在一个像素内完成一次采样后调用它，使采样器前进到下一个样本索引。
bool Sampler::StartNextSample() {
    // Reset array offsets for next pixel sample
    array1DOffset = array2DOffset = 0;
    return ++currentPixelSampleIndex < samplesPerPixel;
}
bool Sampler::SetSampleNumber(int64_t sampleNum) {
    // Reset array offsets for next pixel sample
    array1DOffset = array2DOffset = 0;
    currentPixelSampleIndex = sampleNum;
    return currentPixelSampleIndex < samplesPerPixel;
}
//批量采样
void Sampler::Request1DArray(int n) {
    //CHECK_EQ(RoundCount(n), n);
    samples1DArraySizes.push_back(n);
    sampleArray1D.push_back(std::vector<float>(n * samplesPerPixel));
}
void Sampler::Request2DArray(int n) {
    //CHECK_EQ(RoundCount(n), n);
    samples2DArraySizes.push_back(n);
    sampleArray2D.push_back(std::vector<Point2f>(n * samplesPerPixel));
}
//获取批量采样的样本
const float *Sampler::Get1DArray(int n) {
    if (array1DOffset == sampleArray1D.size()) return nullptr;
    //CHECK_EQ(samples1DArraySizes[array1DOffset], n);
    //CHECK_LT(currentPixelSampleIndex, samplesPerPixel);
    return &sampleArray1D[array1DOffset++][currentPixelSampleIndex * n];
}
const Point2f *Sampler::Get2DArray(int n) {
    if (array2DOffset == sampleArray2D.size()) return nullptr;
    //CHECK_EQ(samples2DArraySizes[array2DOffset], n);
    //CHECK_LT(currentPixelSampleIndex, samplesPerPixel);
    return &sampleArray2D[array2DOffset++][currentPixelSampleIndex * n];
}

//在 StartPixel 时一次性生成并存储该像素所需的所有样本。
PixelSampler::PixelSampler(int64_t samplesPerPixel, int nSampledDimensions)
	: Sampler(samplesPerPixel) {
	for (int i = 0; i < nSampledDimensions; ++i) {
		samples1D.push_back(std::vector<float>(samplesPerPixel));
		samples2D.push_back(std::vector<Point2f>(samplesPerPixel));
	}
}
bool PixelSampler::StartNextSample() {
	current1DDimension = current2DDimension = 0;
	return Sampler::StartNextSample();
}
bool PixelSampler::SetSampleNumber(int64_t sampleNum) {
	current1DDimension = current2DDimension = 0;
	return Sampler::SetSampleNumber(sampleNum);
}
//从 samples2D 缓存中返回，如果超过了预先分配的维度，会降级为低质量的伪随机数
float PixelSampler::Get1D() {
	if (current1DDimension < samples1D.size())
		return samples1D[current1DDimension++][currentPixelSampleIndex];
	else
		return rng.UniformFloat();
}
Point2f PixelSampler::Get2D() {
	if (current2DDimension < samples2D.size())
		return samples2D[current2DDimension++][currentPixelSampleIndex];
	else
		return Point2f(rng.UniformFloat(), rng.UniformFloat());
}

//这种采样器将整个图像的所有像素、所有样本索引（例如 800*600*64 个样本）映射到样本序列中
void GlobalSampler::StartPixel(const Point2i &p) {
	Sampler::StartPixel(p);
	dimension = 0;
	intervalSampleIndex = GetIndexForSample(0);
	arrayEndDim =
		arrayStartDim + sampleArray1D.size() + 2 * sampleArray2D.size();
	for (size_t i = 0; i < samples1DArraySizes.size(); ++i) {
		int nSamples = samples1DArraySizes[i] * samplesPerPixel;
		for (int j = 0; j < nSamples; ++j) {
			int64_t index = GetIndexForSample(j);
			sampleArray1D[i][j] = SampleDimension(index, arrayStartDim + i);
		}
	}
	int dim = arrayStartDim + samples1DArraySizes.size();
	for (size_t i = 0; i < samples2DArraySizes.size(); ++i) {
		int nSamples = samples2DArraySizes[i] * samplesPerPixel;
		for (int j = 0; j < nSamples; ++j) {
			int64_t idx = GetIndexForSample(j);
			sampleArray2D[i][j].x = SampleDimension(idx, dim);
			sampleArray2D[i][j].y = SampleDimension(idx, dim + 1);
		}
		dim += 2;
	}
}
bool GlobalSampler::StartNextSample() {
	dimension = 0;
	intervalSampleIndex = GetIndexForSample(currentPixelSampleIndex + 1);
	return Sampler::StartNextSample();
}
bool GlobalSampler::SetSampleNumber(int64_t sampleNum) {
	dimension = 0;
	intervalSampleIndex = GetIndexForSample(sampleNum);
	return Sampler::SetSampleNumber(sampleNum);
}
float GlobalSampler::Get1D() {
	if (dimension >= arrayStartDim && dimension < arrayEndDim)
		dimension = arrayEndDim;
	return SampleDimension(intervalSampleIndex, dimension++);
}
Point2f GlobalSampler::Get2D() {
	if (dimension + 1 >= arrayStartDim && dimension < arrayEndDim)
		dimension = arrayEndDim;
	Point2f p(SampleDimension(intervalSampleIndex, dimension),
		SampleDimension(intervalSampleIndex, dimension + 1));
	dimension += 2;
	return p;
}













}



































