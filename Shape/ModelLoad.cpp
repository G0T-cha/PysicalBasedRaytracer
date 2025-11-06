// 代码/Shape/ModelLoad.cpp (更新版)

#include "Shape\ModelLoad.h"
#include "Core\Geometry.h"
#include "Core\Transform.h"
#include "Core\primitive.h"
#include "Texture\ConstantTexture.h"
#include "Texture\ImageTexture.h"      // 确保 ImageTexture 被包含
#include "Material\MatteMaterial.h"
#include "Material\PlasticMaterial.h"
#include <assimp/scene.h> 
#include <iostream> 
#include <string>
#include <vector>
#include <map> 

namespace PBR {

    // (processMesh, processNode, loadModel, getDiffuseMaterial 保持不变)
    // ...
    std::shared_ptr<TriangleMesh> ModelLoad::processMesh(aiMesh* mesh, const aiScene* scene, const Transform& ObjectToWorld) {
        // (此函数保持原样)
        long nVertices = mesh->mNumVertices;
        long nTriangles = mesh->mNumFaces;
        int* vertexIndices = new int[nTriangles * 3];
        Point3f* P = new Point3f[nVertices];
        Vector3f* S = nullptr;
        Normal3f* N = new Normal3f[nVertices];
        Point2f* uv = new Point2f[nVertices];
        int* faceIndices = nullptr;
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            P[i].x = mesh->mVertices[i].x;
            P[i].y = mesh->mVertices[i].y;
            P[i].z = mesh->mVertices[i].z;
            if (mesh->HasNormals()) {
                N[i].x = mesh->mNormals[i].x;
                N[i].y = mesh->mNormals[i].y;
                N[i].z = mesh->mNormals[i].z;
            }
            if (mesh->mTextureCoords[0]) {
                uv[i].x = mesh->mTextureCoords[0][i].x;
                uv[i].y = mesh->mTextureCoords[0][i].y;
            }
        }
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++)
                vertexIndices[3 * i + j] = face.mIndices[j];
        }
        if (!mesh->HasNormals()) {
            delete[] N;
            N = nullptr;
        }
        if (!mesh->mTextureCoords[0]) {
            delete[] uv;
            uv = nullptr;
        }
        std::shared_ptr<TriangleMesh> trimesh =
            std::make_shared<TriangleMesh>(ObjectToWorld, nTriangles, vertexIndices, nVertices, P, S, N, uv, faceIndices);
        std::string meshName = mesh->mName.C_Str();
        meshNames.push_back(meshName);
        diffTexName.push_back("");
        specTexName.push_back("");
        delete[] vertexIndices;
        delete[] P;
        delete[] S;
        delete[] N;
        delete[] uv;
        return trimesh;
    }
    void ModelLoad::processNode(aiNode* node, const aiScene* scene, const Transform& ObjectToWorld) {
        // (此函数保持原样)
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene, ObjectToWorld));
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            processNode(node->mChildren[i], scene, ObjectToWorld);
    }
    void ModelLoad::loadModel(std::string path, const Transform& ObjectToWorld) {
        // (此函数保持原样)
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::cerr << "ERROR: Assimp failed to load model: " << path << std::endl;
            std::cerr << "Assimp error: " << import.GetErrorString() << std::endl;
            return;
        }
        directory = path.substr(0, path.find_last_of('/'));
        std::cout << "INFO: Texture directory set to: " << directory << std::endl;
        processNode(scene->mRootNode, scene, ObjectToWorld);
    }
    inline std::shared_ptr<Material> getDiffuseMaterial(std::string filename) {
        // (此函数保持原样)
        if (filename.empty()) {
            return std::make_shared<MatteMaterial>(
                std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.5f)),
                std::make_shared<ConstantTexture<float>>(0.0f),
                std::make_shared<ConstantTexture<float>>(0.0f));
        }
        std::cout << "DEBUG: [getDiffuseMaterial] Attempting to load: " << filename << std::endl;
        std::unique_ptr<TextureMapping2D> map = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
        std::shared_ptr<Texture<Spectrum>> Kt = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(map), filename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        std::shared_ptr<Texture<float>> sigmaRed = std::make_shared<ConstantTexture<float>>(0.0f);
        std::shared_ptr<Texture<float>> bumpMap = std::make_shared<ConstantTexture<float>>(0.0f);
        return std::make_shared<MatteMaterial>(Kt, sigmaRed, bumpMap);
    }

    // ==========================================================
    // 1. 替换 getPlasticMaterial
    // ==========================================================

    /**
     * @brief 为 PBR 纹理 (Diffuse, Metalness, Roughness) 创建一个 PlasticMaterial。
     * 这是您渲染器中实现 PBR 的（手动）方式。
     */
    inline std::shared_ptr<Material> createManualPbrMaterial(
        const std::string& diffFilename,
        const std::string& metalFilename,
        const std::string& roughFilename)
    {
        // --- 1. 加载 Diffuse (Kd) ---
        std::shared_ptr<Texture<Spectrum>> plasticKd;
        if (diffFilename.empty()) {
            plasticKd = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.5f)); // 默认灰色
        }
        else {
            std::cout << "DEBUG: [createManualPbrMaterial] Loading Kd: " << diffFilename << std::endl;
            std::unique_ptr<TextureMapping2D> map1 = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            plasticKd = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(map1), diffFilename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        }

        // --- 2. 加载 Metalness (用作 Ks) ---
        std::shared_ptr<Texture<Spectrum>> plasticKr;
        if (metalFilename.empty()) {
            plasticKr = std::make_shared<ConstantTexture<Spectrum>>(Spectrum(0.0f)); // 默认非金属 (黑色)
        }
        else {
            std::cout << "DEBUG: [createManualPbrMaterial] Loading Kr (from Metal): " << metalFilename << std::endl;
            std::unique_ptr<TextureMapping2D> map2 = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            // 我们仍然将金属度加载为 Spectrum，因为 Ks 槽需要它
            plasticKr = std::make_shared<ImageTexture<RGBSpectrum, Spectrum>>(std::move(map2), metalFilename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        }

        // --- 3. 加载 Roughness (粗糙度) ---
        std::shared_ptr<Texture<float>> plasticRoughness;
        if (roughFilename.empty()) {
            // 如果没有粗糙度贴图，才回退到硬编码值
            std::cout << "DEBUG: [createManualPbrMaterial] No roughness map. Using Constant 0.8." << std::endl;
            plasticRoughness = std::make_shared<ConstantTexture<float>>(0.8f);
        }
        else {
            // 否则，加载粗糙度贴图
            // 【重要】: 这假设 ImageTexture<float, float> 可以工作 (即 MIPMap 支持 float)
            std::cout << "DEBUG: [createManualPbrMaterial] Loading Roughness: " << roughFilename << std::endl;
            std::unique_ptr<TextureMapping2D> map3 = std::make_unique<UVMapping2D>(1.f, 1.f, 0.f, 0.f);
            plasticRoughness = std::make_shared<ImageTexture<float, float>>(std::move(map3), roughFilename, false, 8.f, ImageWrap::Repeat, 1.f, false);
        }

        // --- 4. 创建材质 ---
        std::shared_ptr<Texture<float>> bumpMap = std::make_shared<ConstantTexture<float>>(0.0f);
        // remapRoughness 参数: 您的 MetalMaterial 设为 false，这里也设为 false
        return std::make_shared<PlasticMaterial>(plasticKd, plasticKr, plasticRoughness, bumpMap, false);
    }

    // (旧的 getPlasticMaterial 函数现在可以删除了，因为它被 createManualPbrMaterial 替代了)


    // ... (buildNoTextureModel 保持不变) ...
    void ModelLoad::buildNoTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
        std::vector<std::shared_ptr<Primitive>>& prims, std::shared_ptr<Material> material) {
        // (此函数保持原样)
        std::vector<std::shared_ptr<Shape>> trisObj;
        Transform tri_World2Object = Inverse(tri_Object2World);
        for (int i = 0; i < meshes.size(); i++) {
            for (int j = 0; j < meshes[i]->nTriangles; ++j) {
                std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
                trisObj.push_back(std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j));
            }
        }
        for (int i = 0; i < trisObj.size(); ++i)
            prims.push_back(std::make_shared<GeometricPrimitive>(trisObj[i], material, nullptr, mediumInterface));
        meshes.clear();
        diffTexName.clear();
        specTexName.clear();
        meshNames.clear();
        directory = "";
    }

    // ==========================================================
    // 2. 更新 buildTextureModel
    // ==========================================================
    void ModelLoad::buildTextureModel(Transform& tri_Object2World, const MediumInterface& mediumInterface,
        std::vector<std::shared_ptr<Primitive>>& prims) {

        // --- 1. 手动创建材质 ---
        std::cout << "--- Pre-loading materials ---" << std::endl;
        preloadedMaterials.clear();

        // 这现在会调用我们的新函数，并传入 _R.png 贴图
        preloadedMaterials["cloth"] = createManualPbrMaterial(
            directory + "/T_cloth_D.png", // 漫反射 (Kd)
            directory + "/T_cloth_M.png", // 金属度 (Ks)
            directory + "/T_cloth_R.png"  // 粗糙度 (Roughness)
        );

        // 这满足了您“getMetalMaterial”的请求
        preloadedMaterials["metal"] = createManualPbrMaterial(
            directory + "/T_metal_D.png",
            directory + "/T_metal_M.png",
            directory + "/T_metal_R.png"
        );

        // 【注意】: 您的文件名有 "T_swordl_D.png"，我猜是 "T_sword_D.png"
        preloadedMaterials["sword"] = createManualPbrMaterial(
            directory + "/T_sword_D.png", // (如果真的是 'l' 请修改这里)
            directory + "/T_sword_M.png", // (假设 T_sword_M.png 存在)
            directory + "/T_sword_R.png"
        );

        // 默认材质，用于未匹配的网格
        preloadedMaterials["default"] = createManualPbrMaterial("", "", "");
        std::cout << "--- Material pre-loading complete ---" << std::endl;


        std::vector<std::shared_ptr<Shape>> trisObj;
        Transform tri_World2Object = Inverse(tri_Object2World);

        // --- 2. 循环所有网格并分配材质 ---
        std::cout << "--- Assigning materials to meshes ---" << std::endl;
        for (int i = 0; i < meshes.size(); i++) {

            std::string meshName = meshNames[i];
            std::cout << "Processing Mesh (" << i << "/" << meshes.size() << "): " << meshName << std::endl;

            // --- 手动分配材质 ---
            std::shared_ptr<Material> finalMaterial = preloadedMaterials["default"];
            std::string materialName = "default";

            if (meshName.rfind("Hood", 0) == 0 ||
                meshName.rfind("Padded", 0) == 0 ||
                meshName.rfind("Dress", 0) == 0)
            {
                finalMaterial = preloadedMaterials["cloth"];
                materialName = "cloth";
            }
            else if (meshName.rfind("Merged_Sword", 0) == 0)
            {
                finalMaterial = preloadedMaterials["sword"];
                materialName = "sword";
            }
            else if (meshName.rfind("Crown", 0) == 0 ||
                meshName.rfind("Head_Mask", 0) == 0 ||
                meshName.rfind("Chest", 0) == 0 ||
                meshName.rfind("Shoulder", 0) == 0 ||
                meshName.rfind("ArmStrap", 0) == 0 ||
                meshName.rfind("Bracer", 0) == 0 ||
                meshName.rfind("Glove", 0) == 0 ||
                meshName.rfind("Belt", 0) == 0 ||
                meshName.rfind("Boot", 0) == 0)
            {
                finalMaterial = preloadedMaterials["metal"];
                materialName = "metal";
            }

            std::cout << "  -> Match: '" << materialName << "'. Assigning material." << std::endl;


            // --- 3. 创建图元 ---
            // (此部分保持不变，它现在会使用正确的 finalMaterial)
            if (mediumInterface.inside == nullptr && mediumInterface.outside == nullptr) {
                for (int j = 0; j < meshes[i]->nTriangles; ++j) {
                    std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
                    prims.push_back(std::make_shared<GeometricPrimitive>(
                        std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j),
                        finalMaterial,
                        nullptr, mediumInterface));
                }
            }
            else {
                for (int j = 0; j < meshes[i]->nTriangles; ++j) {
                    std::shared_ptr<TriangleMesh> meshPtr = meshes[i];
                    prims.push_back(std::make_shared<GeometricPrimitive>(
                        std::make_shared<Triangle>(&tri_Object2World, &tri_World2Object, false, meshPtr, j),
                        nullptr, nullptr, mediumInterface));
                }
            }
        }

        // --- 4. 清理 ---
        meshes.clear();
        diffTexName.clear();
        specTexName.clear();
        meshNames.clear();
        preloadedMaterials.clear();
        directory = "";
    }

} // namespace PBR