# Texture

目前没什么内容

## `Texture<T>` (抽象基类)

`Texture.h` 定义了 `Texture<T>` 模板基类。它是一个抽象接口，`T` 可以是 `Spectrum` (光谱)（用于颜色纹理）或 `float`（用于粗糙度、凹凸贴图等）。

**`virtual T Evaluate(const SurfaceInteraction& isect) const = 0;`**

- 是 `Texture` (纹理) 唯一的纯虚函数。目前`Material` (材质) 在 `ComputeScatteringFunctions` (计算散射函数) 中会调用此函数。
- **输入**: `isect` (交点)，包含了 `uv` (纹理坐标)、`dpdu` (dpdu)、`dpdv` (dpdv) 等所有几何信息。
- **输出**: 返回 `T` 类型的值（例如，`Spectrum` (光谱) 颜色）。

## `ConstantTexture<T>`常量纹理

它内部只持有一个 `T value;`（例如一个 `Spectrum` (光谱) 颜色）。它的 `Evaluate` (求值) 函数**完全忽略**传入的 `SurfaceInteraction` (表面相交) 信息，并简单地 `return value;`。