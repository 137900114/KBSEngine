#include "ModelManager.h"
#include "tinyobjloader.h"
#include "graphic.h"

#include "TextureManager.h"
#include "DescriptorAllocator.h"
#include "GraphicUtil.h"


Game::Vector3 Tangent(
	Game::Vector3 p0,Game::Vector2 uv0,
	Game::Vector3 p1,Game::Vector2 uv1,
	Game::Vector3 p2,Game::Vector2 uv2
) {
	Game::Vector3 l01 = p1 - p0;
	Game::Vector3 l02 = p2 - p0;
	Game::Vector3 duv1 = uv1 - uv0;
	Game::Vector3 duv2 = uv2 - uv0;

	float det = duv1.x * duv2.y - duv2.x * duv1.y;
	float ul = duv2.y / det, vl = -duv2.x / det;

	return Game::normalize(l01 * ul + l02 * vl);
}


Model* ModelManager::loadInOBJFormat(const char* pathName,const char* name,UploadBatch* batch) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string pathNameStr = std::string(pathName);
	size_t dir_index = pathNameStr.find_last_of('/');
	std::string dirName = pathNameStr.substr(0, dir_index);

	std::string warn;
	std::string err;
	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, pathName,
		dirName.c_str(), true);
	if(!warn.empty())
		OUTPUT_DEBUG_STRING(("obj parser warning :" + warn + "\n").c_str());
	if(!err.empty())
		OUTPUT_DEBUG_STRING(("obj parser error:" + err + "\n").c_str());
	if (!ret) {
		OUTPUT_DEBUG_STRING(("fail to load obj file:" + std::string(pathName) + "\n").c_str());

		return nullptr;
	}
	
	std::unique_ptr<Model> model = std::make_unique<Model>(name);
	
	
	for (auto& shape : shapes) {
		size_t vindex_start = shape.mesh.indices[0].vertex_index, vindex_end = vindex_start;
		for (auto& index : shape.mesh.indices) {
			if (index.vertex_index < vindex_start) vindex_start = index.vertex_index;
			if (index.vertex_index > vindex_end) vindex_end = index.vertex_index;
		}

		size_t vindex_num = vindex_end - vindex_start + 1;
		std::vector<MeshVertexNormal> vertexs(vindex_num);
		std::vector<size_t> vertexVisitedTimes(vindex_num,0);
		std::vector<uint16_t> indices(shape.mesh.indices.size());
		size_t icounter = 0;
		tinyobj::index_t triangle[3];
		for (auto& index : shape.mesh.indices) {
			vertexs[index.vertex_index - vindex_start].Position = Game::Vector3(&attrib.vertices[index.vertex_index * 3]);
			vertexs[index.vertex_index - vindex_start].TexCoord = Game::Vector2(&attrib.texcoords[index.texcoord_index * 2]);
			vertexs[index.vertex_index - vindex_start].Normal = Game::Vector3(&attrib.normals[index.normal_index * 3]);
			indices[icounter] = index.vertex_index - vindex_start;

			triangle[icounter % 3] = index;
			icounter++;
			if (icounter % 3 == 0) {
				Game::Vector3 tangent = Tangent(
					Game::Vector3(&attrib.vertices[triangle[0].vertex_index * 3]),
					Game::Vector2(&attrib.texcoords[triangle[0].texcoord_index * 2]),
					Game::Vector3(&attrib.vertices[triangle[1].vertex_index * 3]),
					Game::Vector2(&attrib.texcoords[triangle[1].texcoord_index * 2]),
					Game::Vector3(&attrib.vertices[triangle[2].vertex_index * 3]),
					Game::Vector2(&attrib.texcoords[triangle[2].texcoord_index * 2])
				);
				vertexs[triangle[0].vertex_index - vindex_start].Tangent += tangent;
				vertexVisitedTimes[triangle[0].vertex_index - vindex_start]++;
				vertexs[triangle[1].vertex_index - vindex_start].Tangent += tangent;
				vertexVisitedTimes[triangle[1].vertex_index - vindex_start]++;
				vertexs[triangle[2].vertex_index - vindex_start].Tangent += tangent;
				vertexVisitedTimes[triangle[2].vertex_index - vindex_start]++;
			}
		}

		for (size_t i = 0; i != vertexVisitedTimes.size(); i++) {
			if(vertexVisitedTimes[i] != 0)
				vertexs[i].Tangent = Game::normalize(vertexs[i].Tangent / (float)vertexVisitedTimes[i]);
		}

		ID3D12Device* mDevice = gGraphic.GetDevice();
		SubMesh* submesh = new SubMesh(shape.name.c_str(),
			shape.mesh.material_ids[0], mDevice, indices.size(), indices.data(),
			vertexs.size(), vertexs.data(), batch);

		model->PushBackSubMesh(submesh);
	}
	for (auto& material : materials) {
		SubMeshMaterial sbMaterial;
		Texture* bumpMap = nullptr, *diffuseMap = nullptr, *specularMap = nullptr,
			* metalicMap = nullptr,* roughnessMap = nullptr,* emissionMap = nullptr;
		
		if (!material.bump_texname.empty()) {
			std::wstring fullname = String2WString(dirName + "/" + material.bump_texname);
			bumpMap = gTextureManager.loadTexture(fullname.c_str(), fullname.c_str(), true, batch);
			bumpMap->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		if (!material.diffuse_texname.empty()) {
			std::wstring fullname = String2WString(dirName + "/" + material.diffuse_texname);
			diffuseMap = gTextureManager.loadTexture(fullname.c_str(), fullname.c_str(), true, batch);
			diffuseMap->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		if (!material.specular_texname.empty()) {
			std::wstring fullname = String2WString(dirName + "/" + material.specular_texname);
			specularMap = gTextureManager.loadTexture(fullname.c_str(), fullname.c_str(), true, batch);
			specularMap->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		if (!material.metallic_texname.empty()) {
			std::wstring fullname = String2WString(dirName + "/" + material.metallic_texname);
			metalicMap = gTextureManager.loadTexture(fullname.c_str(), fullname.c_str(), true, batch);
			metalicMap->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		if (!material.roughness_texname.empty()) {
			std::wstring fullname = String2WString(dirName + "/" + material.roughness_texname);
			roughnessMap = gTextureManager.loadTexture(fullname.c_str(), fullname.c_str(), true, batch);
			roughnessMap->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		if (!material.emissive_texname.empty()) {
			std::wstring fullname = String2WString(dirName + "/" + material.emissive_texname);
			emissionMap = gTextureManager.loadTexture(fullname.c_str(), fullname.c_str(), true, batch);
			emissionMap->CreateShaderResourceView(gDescriptorAllocator.AllocateDescriptor());
		}
		sbMaterial.diffuse = Game::Vector3(material.diffuse);
		sbMaterial.specular = Game::Vector3(material.specular);
		sbMaterial.roughness = 1. - material.shininess / 70.;
		sbMaterial.metallic = material.metallic;
		sbMaterial.emissionScale = Game::Vector3(material.emission);

		sbMaterial.textures[SUBMESH_MATERIAL_TYPE_BUMP] = bumpMap;
		sbMaterial.textures[SUBMESH_MATERIAL_TYPE_DIFFUSE] = diffuseMap;
		sbMaterial.textures[SUBMESH_MATERIAL_TYPE_SPECULAR] = specularMap;
		sbMaterial.textures[SUBMESH_MATERIAL_TYPE_METALNESS] = metalicMap;
		sbMaterial.textures[SUBEMSH_MATERIAL_TYPE_ROUGHNESS] = roughnessMap;
		sbMaterial.textures[SUBMESH_MATERIAL_TYPE_EMISSION] = emissionMap;

		model->PushBackSubMeshMaterial(sbMaterial);
	}

	Model* modelPtr = model.get();
	modelsByPath[pathName] = std::move(model);
	modelsByName[name] = modelPtr;

	return modelPtr;
}

#include <unordered_set>

static bool supportedByAssimp(const char* extName) {
	static std::unordered_set<std::string> extNames = {
		".dae",".xml",".blend",".bvh",".3ds",".ase",
		".glFT",".ply",".dxf",".ifc",".nff",".smd",
		".vta",".mdl",".md2",".md3",".pk3",".mdc",".md5anim",
		".md5camera",".x",".q3o",".q3s",".raw",".ac",".stl",
		".dxf",".irrmesh",".irr",".off",".ter",".mdl",".hmp",
		".mesh.xml",".skeleton.xml",".material",".ms3d",".lwo",
		".lws",".lxo",".csm",".cob",".scn"
	};

	if (extNames.find(extName) == extNames.end()) {
		return true;
	}
	return false;
}


Model* ModelManager::loadModel(const char* pathName, const char* name,UploadBatch* batch) {
	if (auto model = getModelByPath(pathName);model != nullptr) {
		return model;
	}
	if (auto model = getModelByName(name);model != nullptr) {
		return nullptr;
	}

	std::string pathNameStr(pathName);
	size_t index_of_ext = pathNameStr.find_last_of('.');
	if (index_of_ext >= pathNameStr.size()) {
		return nullptr;
	}
	std::string pathExtName = pathNameStr.substr(index_of_ext, pathNameStr.size() - index_of_ext);

	if (pathExtName == ".obj") {
		if (batch != nullptr) {
			return loadInOBJFormat(pathName, name, batch);
		}
		else {
			UploadBatch mbatch = UploadBatch::Begin();
			Model* model = loadInOBJFormat(pathName, name, &mbatch);
			mbatch.End();
			return model;
		}
	}
	if (supportedByAssimp(pathExtName.c_str())) {
		if (batch != nullptr) {
			return loadByAssimp(pathName, name, batch);
		}
		else {
			UploadBatch mbatch = UploadBatch::Begin();
			Model* model = loadByAssimp(pathName, name, &mbatch);
			mbatch.End();
			return model;
		}
	}
	return nullptr;
}

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#pragma comment(lib,"assimp-vc140-mt.lib")


void processAiNode(Model* model,aiNode* node,const aiScene* scene,
		UploadBatch* batch) {
	for (size_t i = 0; i != node->mNumMeshes;i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		std::vector<MeshVertexNormal> vertices;
		std::vector<uint16_t>		  indices;
		SubMeshMaterial			      material;

		vertices.resize(mesh->mNumVertices);
		for (size_t j = 0; j != mesh->mNumVertices;j++) {
			MeshVertexNormal v;
			v.Position = Game::Vector3(mesh->mVertices[j].x,mesh->mVertices[j].y,
				mesh->mVertices[j].z);
			v.Normal = Game::Vector3(mesh->mNormals[j].x, mesh->mNormals[j].y,
				mesh->mNormals[j].z);
			if (mesh->mTangents == nullptr) {
				v.Tangent = Game::Vector3(0., 1., 0.);
			}
			else {
				v.Tangent = Game::Vector3(mesh->mTangents[j].x, mesh->mTangents[j].y, 
					mesh->mTangents[j].z );
			}
			if (mesh->mTextureCoords[0] == nullptr) {
				v.TexCoord = Game::Vector2(0., 0.);
			}
			else {
				v.TexCoord = Game::Vector2(mesh->mTextureCoords[0][j].x
				,mesh->mTextureCoords[0][j].y);
			}
			vertices[j] = v;
		}

		for (size_t j = 0; j != mesh->mNumFaces;j++) {
			aiFace face = mesh->mFaces[j];
			for (size_t k = 0; k != face.mNumIndices;k++) {
				indices.push_back(face.mIndices[k]);
			}
		}
		
		SubMesh* submesh = new SubMesh(mesh->mName.C_Str(), mesh->mMaterialIndex,
			gGraphic.GetDevice(), indices.size(), indices.data(), vertices.size(),
			vertices.data(), batch);

		model->PushBackSubMesh(submesh);
	}
	for (size_t i = 0; i != node->mNumChildren;i++) {
		processAiNode(model, node->mChildren[i], scene,
			batch);
	}
}


Model* ModelManager::loadByAssimp(const char* pathName, const char* name, UploadBatch* batch) {
	Assimp::Importer imp;
	const aiScene* scene = imp.ReadFile(pathName,
		aiProcess_GenNormals |
		aiProcess_Triangulate |
		aiProcess_CalcTangentSpace
	);

	std::string pathNameStr = std::string(pathName);
	size_t dir_index = pathNameStr.find_last_of('/');
	std::string dirName = pathNameStr.substr(0, dir_index);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene->mRootNode == nullptr) {
		std::string msg = "fail to load model file " + std::string(pathName) + "reason : " + 
			imp.GetErrorString();
		OUTPUT_DEBUG_STRING(msg.c_str());
		return nullptr;
	}
	
	std::unique_ptr<Model> model = std::make_unique<Model>(name);
	
	for (size_t i = 0; i != scene->mNumMaterials;i++) {

		aiMaterial* mat = scene->mMaterials[i];
		SubMeshMaterial material;

		auto loadTexture = [&](aiTextureType type, SUBMESH_MATERIAL_TYPE smType) {
			if (mat->GetTextureCount(type) != 0) {
				aiString str;
				mat->GetTexture(type, 0, &str);
				std::wstring texPath = String2WString(dirName) + String2WString(str.C_Str());
				Texture* tex = gTextureManager.loadTexture(texPath.c_str(), texPath.c_str(),
					true, batch);
				if (tex == nullptr || !tex->IsValid()) {
					std::wstring msg = L"fail to load texture " + texPath;
					OUTPUT_DEBUG_STRINGW(msg.c_str());
					material.textures[smType] = nullptr;
				}
				else {
					material.textures[smType] = tex;
				}
			}
		};
		loadTexture(aiTextureType_DIFFUSE, SUBMESH_MATERIAL_TYPE_DIFFUSE);
		loadTexture(aiTextureType_NORMALS, SUBMESH_MATERIAL_TYPE_BUMP);
		loadTexture(aiTextureType_SPECULAR, SUBMESH_MATERIAL_TYPE_SPECULAR);
		loadTexture(aiTextureType_SHININESS, SUBEMSH_MATERIAL_TYPE_ROUGHNESS);
		loadTexture(aiTextureType_UNKNOWN, SUBMESH_MATERIAL_TYPE_METALNESS);
		loadTexture(aiTextureType_DIFFUSE, SUBMESH_MATERIAL_TYPE_DIFFUSE);

		model->PushBackSubMeshMaterial(material);
	}

	Model* modelPtr = model.get();
	modelsByName[name] = modelPtr;
	modelsByPath[pathName] = std::move(model);

	return modelPtr;
}


