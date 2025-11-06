#pragma once
#ifndef PREMITIVE_H
#define PREMITIVE_H

#include "Core\Geometry.h"
#include "Material\Material.h"
#include "Media\Medium.h"

namespace PBR {
	//图元
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
	//几何图元：形状、材质、面光源
	class GeometricPrimitive : public Primitive {
	public:
		// GeometricPrimitive Public Methods
		virtual Bounds3f WorldBound() const;
		virtual bool Intersect(const Ray& r, SurfaceInteraction* isect) const;
		virtual bool IntersectP(const Ray& r) const;
		GeometricPrimitive(const std::shared_ptr<Shape>& shape,
			const std::shared_ptr<Material>& material,
			const std::shared_ptr<AreaLight>& areaLight,
			const MediumInterface& mediumInterface);

		const AreaLight* GetAreaLight() const;
		const Material* GetMaterial() const;
		virtual void ComputeScatteringFunctions(SurfaceInteraction* isect,
			TransportMode mode,
			bool allowMultipleLobes) const;
	private:
		std::shared_ptr<Material> material;
		std::shared_ptr<AreaLight> areaLight;
		std::shared_ptr<Shape> shape;
		MediumInterface mediumInterface;
	};

	//聚合图元，内部包含了其他图元，没有自己的材质或光源
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
