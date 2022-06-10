#pragma once

#define _USE_MATH_DEFINES
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera
{
public:
	Camera();
	~Camera();

	enum class TrackMode
	{
		Disable,
		Rotate,
		Translate,
		Zoom
	};

	inline glm::vec3 position() const { return m_position; }
	inline glm::mat4 viewMatrix() const { return m_viewMatrix; }
	inline glm::mat4 projMatrix() const { return m_projectionMatrix; }

	void setViewMatrix(glm::mat4 & matrix)
	{
		m_viewMatrix = matrix;
		m_InvViewMatrix = glm::inverse(matrix);
	} // use with caution

	void setProjectionMatrix(glm::mat4 & matrix)
	{
		m_projectionMatrix = matrix;
		m_InvProjectionMatrix = glm::inverse(matrix);
	}
	glm::mat4 &getInvViewMatrix() { return m_InvViewMatrix; }
	glm::mat4 &getInvProjMatrix() { return m_InvProjectionMatrix; }

	inline void tiltLeft() { tilt(-0.1f); }
	inline void tiltRight() { tilt(0.1f); }

	void track(float x, float y);
	void start(TrackMode mode, float x, float y);
	virtual void zoom(float factor){};
	virtual void tilt(float theta) {}
	void stop();

protected:
	/**
	* Called when mouse moves and rotation has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseRotation(float x1, float y1, float x2, float y2) {}

	/**
	* Called when mouse moves and zoom has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseZoom(float x1, float y1, float x2, float y2) {}

	/**
	* Called when mouse moves and panning has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMousePanning(float x1, float y1, float x2, float y2) {}

protected:
	TrackMode track_mode_ = TrackMode::Disable;

	glm::vec3 m_position;
	glm::mat4 m_viewMatrix, m_projectionMatrix;
	glm::mat4 m_InvViewMatrix, m_InvProjectionMatrix;

	bool m_isLastMouseUpToDate;
	float m_lastMouseX, m_lastMouseY;
};

class TurntableCamera : public Camera
{
public:
	TurntableCamera()
		: Camera(),
		  m_center(0.0f, 0.0f, 0.0f),
		  m_zoom(3.0f),
		  m_sensitivity(3.0f),
		  m_zoomSensitivity(1.0f)
	{
		m_quat = glm::quat(sqrt(2.f) / 2.f, -sqrt(2.f) / 2.f, 0.f, 0.f);
		float theta = 1.5f;
		m_quat *= glm::quat(cos(theta / 2.f), float(sin(theta / 2.f)) * glm::vec3(1.0f, 0.0f, 0.0f));

		updateViewMatrix();
	}

	void update()
	{
		updateViewMatrix();
	}

	void init_zoom(float scale) { m_zoom = scale; };
	void zoom(float factor) override;
	void tilt(float factor) override;

protected:
	void updateDeltaMouseRotation(float x1, float y1, float x2, float y2) override;
	void updateDeltaMouseZoom(float x1, float y1, float x2, float y2) override;
	void updateDeltaMousePanning(float x1, float y1, float x2, float y2) override;

	/**
	 * Construct view matrix given quat, center and zoom
	 */
	void updateViewMatrix();

private:
	glm::vec3 m_center;
	glm::quat m_quat;
	float m_zoom;
	float m_sensitivity; // relative to screen size
	float m_zoomSensitivity;
};