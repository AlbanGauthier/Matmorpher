#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

#include <string>
#include <map>
#include <iostream>
#include <tuple>

#include <QFileInfo>

#include <glad/glad.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Model.h"
#include "Utils/tangent_space.h"

size_t compSizeFromType(int comp)
{
    switch (comp)
    {
    case TINYGLTF_PARAMETER_TYPE_BYTE:
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
        return sizeof(char);

    case TINYGLTF_PARAMETER_TYPE_SHORT:
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
        return sizeof(short);

    case TINYGLTF_PARAMETER_TYPE_INT:
    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
        return sizeof(int);
    case TINYGLTF_PARAMETER_TYPE_FLOAT:
        return sizeof(float);
    default:
        std::cerr << "Error: unknown component type" << std::endl;
        exit(1);
    }
}

template <typename T1, typename T2>
std::vector<T1> getData(const tinygltf::Model &model, int acc)
{
    size_t stride =
        model.bufferViews[model.accessors[acc].bufferView]
            .byteStride;
    const unsigned char *data =
        model
            .buffers
                [model
                     .bufferViews[model.accessors[acc].bufferView]
                     .buffer]
            .data.data();
    data += model.accessors[acc].byteOffset;
    data += model.bufferViews[model.accessors[acc].bufferView]
                .byteOffset;

    size_t count = model.accessors[acc].count;
    if (stride == 0)
        stride = sizeof(T2);
    std::vector<T1> vec;
    for (int i = 0; i < count; i++)
    {
        vec.push_back(((T2 *)data)[0]);
        data += stride;
    }
    return vec;
}

std::unique_ptr<Model> Model::load(const char* filename)
{
	QFileInfo path = QFileInfo(filename);
	auto ext = path.completeSuffix();
	if (ext == "glb" && ext == "gltf")
	{
		std::cerr << "Error: unsupported file extension " << ext.toStdString();
		return std::unique_ptr<Model>();
	}

	tinygltf::Model gltf_model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;
	bool ret;
	if (ext == ".glb")
		ret = loader.LoadBinaryFromFile(&gltf_model, &err, &warn, std::string(filename));
	else
		ret = loader.LoadASCIIFromFile(&gltf_model, &err, &warn, std::string(filename));

	if (!warn.empty())
	{
		std::cerr << "Warning when loading file " << filename;
		std::cerr << warn.c_str();
	}

	if (!err.empty())
	{
		std::cerr << "Error when loading file " << filename;
		std::cerr << err.c_str();
	}

	if (!ret)
		return std::unique_ptr<Model>();

	std::unique_ptr<Model> model = std::unique_ptr<Model>(new Model);
	model->filename_ = filename;

	// Load Buffers
	for (const auto& bv : gltf_model.bufferViews)
	{
		const auto& vec = gltf_model.buffers[bv.buffer].data;
		unsigned int vbo;
		glGenBuffers(1, &vbo);
		glBindBuffer(bv.target, vbo);
		glBufferData(bv.target, bv.byteLength, vec.data() + bv.byteOffset, GL_STATIC_DRAW);
		model->buffers_.push_back(vbo);
	}

	//Load Materials
	model->materials_.reserve(gltf_model.materials.size());
	for (const auto& m : gltf_model.materials)
	{
		model->materials_.resize(model->materials_.size() + 1);
		auto& mat = model->materials_.back();
		mat.name = m.name;

		auto it = m.values.find("baseColorTexture");
		if (it != m.values.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			auto& t = gltf_model.textures[val.TextureIndex()];
			auto& img = gltf_model.images[t.source];
			mat.color_map_filename = img.uri;

			const unsigned char* mydata = img.image.data();

			// Ignore Texture.fomat.
			GLenum format = GL_RGBA;
			if (img.component == 3) {
				format = GL_RGB;
			}

			glGenTextures(1, &mat.color_map);
			glBindTexture(GL_TEXTURE_2D, mat.color_map);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, img.width, img.height, 0, format, img.pixel_type, mydata);
			glGenerateMipmap(GL_TEXTURE_2D);

			if (t.sampler != -1)
			{
				auto& sampler = gltf_model.samplers[t.sampler];

				// set the texture wrapping/filtering options (on the currently bound texture object)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
			}
		}

		it = m.values.find("metallicRoughnessTexture");
		if (it != m.values.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			auto& t = gltf_model.textures[val.TextureIndex()];
			auto& img = gltf_model.images[t.source];
			mat.metallic_roughness_map_filename = img.uri;

			const unsigned char* mydata = img.image.data();
			// Ignore Texture.fomat.
			GLenum format = GL_RGBA;
			if (img.component == 3) {
				format = GL_RGB;
			}

			glGenTextures(1, &mat.metallic_roughness_map);
			glBindTexture(GL_TEXTURE_2D, mat.metallic_roughness_map);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, format, img.pixel_type, mydata);
			glGenerateMipmap(GL_TEXTURE_2D);

			if (t.sampler != -1)
			{
				auto& sampler = gltf_model.samplers[t.sampler];

				// set the texture wrapping/filtering options (on the currently bound texture object)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
			}
		}

		it = m.additionalValues.find("heightTexture");
		if (it != m.additionalValues.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			auto& t = gltf_model.textures[val.TextureIndex()];
			auto& img = gltf_model.images[t.source];
			mat.height_map_filename = img.uri;

			const unsigned char* mydata = img.image.data();
			GLenum format = GL_RGBA;
			if (img.component == 3) {
				format = GL_RGB;
			}

			glGenTextures(1, &mat.height_map);
			glBindTexture(GL_TEXTURE_2D, mat.height_map);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, format, img.pixel_type, mydata);
			glGenerateMipmap(GL_TEXTURE_2D);

			if (t.sampler != -1)
			{
				auto& sampler = gltf_model.samplers[t.sampler];

				// set the texture wrapping/filtering options (on the currently bound texture object)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
			}
		}

		it = m.values.find("baseColorFactor");
		if (it != m.values.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			auto factor = val.ColorFactor();
			mat.base_color_factor = glm::vec4(factor[0], factor[1], factor[2], factor[3]);
		}

		it = m.values.find("metallicFactor");
		if (it != m.values.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			mat.metallic_roughness_values = glm::vec2(val.Factor(), mat.metallic_roughness_values.y);
		}

		it = m.values.find("roughnessFactor");
		if (it != m.values.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			mat.metallic_roughness_values = glm::vec2(mat.metallic_roughness_values.y, val.Factor());
		}

		it = m.additionalValues.find("normalTexture");
		if (it != m.additionalValues.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			auto& t = gltf_model.textures[val.TextureIndex()];
			auto& img = gltf_model.images[t.source];
			mat.normal_map_filename = img.uri;

			const unsigned char* mydata = img.image.data();
			GLenum format = GL_RGBA;
			if (img.component == 3) {
				format = GL_RGB;
			}

			glGenTextures(1, &mat.normal_map);
			glBindTexture(GL_TEXTURE_2D, mat.normal_map);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, format, img.pixel_type, mydata);
			glGenerateMipmap(GL_TEXTURE_2D);

			if (t.sampler != -1)
			{
				auto& sampler = gltf_model.samplers[t.sampler];

				// set the texture wrapping/filtering options (on the currently bound texture object)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
			}
		}

		it = m.additionalValues.find("occlusionTexture");
		if (it != m.additionalValues.end())
		{
			mat.has_occlusion_map = true;
		}

		/* it = m.additionalValues.find("emissiveTexture");
		if (it != m.additionalValues.end())
		{
			auto& name = it->first;
			auto& val = it->second;

			auto& t = gltf_model.textures[val.TextureIndex()];
			auto& img = gltf_model.images[t.source];
			mat.emissive_map_filename = img.uri;

			const unsigned char* mydata = img.image.data();
			GLenum format = GL_RGBA;
			if (img.component == 3) {
				format = GL_RGB;
			}

			glGenTextures(1, &mat.emissive_map);
			glBindTexture(GL_TEXTURE_2D, mat.emissive_map);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, format, img.pixel_type, mydata);
			glGenerateMipmap(GL_TEXTURE_2D);

			if (t.sampler != -1)
			{
				auto& sampler = gltf_model.samplers[t.sampler];

				// set the texture wrapping/filtering options (on the currently bound texture object)
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, sampler.wrapS);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, sampler.wrapT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampler.minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampler.magFilter);
			}

			it = m.additionalValues.find("emissiveFactor");
			if (it != m.additionalValues.end())
			{
				auto& name = it->first;
				auto& val = it->second;

				auto factor = val.ColorFactor();
				mat.emissive_factor = glm::vec4(factor[0], factor[1], factor[2], 0.0f);
			}
		} */

	}

	// Load Meshes
	std::vector<int> primitive_offsets;
	for (const auto& m : gltf_model.meshes)
	{
		primitive_offsets.push_back(static_cast<int>(model->primitives_.size()));
		int i = 0;
		for (const auto& p : m.primitives)
		{
			Primitive primitive;
			primitive.name = m.name + "_" + std::to_string(i);
			GLuint vao = GLuint(model->vao_buffers_.size());
			primitive.vao_id = vao;
			glGenVertexArrays(1, &primitive.vao_id);
			glBindVertexArray(primitive.vao_id);

			model->vao_buffers_.push_back(primitive.vao_id);

			//QOpenGLVertexArrayObject &vao = *model->vao_buffers_.back();
			//vao.create();
			//vao.bind();

			for (const auto& iter : p.attributes)
			{
				auto& name = iter.first;
				auto& a = iter.second;

				int attrib_index = -1;
				for (auto& iter2 : attribute_mapping) { //[attr_name, id, dum]
					auto& attr_name = std::get<0>(iter2);
					auto& id = std::get<1>(iter2);
					if (name.compare(attr_name) == 0)
					{
						attrib_index = id;
						break;
					}
				}
				if (attrib_index != -1)
				{
					const auto& acc = gltf_model.accessors[a];
					const auto& bv = gltf_model.bufferViews[acc.bufferView];

					glVertexArrayVertexBuffer(primitive.vao_id,
						attrib_index,
						model->buffers_[acc.bufferView],
						0,
						GLsizei(bv.byteStride != 0 ? bv.byteStride : acc.type * compSizeFromType(acc.componentType)));
					glVertexArrayAttribFormat(primitive.vao_id,
						attrib_index,
						acc.type,
						acc.componentType,
						acc.normalized,
						GLuint(acc.byteOffset));

					//if it is a position attributes
					if (attrib_index == 0)
					{
						primitive.aabb.extend(glm::vec3(acc.minValues[0], acc.minValues[1], acc.minValues[2]));
						primitive.aabb.extend(glm::vec3(acc.maxValues[0], acc.maxValues[1], acc.maxValues[2]));
					}
				}
			}
			// If mesh does not have tangents, compute then using mikktspace
			if (!p.attributes.count("TANGENT"))
			{
				std::vector<unsigned int> faces;
				if (gltf_model.accessors[p.indices].componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					faces = getData<unsigned int, unsigned char>(gltf_model, p.indices);
					auto face_buf = getData<unsigned char, unsigned char>(gltf_model, p.indices);
				}
				else if (gltf_model.accessors[p.indices].componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					faces = getData<unsigned int, unsigned short>(gltf_model, p.indices);
					auto face_buf = getData<unsigned short, unsigned short>(gltf_model, p.indices);
				}
				else if (gltf_model.accessors[p.indices].componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					faces = getData<unsigned int, unsigned int>(gltf_model, p.indices);
					auto face_buf = getData<unsigned int, unsigned int>(gltf_model, p.indices);
				}
				std::vector<glm::vec3> pos = getData<glm::vec3, glm::vec3>(gltf_model, p.attributes.at("POSITION"));
				std::vector<glm::vec3> nor = getData<glm::vec3, glm::vec3>(gltf_model, p.attributes.at("NORMAL"));
				std::vector<glm::vec2> tex = getData<glm::vec2, glm::vec2>(gltf_model, p.attributes.at("TEXCOORD_0"));
				std::vector<glm::vec4> tangents = genMikkTSpace(pos, nor, tex, faces);

				unsigned int vbo = 0;
				glGenBuffers(1, &vbo);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, sizeof(tangents[0]) * tangents.size(), tangents.data(), GL_STATIC_DRAW);

				model->buffers_.push_back(vbo);

				glVertexArrayVertexBuffer(primitive.vao_id,
					3,
					vbo,
					0,
					sizeof(tangents[0]));
				glVertexArrayAttribFormat(primitive.vao_id,
					3,
					4,
					GL_FLOAT,
					false,
					0);
			}

			const auto& acc = gltf_model.accessors[p.indices];

			glVertexArrayElementBuffer(primitive.vao_id, model->buffers_[acc.bufferView]);

			primitive.material_id = p.material;
			primitive.mode = p.mode;
			primitive.count = acc.count;
			primitive.component_type = acc.componentType;
			primitive.offset = acc.byteOffset;
			model->primitives_.push_back(primitive);
			i++;
		}
	}

	// Load Nodes
	model->nodes_.reserve(gltf_model.nodes.size());
	int i = 0;
	for (const auto& n : gltf_model.nodes)
	{
		Node node;
		if (n.name.empty())
			node.name = "Node " + std::to_string(i);
		else
			node.name = n.name;
		node.children = n.children;
		if (n.matrix.size() == 16)
		{
			std::vector<float> fvec;
			for (float d : n.matrix) {
				fvec.push_back(d);
			}
			node.transform *= glm::make_mat4(fvec.data());
		}
		else
		{
			if (n.translation.size() == 3)
			{
				node.transform = glm::translate(node.transform, glm::vec3(n.translation[0], n.translation[1], n.translation[2]));
			}
			if (n.rotation.size() == 4)
			{
				node.transform = glm::toMat4(glm::quat(n.rotation[3], n.rotation[0], n.rotation[1], n.rotation[2])) * node.transform;
			}
			if (n.scale.size() == 3)
			{
				node.transform = glm::scale(node.transform, glm::vec3(n.scale[0], n.scale[1], n.scale[2]));
			}
		}
		if (n.mesh != -1)
		{
			node.primitive_number = int(gltf_model.meshes[n.mesh].primitives.size());
			node.first_primitive = primitive_offsets[n.mesh];
		}
		model->nodes_.push_back(node);
		i++;
	}

	// Load Scenes
	model->scenes_.reserve(gltf_model.scenes.size());
	i = 0;
	for (const auto& s : gltf_model.scenes)
	{
		Scene scene;
		if (i == 0)
			scene.visible = true;
		if (s.name.empty())
			scene.name = "Scene " + std::to_string(i);
		else
			scene.name = s.name;
		for (auto& n : s.nodes)
			scene.nodes.push_back(n);
		model->scenes_.push_back(scene);
		i++;
	}

	return model;
}

void Model::draw(const Shader &shader, const glm::mat4 &transform, Mode mode) const
{
    for (const auto &s : scenes_)
        if (s.visible)
            for (const auto &n : s.nodes)
                drawNode(nodes_[n], shader, transform, mode);
}

void Model::drawScene(int s, const Shader& shader, const glm::mat4 &transform, Mode mode) const
{
    if (scenes_.size() > s)
        for (const auto &n : scenes_[s].nodes)
            drawNode(nodes_[n], shader, transform, mode);
}

void Model::drawNode(const Node &node, const Shader& shader, const glm::mat4 &transform, Mode mode) const
{
    if (!node.visible)
        return;

    glm::mat4 node_trans = transform * node.transform;
    for (const auto &c : node.children)
        drawNode(nodes_[c], shader, node_trans, mode);

    if (node.first_primitive != -1)
        for (int i = 0; i < node.primitive_number; i++)
            if (primitives_[size_t(node.first_primitive) + i].visible)
				shader.drawPrimitive(*this, primitives_[size_t(node.first_primitive) + i], node_trans, mode);
}

AlignedBox Model::sceneAlignedBox(int s)
{
    AlignedBox aabb;
    if (scenes_.size() > s)
        for (const auto &n : scenes_[s].nodes)
            aabb.extend(nodeAlignedBox(nodes_[n], glm::mat4(1.0)));

    return aabb;
}

AlignedBox Model::nodeAlignedBox(const Node &node, const glm::mat4 &transform)
{
    if (!node.visible)
        return AlignedBox();

    glm::mat4 node_trans = transform * node.transform;
    AlignedBox aabb;
    for (const auto &c : node.children)
        aabb.extend(nodeAlignedBox(nodes_[c], transform));

    if (node.first_primitive != -1)
        for (int i = 0; i < node.primitive_number; i++)
            aabb.extend(primitives_[size_t(node.first_primitive) + i].aabb.transform(node_trans));

    return aabb;
}