#include "Texture\ImageTexture.h"
#include "include\stb_image.h"



namespace PBR {

template class ImageTexture<float, float>;
template class ImageTexture<RGBSpectrum, Spectrum>;

RGBSpectrum* loadImage(const std::string &filename, Point2i &resolution) {
	float *data;
	RGBSpectrum *data_s;
	if (filename != "") {
		//texels = ReadImage(texmap, &resolution);
		int imageWidth, imageHeight, nrComponents;
		stbi_set_flip_vertically_on_load(true);
		data = stbi_loadf(filename.c_str(), &imageWidth, &imageHeight, &nrComponents, 0);
		if (!data)return nullptr;
		data_s = new RGBSpectrum[imageWidth * imageHeight];
		for (int j = 0; j < imageHeight; j++) {
			for (int i = 0; i < imageWidth; i++) {
				RGBSpectrum r;
				r[0] = data[(i + j * imageWidth) * nrComponents + 0];
				r[1] = data[(i + j * imageWidth) * nrComponents + 1];
				r[2] = data[(i + j * imageWidth) * nrComponents + 2];
				data_s[i + j * imageWidth] = r;
			}
		}
		resolution.x = imageWidth;
		resolution.y = imageHeight;
		free(data);
		return data_s;
	}
	return nullptr;
}

// ImageTexture Method Definitions
template <typename Tmemory, typename Treturn>
ImageTexture<Tmemory, Treturn>::ImageTexture(
	std::unique_ptr<TextureMapping2D> mapping, const std::string &filename,
	bool doTrilinear, float maxAniso, ImageWrap wrapMode, float scale,
	bool gamma)
	: mapping(std::move(mapping)) {
	mipmap =
		GetTexture(filename, doTrilinear, maxAniso, wrapMode, scale, gamma);
}

// 全局纹理缓存
template <typename Tmemory, typename Treturn>
std::map<TexInfo, std::unique_ptr<MIPMap<Tmemory>>>
ImageTexture<Tmemory, Treturn>::textures;

template <typename Tmemory, typename Treturn>
MIPMap<Tmemory> *ImageTexture<Tmemory, Treturn>::GetTexture(
	const std::string &filename, bool doTrilinear, float maxAniso,
	ImageWrap wrap, float scale, bool gamma) {
	// 检查缓存
	TexInfo texInfo(filename, doTrilinear, maxAniso, wrap, scale, gamma);
	if (textures.find(texInfo) != textures.end())
		return textures[texInfo].get();

	// 加载新纹理
	Point2i resolution;
	std::unique_ptr<RGBSpectrum[]> texels(loadImage(filename, resolution));
	if (!texels) {
		resolution.x = resolution.y = 1;
		RGBSpectrum *rgb = new RGBSpectrum[1];
		*rgb = RGBSpectrum(0.5f);
		texels.reset(rgb);
	}

	// Flip image in y; texture coordinate space has (0,0) at the lower
	// left corner.
	for (int y = 0; y < resolution.y / 2; ++y)
		for (int x = 0; x < resolution.x; ++x) {
			int o1 = y * resolution.x + x;
			int o2 = (resolution.y - 1 - y) * resolution.x + x;
			//std::swap(texels[o1], texels[o2]);
		}
	// 创建MIPMAP
	MIPMap<Tmemory> *mipmap = nullptr;
	if (texels) {
		// Convert texels to type _Tmemory_ and create _MIPMap_
		std::unique_ptr<Tmemory[]> convertedTexels(
			new Tmemory[resolution.x * resolution.y]);
		for (int i = 0; i < resolution.x * resolution.y; ++i)
			convertIn(texels[i], &convertedTexels[i], scale, gamma);
		// 分辨率、像素数据
		mipmap = new MIPMap<Tmemory>(resolution, convertedTexels.get(),
			doTrilinear, maxAniso, wrap);
	}
	else {
		// Create one-valued _MIPMap_
		Tmemory oneVal = scale;
		mipmap = new MIPMap<Tmemory>(Point2i(1, 1), &oneVal);
	}
	// 存回缓存
	textures[texInfo].reset(mipmap);
	return mipmap;
}







}







