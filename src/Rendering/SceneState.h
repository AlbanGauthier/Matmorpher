#pragma once

#include "Utils/Camera.h"

enum class DebugViewMode {
	PBRView		= 0,
	BaseColor	= 1,
	Height		= 2,
	Metallic	= 3,
	Normal		= 4,
	Roughness	= 5,
	Detail		= 6,
	GeomNormal	= 7,
	SSAO		= 8
};

enum class AlbedoInterpolation {
	Linear			= 1,
	//TextureDesign	= 2,
	Gaussianized	= 3
};

enum class RenderedMesh {
	Plane	= 0,
	Sphere	= 1,
};

struct SceneState
{
    //Viewport data
    float width = 0;
    float height = 0;
	float zNear = 0.01f;
	float zFar = 50.f;
	std::vector<int> viewport = { 0, 0, 0, 0 }; //for screenshots

	DebugViewMode debug_view_mode		= DebugViewMode::PBRView;
	RenderedMesh renderedMesh			= RenderedMesh::Sphere;
	AlbedoInterpolation albedo_interp	= AlbedoInterpolation::Gaussianized;

	bool ycbcr					= false;
	bool dark_background		= true;
	bool wireframe				= false;

	// Interpolation parameters
	float global_interpolation	= 0.0f;
	float mix_color				= 0.f;
	float mix_roughness			= 0.f;
	float mix_metallic			= 0.f;
	float mix_height			= 0.f;
	float mix_normal			= 0.f;
	
	unsigned int triangle_count = 0;
	uint64_t timequery			= 0;

	std::string mat1_name		= "none";
	std::string mat1_path		= "";
	std::string mat2_name		= "none";
	std::string mat2_path		= "";
	std::string warp_grid		= "none";

	//HeightMap parameters
	float height_factor = 0.15f;
	float comp_hf_1 = 0.0f;
	float comp_hf_2 = 0.0f;
	int tess_level = 48;

	float ssao_radius = 0.08f;

    // Global transform
	glm::mat4 transform = glm::mat4(1.0f);
    TurntableCamera camera = TurntableCamera();
};
