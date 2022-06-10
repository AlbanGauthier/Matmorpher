#pragma once

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <glm/glm.hpp>

#include "Utils/AlignedBox.h"
#include "Shader.h"

// Mapping between gltf attributes, binding points and shader inputs
constexpr std::tuple<const char*, int, const char*> attribute_mapping[] = {
	{"POSITION",	0,	"vtx_position"	},
	{"NORMAL",		1,	"vtx_normal"	},
	{"TEXCOORD_0",	2,	"vtx_texcoord"	},
	{"TANGENT",		3,	"vtx_tangent"	} };

class Shader;

// A Material represent the appearance of an object
// It is composed of a set of texture and factore representing the different properties of the material
struct Material
{
  std::string name;

  glm::vec4 base_color_factor = glm::vec4(1, 1, 1, 1);
  std::string color_map_filename;
  GLuint color_map = 0;

  glm::vec2 metallic_roughness_values = glm::vec2(1, 1);
  bool has_occlusion_map = false;
  float occlusion_strength = 1.0;
  std::string metallic_roughness_map_filename;
  GLuint metallic_roughness_map = 0;

  std::string normal_map_filename;
  GLuint normal_map = 0;

  //glm::vec3 emissive_factor = glm::vec3(1, 1, 1);
  //std::string emissive_map_filename;
  //GLuint emissive_map;

  float height_factor = 50.0f;
  std::string height_map_filename;
  GLuint height_map = 0;
};

// A Primitive represents a single geometric entity
struct Primitive
{
  std::string name;
  GLuint vao_id = 0;            // Index of the Vertex Array buffer
  GLuint material_id = 0;       // Index of the material
  GLenum mode = 0;				// Type of component (triangle,triangle fan, quad,...)
  size_t count = 0;				// Number of component
  GLenum component_type = 0;	// Type of index (unsigned int, short int, ...)
  size_t offset = 0;			// Offset of the first index in the index buffer
  AlignedBox aabb;				// Axis Aligned bounding box
  bool visible = true;
};

// A Node in the scene graph
struct Node
{
  std::string name;
  glm::mat4 transform = glm::mat4(1.0);      // Transformation matrix of this node
  std::vector<int> children; // List of children of the node
  int first_primitive = -1;  // Index of the first primitive for this node or -1 if no primitive
  int primitive_number = 0;  // Total number of primitives
  bool visible = true;
};

struct Scene
{
  std::string name;
  std::vector<int> nodes;
  bool visible = false;
};

enum class Mode;

// Represents a gltf 2.0 model
// The gltf specification can be found:
// https://github.com/KhronosGroup/glTF/blob/master/specification/2.0/README.md
struct Model
{
	Model() {};
	~Model() {
		for (unsigned int buf : buffers_)
			glDeleteBuffers(1, &buf);
		for (unsigned int vao : vao_buffers_)
			glDeleteVertexArrays(1, &vao);
		for (auto& mat : materials_) {
			glDeleteTextures(1, &mat.color_map);
			glDeleteTextures(1, &mat.metallic_roughness_map);
			glDeleteTextures(1, &mat.normal_map);
			//glDeleteTextures(1, &mat.emissive_map);
			glDeleteTextures(1, &mat.height_map);
		}

		filename_.clear();
		scenes_.clear();
		nodes_.clear();
		buffers_.clear();
		vao_buffers_.clear();
		primitives_.clear();
		materials_.clear();
	};
	std::string filename_;
	std::vector<Scene> scenes_;
	std::vector<Node> nodes_;
	//std::vector<unsigned int> vbo_buffers_;
	std::vector<unsigned int> buffers_;
	//std::vector<unsigned int> ebo_buffers_;
	std::vector<unsigned int> vao_buffers_;
	std::vector<Primitive> primitives_;
	std::vector<Material> materials_;

	static std::unique_ptr<Model> load(const char* filename);

	// Draw all visible scene using shader
	void draw(const Shader& shader, const glm::mat4& transform = glm::mat4(), Mode mode = Mode::PBR) const;

	// Draw scene s using shader
	void drawScene(int s, const Shader& shader, const glm::mat4& transform = glm::mat4(), Mode mode= Mode::PBR) const;

	void drawNode(const Node& node, const Shader& shader, const glm::mat4& transform, Mode mode) const;

	// Get bounding box of scene s
	AlignedBox sceneAlignedBox(int s);

private:
	AlignedBox nodeAlignedBox(const Node& node, const glm::mat4& transform);
};
