#ifdef _WIN32
#include <windows.h>
#else
#define APIENTRY
#endif

#include <glad/glad.h>	// must be the first include
#include <cstdlib>		// for EXIT_FAILURE and EXIT_SUCCESS
#include <cmath>
#include <random>
#include <time.h>

#include "stb_image.h"
#include "stb_image_write.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include "Window/ViewerWidget.h"
#include "Rendering/Viewer.h"
#include "Rendering/Shader.h"
#include "Rendering/Debug.h"  // for enableGlDebug()
#include "Utils/MathUtils.h"
#include "Utils/Histogram.h"
#include "Utils/NormalReorientation.h"

using std::string;
using std::cout;
using std::cerr;
using std::endl;
using std::vector;

const float ssaoKernelSize = 64.0f;

std::vector<std::string> maps_name = { 
	"color", 
	"height", 
	"metallic", 
	"normal", 
	"roughness" 
};

Viewer::Viewer(SceneState& scene_state)
	: scene_state_(scene_state)
{

#ifndef NDEBUG
	enableGlDebug();
#endif // !NDEBUG

	// Shaders
	{
		// exe in ./bin/
		m_customShader.loadShaders(vector<const char*>
		{
			"../src/shaders/matmorpher.vert",
				"../src/shaders/matmorpher.frag",
				"../src/shaders/matmorpher.tesc",
				"../src/shaders/matmorpher.tese",
				NULL
		});

		m_screenShader.loadShaders(vector<const char*>
		{
			"../src/shaders/screen.vert",
				"../src/shaders/screen.frag",
				NULL, NULL, NULL
		});

		m_normalComputeShader.loadShader("../src/shaders/normal.comp");

		m_histogramComputeShader.loadShader("../src/shaders/gaussianization.comp");
	}

	//number of control points per patch : 3 is default
	glPatchParameteri(GL_PATCH_VERTICES, 3);

	glGenQueries(1, &primitive_query_id_);
	glCreateQueries(GL_TIME_ELAPSED, 1, &time_query_id_);

	// (5 PBR + 1 Gauss) * nb_mat + warp_tex + 2 Inv_CDF + SSAO = 16
	pbrTextureGLIndex = std::vector<GLuint>(16, 0);

	initSSAO();
	setupMesh();
	setupIrradianceMapAndBrdfLut();
	initializeInterpolatedLUT();

}

Viewer::~Viewer()
{
	glDeleteQueries(1, &primitive_query_id_);
	glDeleteQueries(1, &time_query_id_);

	//Textures
	if (glIsTexture(irradianceMapGLIndex))
		glDeleteTextures(1, &irradianceMapGLIndex);
	if (glIsTexture(dfgLUTGLIndex))
		glDeleteTextures(1, &dfgLUTGLIndex);
	for (int i = 0; i < pbrTextureGLIndex.size(); i++)
		if (glIsTexture(pbrTextureGLIndex[i]))
			glDeleteTextures(1, &pbrTextureGLIndex[i]);

	//Buffers
	glDeleteBuffers(1, &gridVBO);
	glDeleteBuffers(1, &gridEBO);
	glDeleteVertexArrays(1, &gridVAO);
}

void Viewer::render() const
{
	glBeginQuery(GL_TIME_ELAPSED, time_query_id_);
	glBeginQuery(GL_PRIMITIVES_GENERATED, primitive_query_id_);

	draw();

	glEndQuery(GL_TIME_ELAPSED);
	glGetQueryObjectui64v(time_query_id_, GL_QUERY_RESULT, &scene_state_.timequery);

	glEndQuery(GL_PRIMITIVES_GENERATED);
	glGetQueryObjectuiv(primitive_query_id_, GL_QUERY_RESULT, &scene_state_.triangle_count);
}

void Viewer::initSSAO() {

	m_normalShader.loadShaders(vector<const char*> {
		"../src/shaders/matmorpher.vert",
		"../src/shaders/gbuffer.frag",
		"../src/shaders/matmorpher.tesc",
		"../src/shaders/matmorpher.tese",
			NULL});

	m_ssaoShader.loadShaders(vector<const char*>{
		"../src/shaders/ssao.vert",
		"../src/shaders/ssao.frag",
		NULL, NULL, NULL});

	m_ssaoBlurShader.loadShaders(vector<const char*> {
		"../src/shaders/ssao.vert",
		"../src/shaders/ssao_blur.frag",
		NULL, NULL, NULL});

	// generate sample kernel
	// ----------------------
	std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0);
	std::default_random_engine generator;
	for (unsigned int i = 0; i < ssaoKernelSize; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator));
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / ssaoKernelSize;

		// scale samples s.t. they're more aligned to center of kernel
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	// generate noise texture
	// ----------------------
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f); // rotate around z-axis (in tangent space)
		ssaoNoise.push_back(noise);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &glNoiseTexture);
	glTextureStorage2D(glNoiseTexture, 1, GL_RGB16F, 4, 4);
	glTextureSubImage2D(glNoiseTexture, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, &ssaoNoise[0]);

	setupQuadRendering();
}

void Viewer::draw() const
{
	// First of all, save the already bound framebuffer (by Qt)
	GLint qt_buffer;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &qt_buffer);

	{
		//////////////////////
		// 1. Normals & Positions
		//////////////////////

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_gbuffer);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		m_normalShader.use();

		GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, drawBuffers);

		drawViews(m_normalShader);
	}

	{
		//////////////////////
		// 2. render SSAO only
		//////////////////////

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_ssao);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		//bind vao
		glBindVertexArray(quadVAO);
		drawQuadAndSetupSSAO();
	}

	{
		//////////////////////
		// 3. SSAO Blur Pass
		//////////////////////

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_ssao_blur);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glBindVertexArray(quadVAO);
		drawQuadAndBlurSSAO();
	}

	{
		////////////////////////
		// 4. HDR PBR Rendering
		////////////////////////

		// render scene into floating point framebuffer
		// -----------------------------------------------

		glEnable(GL_MULTISAMPLE);

		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_RENDER);

		if (scene_state_.dark_background) glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		else glClearColor(1.f, 1.f, 1.f, 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glEnable(GL_DEPTH_TEST);

		for (int i = 0; i < pbrTextureGLIndex.size(); i++)
		{
			glBindTextureUnit(i, pbrTextureGLIndex[i]);
		}
		
		if (scene_state_.wireframe)
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		else
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glGetIntegerv(GL_VIEWPORT, scene_state_.viewport.data());

		drawViews(m_customShader);

		glDisable(GL_MULTISAMPLE);
	}

	{
		////////////////////////
		// 5. Resolve MSAA
		////////////////////////

		// Resolved multisampling
		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo_RENDER);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fbo_RESOLVE);
		glBlitFramebuffer(
			0, 0, scene_state_.width, scene_state_.height,
			0, 0, scene_state_.width, scene_state_.height,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	{
		// -----------------------------------------------
		// Render the colorbuffer from the MS FBO into 
		// original Qt framebuffer and apply Tonemapping
		// -----------------------------------------------

		glViewport(0, 0, scene_state_.width, scene_state_.height);

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		glBindFramebuffer(GL_FRAMEBUFFER, qt_buffer);

		m_screenShader.use();

		m_screenShader.setInt("debug_view_mode", static_cast<int>(scene_state_.debug_view_mode));

		glBindTextureUnit(0, m_Texture_RESOLVE);

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

}

void Viewer::drawViews(const Shader& shader) const
{
	initShaderVariables(shader);

	const float zNear = scene_state_.zNear;
	const float zFar = scene_state_.zFar;
	const float fov = 45.0;
	float viewport_w = 0.0f;
	float viewport_h = 0.0f;

	// left view : LINEAR
	shader.setInt("albedo_interp", static_cast<int>(AlbedoInterpolation::Linear));
	shader.setBool("no_warp", true);

	viewport_w = scene_state_.width / 2.0f;
	viewport_h = scene_state_.height;
	auto mat_proj = glm::perspectiveFov(
		glm::radians(fov), viewport_w, viewport_h, zNear, zFar);
	scene_state_.camera.setProjectionMatrix(mat_proj);

	glViewport(0, 0, viewport_w, viewport_h);
	g_mesh->render();

	// right view : OURS
	shader.setInt("albedo_interp", static_cast<int>(AlbedoInterpolation::Gaussianized));
	shader.setBool("no_warp", false);

	glBindTextureUnit(warpgrid_layout, pbrTextureGLIndex[warpgrid_layout]);

	viewport_w = scene_state_.width / 2.0f;
	viewport_h = scene_state_.height;
	mat_proj = glm::perspectiveFov(
		glm::radians(fov), viewport_w, viewport_h, zNear, zFar);
	scene_state_.camera.setProjectionMatrix(mat_proj);
	glViewport(scene_state_.width / 2.0f, 0, viewport_w, viewport_h);
	g_mesh->render();
}

void Viewer::drawQuadAndSetupSSAO() const
{
	m_ssaoShader.use();

	glViewport(0, 0, scene_state_.width, scene_state_.height);

	glBindTextureUnit(0, m_gNormal);
	glBindTextureUnit(1, m_gPosition);
	glBindTextureUnit(2, glNoiseTexture);

	const float zNear = scene_state_.zNear;
	const float zFar = scene_state_.zFar;
	const float fov = 45.0;
	float viewport_w = scene_state_.width;
	float viewport_h = scene_state_.height;

	viewport_w = scene_state_.width / 2.0f;
	viewport_h = scene_state_.height;

	m_ssaoShader.setFloat("screen_height", scene_state_.height);
	m_ssaoShader.setFloat("screen_width", scene_state_.width);

	m_ssaoShader.setMat4("viewMatrixInv", scene_state_.camera.getInvViewMatrix());
	m_ssaoShader.setMat4("viewMatrix", scene_state_.camera.viewMatrix());

	m_ssaoShader.setFloat("radius", scene_state_.ssao_radius);

	glm::mat4 ProjMat = glm::perspectiveFov(glm::radians(fov), viewport_w, viewport_h, zNear, zFar);
	glm::mat4 InvProjMat = glm::inverse(ProjMat);

	m_ssaoShader.setMat4("projMatrix", ProjMat);
	m_ssaoShader.setMat4("projMatrixInv", InvProjMat);

	// Send kernel + rotation 
	for (unsigned int i = 0; i < ssaoKernelSize; ++i)
		m_ssaoShader.setVec3("samples[" + std::to_string(i) + "]", ssaoKernel[i]);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Viewer::drawQuadAndBlurSSAO() const
{
	m_ssaoBlurShader.use();

	glViewport(0, 0, scene_state_.width, scene_state_.height);
	
	glBindTextureUnit(0, m_SSAOColorBuffer);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Viewer::initShaderVariables(const Shader& shader) const
{
	shader.use();

	shader.setMat4("mat_proj", scene_state_.camera.projMatrix());
	shader.setMat4("mat_view", scene_state_.camera.viewMatrix());
	shader.setVec3("cam_pos", scene_state_.camera.position());

	//Debug options
	shader.setInt("debug_view_mode", static_cast<int>(scene_state_.debug_view_mode));
	shader.setBool("ycbcr_interp", scene_state_.ycbcr);
	shader.setInt("rendered_mesh", static_cast<int>(scene_state_.renderedMesh));

	//Technical parameters
	shader.setInt("tess_level", scene_state_.tess_level);
	shader.setFloat("height_factor", scene_state_.height_factor);
	shader.setFloat("hf_1", scene_state_.comp_hf_1);
	shader.setFloat("hf_2", scene_state_.comp_hf_2);
	shader.setBool("wireframe", scene_state_.wireframe);

	shader.setFloat("screen_width", scene_state_.width);
	shader.setFloat("screen_height", scene_state_.height);
	shader.setFloat("textureSize", PBRTextWidth); // Assuming PBRTextWidth == PBRTextHeight

	//Interpolation parameters
	shader.setFloat("interpolation_value", scene_state_.global_interpolation);
	shader.setFloat("mix_color", scene_state_.mix_color);
	shader.setFloat("mix_roughness", scene_state_.mix_roughness);
	shader.setFloat("mix_metallic", scene_state_.mix_metallic);
	shader.setFloat("mix_height", scene_state_.mix_height);
	shader.setFloat("mix_normal", scene_state_.mix_normal);
}

void Viewer::resize(int width, int height)
{
	glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
	updateProjectionMat();
	generateFramebuffers();
}

void Viewer::setupQuadRendering()
{
	float quadVertices[] = {
		// vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
		// positions   // texCoords
		-1.0f, 1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 0.0f,

		-1.0f, 1.0f, 0.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 0.0f,
		1.0f, 1.0f, 1.0f, 1.0f };

	// screen quad VAO
	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
}

void Viewer::generateFramebuffers()
{
	float viewport_width = scene_state_.width;
	float viewport_height = scene_state_.height;

	{
		// delete all if necessary
		// --------------------------

		if (glIsFramebuffer(m_fbo_gbuffer))
			glDeleteFramebuffers(1, &m_fbo_gbuffer);
		if (glIsFramebuffer(m_fbo_ssao))
			glDeleteFramebuffers(1, &m_fbo_ssao);
		if (glIsFramebuffer(m_fbo_ssao_blur))
			glDeleteFramebuffers(1, &m_fbo_ssao_blur);

		if (glIsTexture(m_gPosition))
			glDeleteTextures(1, &m_gPosition);
		if (glIsTexture(m_gNormal))
			glDeleteTextures(1, &m_gNormal);
		if (glIsTexture(m_SSAOColorBuffer))
			glDeleteTextures(1, &m_SSAOColorBuffer);

		if (glIsFramebuffer(m_fbo_RENDER))
			glDeleteFramebuffers(1, &m_fbo_RENDER);
		if (glIsTexture(m_Texture_RENDER))
			glDeleteTextures(1, &m_Texture_RENDER);
		if (glIsRenderbuffer(m_rbo_RENDER))
			glDeleteRenderbuffers(1, &m_rbo_RENDER);

		if (glIsFramebuffer(m_fbo_RESOLVE))
			glDeleteFramebuffers(1, &m_fbo_RESOLVE);
		if (glIsTexture(m_Texture_RESOLVE))
			glDeleteTextures(1, &m_Texture_RESOLVE);

		if (glIsRenderbuffer(m_depthrenderbuffer))
			glDeleteRenderbuffers(1, &m_depthrenderbuffer);
	}

	{
		// configure HDR framebuffer
		// --------------------------

		/* create custom color buffer */
		glGenTextures(1, &m_Texture_RESOLVE);
		glBindTexture(GL_TEXTURE_2D, m_Texture_RESOLVE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, viewport_width, viewport_height, 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);

		/* create floating point multisampled color buffer */
		glGenTextures(1, &m_Texture_RENDER);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_Texture_RENDER);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 16, GL_RGBA16F, viewport_width, viewport_height, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenRenderbuffers(1, &m_rbo_RENDER);
		glBindRenderbuffer(GL_RENDERBUFFER, m_rbo_RENDER);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, 16, GL_DEPTH_COMPONENT24, viewport_width, viewport_height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		/* FBOs */
		glGenFramebuffers(1, &m_fbo_RENDER);
		GLenum const BuffersRender = GL_COLOR_ATTACHMENT0;
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_RENDER);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_Texture_RENDER, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_rbo_RENDER);
		glDrawBuffers(1, &BuffersRender);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;

		glGenFramebuffers(1, &m_fbo_RESOLVE);
		GLenum const BuffersResolve = GL_COLOR_ATTACHMENT0;
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_RESOLVE);
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_Texture_RESOLVE, 0);
		glDrawBuffers(1, &BuffersResolve);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDrawBuffer(GL_BACK);
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "Framebuffer not complete!" << std::endl;
	}

	{
		// normal color texture
		glGenTextures(1, &m_gNormal);
		glBindTexture(GL_TEXTURE_2D, m_gNormal);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, viewport_width, viewport_height, 0, GL_RGB, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		// position color texture
		glGenTextures(1, &m_gPosition);
		glBindTexture(GL_TEXTURE_2D, m_gPosition);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, viewport_width, viewport_height, 0, GL_RGB, GL_FLOAT, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);		
		glBindTexture(GL_TEXTURE_2D, 0);

		// The depth buffer (for depth testing)
		glGenRenderbuffers(1, &m_depthrenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depthrenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, viewport_width, viewport_height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// configure gNormal framebuffer
		// --------------------------
		glGenFramebuffers(1, &m_fbo_gbuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_gbuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gNormal, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gPosition, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthrenderbuffer);
		unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
		glDrawBuffers(2, attachments);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	
	{
		// configure SSAO framebuffer
		// --------------------------
		glGenFramebuffers(1, &m_fbo_ssao);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_ssao);

		glGenTextures(1, &m_SSAOColorBuffer);
		glBindTexture(GL_TEXTURE_2D, m_SSAOColorBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, viewport_width, viewport_height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_SSAOColorBuffer, 0);
		glBindTexture(GL_TEXTURE_2D, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	
	{
		// configure SSAO Blur framebuffer
		// --------------------------
		glGenFramebuffers(1, &m_fbo_ssao_blur);
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbo_ssao_blur);

		glGenTextures(1, &pbrTextureGLIndex[ssao_layout]);
		glBindTexture(GL_TEXTURE_2D, pbrTextureGLIndex[ssao_layout]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, viewport_width, viewport_height, 0, GL_RGB, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pbrTextureGLIndex[ssao_layout], 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Intermediate framebuffer is not complete!" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

void Viewer::initAndLoadGaussianizedTexture(GLuint textureLayoutIdx, GLuint inv_cdf_layout, const string& mat_path)
{
	if (glIsTexture(pbrTextureGLIndex[textureLayoutIdx]))
		glDeleteTextures(1, &pbrTextureGLIndex[textureLayoutIdx]);

	glCreateTextures(GL_TEXTURE_2D, 1, &pbrTextureGLIndex[textureLayoutIdx]);
	glBindTextureUnit(textureLayoutIdx, pbrTextureGLIndex[textureLayoutIdx]);
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureStorage2D(pbrTextureGLIndex[textureLayoutIdx], 1, GL_RGB32F, PBRTextWidth, PBRTextHeight);

	loadGaussianTexture(textureLayoutIdx, inv_cdf_layout, mat_path);
}

void Viewer::loadGaussianTexture(GLuint gaussian_texture_layout_idx, GLuint inv_cdf_layout, const string& mat_path)
{
	stbi_set_flip_vertically_on_load(true);

	int width, height, nrChannels;
	unsigned char* img_ptr = stbi_load((mat_path + "/color.png").c_str(), &width, &height, &nrChannels, 4);
	if (img_ptr)
	{
		size_t img_2d_size = static_cast<size_t>(width) * static_cast<size_t>(height);
		vector<float> gaussianized(img_2d_size * static_cast<size_t>(4), 0);
		vector<float> inv_cdf_LUT;

		auto startTime = std::chrono::system_clock::now();

		//getGaussianizedAlbedoAndCDF(img_ptr, gaussianized, inv_cdf_LUT, width, height, nrChannels, scene_state_.ycbcr_interpolation);

		gaussianizedAlbedoGPU(inv_cdf_LUT, gaussianized, m_histogramComputeShader, img_ptr, width, height, nrChannels, scene_state_.ycbcr);

		auto endTime = std::chrono::system_clock::now();
		auto time = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
		cout << "gaussianizing : " << time << " ms" << endl;

		glTextureSubImage2D(pbrTextureGLIndex[gaussian_texture_layout_idx], 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, gaussianized.data());
		glGenerateTextureMipmap(pbrTextureGLIndex[gaussian_texture_layout_idx]);

		GLsizei cdf_width = histogram_size_resampled, cdf_height = 3;
		glTextureSubImage2D(pbrTextureGLIndex[inv_cdf_layout], 0, 0, 0, cdf_width, cdf_height, GL_RED, GL_FLOAT, inv_cdf_LUT.data());

		stbi_image_free(img_ptr);
	}
	else
	{
		std::cerr << "Failed to load albedo texture at: " << mat_path << std::endl;
		exit(EXIT_FAILURE);
	}

	stbi_set_flip_vertically_on_load(false);


}

void Viewer::reloadGaussianTextures()
{
	if (!scene_state_.mat1_path.empty())
		loadGaussianTexture(gaussian1_layout, inv_cdf1_layout, scene_state_.mat1_path);
	else
		cerr << "warning: material1 not loaded, trying to compute gaussianized texture!" << endl;
	if (!scene_state_.mat2_path.empty())
		loadGaussianTexture(gaussian2_layout, inv_cdf2_layout, scene_state_.mat2_path);
	else
		cerr << "warning: material2 not loaded, trying to compute gaussianized texture!" << endl;
}

void Viewer::initializeInterpolatedLUT()
{
	GLuint cdf_width = histogram_size_resampled, cdf_height = 3;

	// inverse CDF of texture 1
	if (glIsTexture(pbrTextureGLIndex[inv_cdf1_layout]))
		glDeleteTextures(1, &pbrTextureGLIndex[inv_cdf1_layout]);

	glCreateTextures(GL_TEXTURE_2D, 1, &pbrTextureGLIndex[inv_cdf1_layout]);
	glBindTextureUnit(inv_cdf1_layout, pbrTextureGLIndex[inv_cdf1_layout]);
	glTextureStorage2D(pbrTextureGLIndex[inv_cdf1_layout], 1, GL_R32F, cdf_width, cdf_height);

	// inverse CDF of texture 2
	if (glIsTexture(pbrTextureGLIndex[inv_cdf2_layout]))
		glDeleteTextures(1, &pbrTextureGLIndex[inv_cdf2_layout]);

	glCreateTextures(GL_TEXTURE_2D, 1, &pbrTextureGLIndex[inv_cdf2_layout]);
	glBindTextureUnit(inv_cdf2_layout, pbrTextureGLIndex[inv_cdf2_layout]);
	glTextureStorage2D(pbrTextureGLIndex[inv_cdf2_layout], 1, GL_R32F, cdf_width, cdf_height);
}

void Viewer::loadMaterial(string filepath, int mat_idx)
{
	texture_layout_idx = mat_idx == 1 ? 0 : 5;

	std::string mat_path = filepath;
	std::size_t found = filepath.find_last_of("/\\");
	std::string mat_name = filepath.substr(found + 1);

	if (mat_idx == 1) {
		scene_state_.mat1_path = mat_path;
		scene_state_.mat1_name = mat_name;
	}
	else {
		scene_state_.mat2_path = mat_path;
		scene_state_.mat2_name = mat_name;
	}

	bool valid_material = true;

	// load PBR textures
	for (int i = 0; i < 5; i++)
	{
		valid_material = valid_material &&
			setupTexture(texture_layout_idx + i, (mat_path + "/" + maps_name[i] + ".png").c_str(), static_cast<TextureType>(i));
	}

	if (valid_material)
	{
		// load gaussianized albedo
		if (mat_idx == 1)
			initAndLoadGaussianizedTexture(gaussian1_layout, inv_cdf1_layout, mat_path);
		else
			initAndLoadGaussianizedTexture(gaussian2_layout, inv_cdf2_layout, mat_path);

		unsigned char* albedo_img_ptr = stbi_load(
			(mat_path + "/color.png").c_str(),
			&albedo_width, &albedo_height, &albedo_nrChannels, 3);

		if (albedo_nrChannels != 3)
			cout << "warning: colormap has an alpha channel" << endl;

		albedo_nrChannels = 3;

		if (albedo_img_ptr)
		{
			vector<double> albedo_channel = vector<double>(size_t(albedo_width) * size_t(albedo_height), 0);
			if (mat_idx == 1)
			{
				albedo_img_1 = vector<vector<double>>(albedo_nrChannels, albedo_channel);
				loadImageAsLinearChannelSeparatedVector(albedo_img_ptr, albedo_img_1, albedo_width, albedo_height, albedo_nrChannels);
			}
			else
			{
				albedo_img_2 = vector<vector<double>>(albedo_nrChannels, albedo_channel);
				loadImageAsLinearChannelSeparatedVector(albedo_img_ptr, albedo_img_2, albedo_width, albedo_height, albedo_nrChannels);
			}
			stbi_image_free(albedo_img_ptr);
		}
		else
		{
			std::cout << "could not load png at : " << mat_name + "/color.png" << std::endl;
		}

		// compute proper height factor and load normal from height
		computeNormalFromHeight(mat_idx, mat_path);
	}
}

void Viewer::loadWarpGrid(string filename)
{
	std::size_t found = filename.find_last_of("/\\");

	scene_state_.warp_grid = filename.substr(found + 1);

	string filenameStr = filename;
	char* argv[] = { &filenameStr[0] };
	
	warp_map = std::unique_ptr<Warpgrid>(new Warpgrid(0, argv, WarpgridType::OpenFromFile));
	if (warp_map->isLoaded()) loadWarpGridOnGPU();
	else std::cerr << "Error loading warp grid from file" << std::endl;
}

void Viewer::loadWarpGridOnGPU()
{
	vector<float> warpdata;
	size_t nvertices = 0;
	GLuint current_glID = -1;

	if (warp_map->isLoaded())
	{
		warpdata = warp_map->getPointDataConstRef();
		nvertices = warp_map->getNumberOfVertices();

		if (glIsTexture(pbrTextureGLIndex[warpgrid_layout]))
			glDeleteTextures(1, &pbrTextureGLIndex[warpgrid_layout]);

		glCreateTextures(GL_TEXTURE_2D, 1, &pbrTextureGLIndex[warpgrid_layout]);
		glBindTextureUnit(warpgrid_layout, pbrTextureGLIndex[warpgrid_layout]);

		current_glID = pbrTextureGLIndex[warpgrid_layout];
	}
	else
	{
		cout << "cannot load warp grid" << endl;
		exit(EXIT_FAILURE);
	}

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTextureParameteri(current_glID, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT); //GL_CLAMP_TO_EDGE ??
	glTextureParameteri(current_glID, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT); //GL_CLAMP_TO_EDGE ??
	glTextureParameteri(current_glID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(current_glID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load and generate the texture
	GLsizei width = int(sqrt(nvertices)), height = int(sqrt(nvertices));
	glTextureStorage2D(current_glID, 1, GL_RG32F, width, height);
	glTextureSubImage2D(current_glID, 0, 0, 0, width, height, GL_RG, GL_FLOAT, warpdata.data());
}

void Viewer::computeNormalFromHeight(int mat_id, const string& mat_path)
{
	if (mat_id == 1)
		computeHeightFactor(m_normalComputeShader, mat_id, mat_path, scene_state_.comp_hf_1);
	else if (mat_id == 2)
		computeHeightFactor(m_normalComputeShader, mat_id, mat_path, scene_state_.comp_hf_2);
	else
		cout << "wrong material ID given for computing height factor!" << endl;
}

void Viewer::captureScreenshot()
{
	// https://vallentin.dev/2013/09/02/opengl-screenshot

	std::string filename;
	filename += "../screenshots/";
	filename += createScreenshotBasename();

	int x = scene_state_.viewport[0];
	int y = scene_state_.viewport[1];
	int width = scene_state_.viewport[2];
	int height = scene_state_.viewport[3];

	std::vector<char> data(size_t(3) * width * height); // 3 components (R, G, B)

	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, data.data());

	stbi_flip_vertically_on_write(1);

	int saved = stbi_write_png(filename.c_str(), width, height, 3, data.data(), 0);

	if (!saved)
		cerr << "Failed Saving Image: " << filename << endl;
	else
		cout << "Successfully Saved Image: " << filename << endl;
		
}

void Viewer::updateProjectionMat()
{
	const float fov = 45.0;

	if (scene_state_.width > 0.0f && scene_state_.height > 0.0f)
	{
		glm::mat4 perspectiveFov = glm::perspectiveFovRH(
			glm::radians(fov),
			scene_state_.width,
			scene_state_.height,
			scene_state_.zNear,
			scene_state_.zFar);
		scene_state_.camera.setProjectionMatrix(perspectiveFov);
	}
}

void Viewer::setupMesh()
{
	switch (scene_state_.renderedMesh)
	{
	case RenderedMesh::Plane:
		g_mesh = Mesh::genPlane();
		break;
	case RenderedMesh::Sphere:
		g_mesh = Mesh::genSphere();
		break;
	default:
		cout << "wrong mesh provided" << endl;
		break;
	}

	if (!g_mesh->init())
	{
		cout << "something wrong happened when loading the mesh" << endl;
		exit(1);
	}
}

void Viewer::setupIrradianceMapAndBrdfLut()
{
	glCreateTextures(GL_TEXTURE_2D, 1, &dfgLUTGLIndex);
	glBindTextureUnit(24, dfgLUTGLIndex);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTextureParameteri(dfgLUTGLIndex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(dfgLUTGLIndex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(dfgLUTGLIndex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(dfgLUTGLIndex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	// load and generate the texture
	GLsizei width, height, nrChannels;
	unsigned char* data = stbi_load("../src/assets/envmap/brdfLUT.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTextureStorage2D(dfgLUTGLIndex, 1, GL_RGBA8, width, height);
		glTextureSubImage2D(dfgLUTGLIndex, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else
	{
		std::cerr << "Failed to load brdf lut" << std::endl;
	}
	stbi_image_free(data);

	width = 256; height = 256;
	unsigned int mips = 5;
	std::vector<std::string> map_id = { "px", "nx", "py", "ny", "pz", "nz" };

	glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &irradianceMapGLIndex);
	glBindTextureUnit(25, irradianceMapGLIndex);
	glTextureStorage2D(irradianceMapGLIndex, mips, GL_RGBA32F, width, height);

	bool load_success = true;

	for (unsigned int mip_lvl = 0; mip_lvl < mips; ++mip_lvl)
	{
		for (unsigned int face_id = 0; face_id < 6; ++face_id)
		{
			std::string path = "../src/assets/envmap/m" + std::to_string(mip_lvl)
				+ "_" + map_id[face_id] + ".hdr";
			unsigned char* env_data = stbi_load(path.data(), &width, &height, &nrChannels, 0);
			if (env_data)
			{
				glTextureSubImage3D(irradianceMapGLIndex, mip_lvl, 0, 0, face_id,
					width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, env_data);
			}
			else
			{
				load_success = false;
			}
			stbi_image_free(env_data);
		}
	}

	if (!load_success)
		std::cerr << "Failed to load cube maps" << std::endl;

	glTextureParameteri(irradianceMapGLIndex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(irradianceMapGLIndex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(irradianceMapGLIndex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTextureParameteri(irradianceMapGLIndex, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(irradianceMapGLIndex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

bool Viewer::setupTexture(int textureLayoutIdx, const char* path, TextureType type)
{
	stbi_set_flip_vertically_on_load(true);

	if (glIsTexture(pbrTextureGLIndex[textureLayoutIdx]))
		glDeleteTextures(1, &pbrTextureGLIndex[textureLayoutIdx]); 

	glCreateTextures(GL_TEXTURE_2D, 1, &pbrTextureGLIndex[textureLayoutIdx]);
	glBindTextureUnit(textureLayoutIdx, pbrTextureGLIndex[textureLayoutIdx]);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(pbrTextureGLIndex[textureLayoutIdx], GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// load and generate the texture
	GLsizei nrChannels = 0;
	unsigned short* data = nullptr;

	if (type == TextureType::HeightMap 
		|| type == TextureType::RoughnessMap
		|| type == TextureType::MetallicMap)
	{
		data = stbi_load_16(path, &PBRTextWidth, &PBRTextHeight, &nrChannels, 1);
		if (nrChannels != 1)
			cout << "warning: greyscale map has more than 1 channel!" << endl;
		nrChannels = 1;
	}

	if (type == TextureType::NormalMap
		|| type == TextureType::AlbedoMap)
	{
		data = stbi_load_16(path, &PBRTextWidth, &PBRTextHeight, &nrChannels, 3);
		if (nrChannels != 3)
			cout << "warning: rgb map does not have 3 channels as it should!" << endl;
		nrChannels = 3;
	}

	GLenum format = GL_RGBA;
	if (nrChannels == 3)
		format = GL_RGB;
	else if (nrChannels == 1)
		format = GL_RED;

	if (data)
	{
		switch (type)
		{
		case TextureType::AlbedoMap: // Reading as Linear and applying gamma correction in the fragment shader
		case TextureType::NormalMap:
		{
			glTextureStorage2D(pbrTextureGLIndex[textureLayoutIdx], 1, GL_RGB16, PBRTextWidth, PBRTextHeight);
			glTextureSubImage2D(pbrTextureGLIndex[textureLayoutIdx], 0, 0, 0, PBRTextWidth, PBRTextHeight, format, GL_UNSIGNED_SHORT, data);
			glGenerateTextureMipmap(pbrTextureGLIndex[textureLayoutIdx]);
			break;
		}
		case TextureType::HeightMap:
		case TextureType::MetallicMap:
		case TextureType::RoughnessMap:
		{
			glTextureStorage2D(pbrTextureGLIndex[textureLayoutIdx], 1, GL_R16, PBRTextWidth, PBRTextHeight);
			glTextureSubImage2D(pbrTextureGLIndex[textureLayoutIdx], 0, 0, 0, PBRTextWidth, PBRTextHeight, format, GL_UNSIGNED_SHORT, data);
			glGenerateTextureMipmap(pbrTextureGLIndex[textureLayoutIdx]);
			break;
		}
		default:
			break;
		}
	}
	else
	{
		std::cout << "Failed to load texture : " << path << std::endl;
		return false;
	}
	if (data)
		stbi_image_free(data);

	stbi_set_flip_vertically_on_load(false);

	return true;
}

const char* Viewer::createScreenshotBasename()
{
	static char basename[30];

#ifdef _WIN32

	struct tm newtime;
	char am_pm[] = "AM";
	__time64_t long_time;
	errno_t err;

	// Get time as 64-bit integer.
	_time64(&long_time);
	// Convert to local time.
	err = _localtime64_s(&newtime, &long_time);
	if (err)
	{
		printf("Invalid argument to _localtime64_s.");
		return basename;
	}

	// Convert to an filename representation.
	strftime(basename, 26, "%Y-%m-%d_%H.%M.%S.png", &newtime);

#else

	time_t t = time(NULL);
	strftime(basename, 30, "%Y-%m-%d_%H.%M.%S.png", localtime(&t));

#endif //_WIN32

	return basename;
}

void Viewer::reloadShaders(int i)
{
	switch (i)
	{
		case 0:
			while (!m_customShader.reloadShader()) {}
			break;
		case 1:
			while (!m_screenShader.reloadShader()) {}
			break;
		case 2:
			while (!m_normalShader.reloadShader()) {}
			break;
		case 3:
			while (!m_ssaoShader.reloadShader()) {}
			break;
		case 4:
			while (!m_ssaoBlurShader.reloadShader()) {}
			break;
		default:
			break;
	}

	cout << "shader compiled successfully" << endl;
}