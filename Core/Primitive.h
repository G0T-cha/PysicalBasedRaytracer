#pragma once
#ifndef PREMITIVE_H
#define PREMITIVE_H

#include "Core\Geometry.h"
#include "Material\Material.h"

namespace PBR {
	//ͼԪ
	class Primitive {
	public:
		virtual ~Primitive();
		virtual Bounds3f WorldBound() const = 0;
		virtual bool Intersect(const Ray& r, SurfaceInteraction*) const = 0;
		virtual bool IntersectP(const Ray& r) const = 0;
		virtual const AreaLight* GetAreaLight() const = 0;
		virtual const Material* GetMaterial() const = 0;
		virtual void ComputeScatteringFunctions(SurfaceInteraction* isect,
			TransportMode mode,
			bool allowMultipleLobes) const = 0;
	};
	//����ͼԪ����״�����ʡ����Դ
	class GeometricPrimitive : public Primitive {
	public:
		// GeometricPrimitive Public Methods
		virtual Bounds3f WorldBound() const;
		virtual bool Intersect(const Ray& r, SurfaceInteraction* isect) const;
		virtual bool IntersectP(const Ray& r) const;
		GeometricPrimitive(const std::shared_ptr<Shape>& shape,
			const std::shared_ptr<Material>& material,
			const std::shared_ptr<AreaLight>& areaLight);

		const AreaLight* GetAreaLight() const;
		const Material* GetMaterial() const;
		virtual void ComputeScatteringFunctions(SurfaceInteraction* isect,
			TransportMode mode,
			bool allowMultipleLobes) const;
	private:
		std::shared_ptr<Material> material;
		std::shared_ptr<AreaLight> areaLight;
		std::shared_ptr<Shape> shape;
	};

	//�ۺ�ͼԪ���ڲ�����������ͼԪ��û���Լ��Ĳ��ʻ��Դ
	class Aggregate : public Primitive {
	public:
		// Aggregate Public Methods
		virtual void ComputeScatteringFunctions(SurfaceInteraction* isect,
			TransportMode mode,
			bool allowMultipleLobes) const {}
		const AreaLight* GetAreaLight() const { return nullptr; }
		const Material* GetMaterial() const { return nullptr; }
	};
}

#endif // !PREMITIVE_H
