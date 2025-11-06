// 代码/Shape/ModelLoad.h (更新版)

#pragma once
#ifndef __ModelLoad_h__
#define __ModelLoad_h__


#include "Core\PBR.h"
#include "Shape\Triangle.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "Texture\ImageTexture.h"
#include "Media\Medium.h"

#include <memory>
#include <string>
#include <vector>
#include <map> // 包含 map

namespace PBR {


	class ModelLoad {
	public:
		//  模型数据  
		std::vector<std::shared_ptr<TriangleMesh>> meshes;
		std::vector<std::string> diffTexName, specTexName;

		// ==========================================================
		// 1. 在这里添加新的成员变量
		// ==========================================================
		std::vector<std::string> meshNames; // 用于存储网格名称
		// (我们还将使用一个 map 来预加载材质，以提高效率)
		std::map<std::string, std::shared_ptr<Material>> preloadedMaterials;

		std::string directory;
		//  函数  
		void loadModel(std::string path, const Transform& ObjectToWorld);
		void processNode(aiNode* node, const aiScene* scene, const Transform& ObjectToWorld);
		std::shared_ptr<TriangleMesh> processMesh(aiMesh* mesh, const aiScene* scene, const Transform& ObjectToWorld);
		void buildNoTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
			std::vector<std::shared_ptr<Primitive>>& prims, std::shared_ptr<Material> material);
		void ModelLoad::buildTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
			std::vector<std::shared_ptr<Primitive>>& prims);
	};
}
#endif