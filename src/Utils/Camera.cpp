#include "Camera.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <algorithm>

using glm::mat3;
using glm::mat4;
using glm::quat;
using glm::vec3;

using namespace std;

Camera::Camera() : m_isLastMouseUpToDate(false),
					m_position(0.0f, 0.0f, 0.0f),
					m_viewMatrix(1.0f),
					m_projectionMatrix(1.0f),
					m_InvViewMatrix(1.0f),
					m_InvProjectionMatrix(5.0f),
					m_lastMouseX(0.f),
					m_lastMouseY(0.f)
{
}

Camera::~Camera()
{
}

void Camera::track(float x, float y)
{
	if (m_isLastMouseUpToDate)
	{
		switch (track_mode_)
		{
		case TrackMode::Rotate:
			updateDeltaMouseRotation(m_lastMouseX, m_lastMouseY, x, y);
			break;
		case TrackMode::Zoom:
			updateDeltaMouseZoom(m_lastMouseX, m_lastMouseY, x, y);
			break;
		case TrackMode::Translate:
			updateDeltaMousePanning(m_lastMouseX, m_lastMouseY, x, y);
			break;
		};
	}
	m_lastMouseX = x;
	m_lastMouseY = y;
	m_isLastMouseUpToDate = true;
}

void Camera::start(TrackMode mode, float x, float y)
{
	m_lastMouseX = x;
	m_lastMouseY = y;
	track_mode_ = mode;
}

void Camera::stop()
{
	track_mode_ = TrackMode::Disable;
}

void TurntableCamera::updateViewMatrix()
{
	m_viewMatrix = glm::translate(vec3(0.f, 0.f, -m_zoom)) * glm::toMat4(m_quat) * glm::translate(m_center);
	m_InvViewMatrix = glm::inverse(m_viewMatrix);
	m_position = vec3(m_InvViewMatrix[3]);
	//updateUbo();
}

void TurntableCamera::updateDeltaMouseRotation(float x1, float y1, float x2, float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;
	float theta;

	// rotate around camera X axis by dy
	theta = dy * m_sensitivity;
	vec3 xAxis = vec3(glm::row(m_viewMatrix, 0));
	m_quat *= quat(cos(theta / 2.f), sin(theta / 2.f) * xAxis);

	// rotate around world Z axis by dx
	theta = dx * m_sensitivity;
	m_quat *= quat(cos(theta / 2.f), sin(theta / 2.f) * vec3(0.f, 1.f, 0.f));

	updateViewMatrix();
}

void TurntableCamera::updateDeltaMouseZoom(float x1, float y1, float x2, float y2)
{
	float dy = y2 - y1;
	m_zoom *= (1.f + dy * m_zoomSensitivity);
	m_zoom = max(m_zoom, 0.0001f);
	updateViewMatrix();
}

void TurntableCamera::updateDeltaMousePanning(float x1, float y1, float x2, float y2)
{
	float dx = (x2 - x1) / 1.f * m_zoom;
	float dy = -(y2 - y1) / 1.f * m_zoom;

	// move center along the camera screen plane
	vec3 xAxis = vec3(glm::row(m_viewMatrix, 0));
	vec3 yAxis = vec3(glm::row(m_viewMatrix, 1));
	m_center += dx * xAxis + dy * yAxis;

	updateViewMatrix();
}

void TurntableCamera::zoom(float factor)
{
	m_zoom *= (1.f + factor * m_zoomSensitivity);
	m_zoom = max(m_zoom, 0.0001f);
	updateViewMatrix();
}

void TurntableCamera::tilt(float theta)
{
	vec3 zAxis = vec3(glm::row(m_viewMatrix, 2));
	m_quat *= quat(cos(theta / 2.f), sin(theta / 2.f) * zAxis);
	updateViewMatrix();
}
