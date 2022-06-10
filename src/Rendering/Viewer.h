#pragma once

#include <glad/glad.h> // must be the first include

#include <QString>

#include "Rendering/Shader.h"
#include "Rendering/SceneState.h"
#include "Rendering/Mesh.h"
#include "Utils/Camera.h"
#include "Warpgrid/Warpgrid.h"

class ViewerWidget;

enum class TextureType {
	AlbedoMap,
	HeightMap,
	MetallicMap,
	NormalMap,
	RoughnessMap
};

class Viewer
{
public:
	Viewer(SceneState& scene_state);
	~Viewer();

	void render() const;
	void resize(int width, int height);

	void loadMaterial(string filepath, int mat_idx);
	void loadWarpGrid(string filename);

	void initializeInterpolatedLUT();
	void reloadGaussianTextures();

	void setupMesh();

	void captureScreenshot();
	void reloadShaders(int i);

	const char* createScreenshotBasename();

private:
	void updateProjectionMat();
	void setupIrradianceMapAndBrdfLut();

	void draw() const;
	void drawViews(const Shader&) const;
	void initShaderVariables(const Shader&) const;

	bool setupTexture(int, const char *, TextureType);
	void loadWarpGridOnGPU();

	void initAndLoadGaussianizedTexture(GLuint textureLayoutIdx, GLuint inv_cdf_layout, const std::string& mat_path);
	void loadGaussianTexture(GLuint gaussian_texture_layout_idx, GLuint inv_cdf_layout, const std::string& mat_path);

	void computeNormalFromHeight(int mat_id, const std::string& mat_path);

	//Shader m_otmapShader;
	Shader m_normalComputeShader;
	Shader m_histogramComputeShader;
	Shader m_customShader;
	Shader m_objShader;

	// Textures
	std::vector<GLuint> pbrTextureGLIndex;
	GLuint irradianceMapGLIndex = 0;
	GLuint dfgLUTGLIndex = 0;

	// GLSL Layout
	GLuint warpgrid_layout	 = 10;
	GLuint gaussian1_layout  = 11;
	GLuint gaussian2_layout  = 12;
	GLuint inv_cdf1_layout	 = 13;
	GLuint inv_cdf2_layout	 = 14;
	GLuint ssao_layout		 = 15;
	
	int texture_layout_idx = 0;
	
	// regular grid for otmap interpolation
	unsigned int gridVAO = 0, gridVBO = 0, gridEBO = 0;
	std::vector<unsigned int> gridTriangles;
	std::unique_ptr<Mesh> g_mesh;

	// reference to ViewerWidget::scene_state
	SceneState& scene_state_;
	
	GLuint primitive_query_id_ = 0;
	GLuint time_query_id_ = 0;
	GLuint triCnt = 0;

	std::unique_ptr<Warpgrid> warp_map;

	GLsizei albedo_width = 0, albedo_height = 0, albedo_nrChannels = 0;
	vector<vector<double>> albedo_img_1;
	vector<vector<double>> albedo_img_2;

	GLsizei PBRTextWidth = 0, PBRTextHeight = 0;

	////////////////// 
	/// SSAO
	//////////////////

	void initSSAO();
	void setupQuadRendering();
	void generateFramebuffers();
	void drawQuadAndSetupSSAO() const;
	void drawQuadAndBlurSSAO() const;
	
	Shader m_normalShader;
	Shader m_ssaoShader;
	Shader m_ssaoBlurShader;
	Shader m_screenShader;

	GLuint glNoiseTexture;
	std::vector<glm::vec3> ssaoNoise;
	std::vector<glm::vec3> ssaoKernel;

	// quad for texture rendering
	unsigned int quadVAO = 0, quadVBO = 0;

	// Framebuffer
	GLuint m_fbo_gbuffer;		//gbuff fbo
	GLuint m_fbo_ssao;			//SSAO fbo
	GLuint m_fbo_ssao_blur;		//SSAO Blur fbo
	
	// MSAA
	GLuint m_fbo_RENDER;		//enables MSAA HDR
	GLuint m_Texture_RENDER;	//enables MSAA HDR
	GLuint m_rbo_RENDER;		//enables MSAA HDR

	GLuint m_fbo_RESOLVE;		//HDR replaces Qt
	GLuint m_Texture_RESOLVE;	//HDR replaces Qt

	// Textures
	GLuint m_gPosition;
	GLuint m_gNormal;
	GLuint m_SSAOColorBuffer;

	// Render buffer
	GLuint m_depthrenderbuffer;
};