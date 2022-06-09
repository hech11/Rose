#include "PerspectiveCamera.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/fwd.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include <corecrt_math_defines.h>
#include <algorithm>


#include "Rose/Core/Log.h"


namespace Rose
{
	//////////////////////////////////////////////////////////////////////////
	////////////////////////// PerspectiveCamera /////////////////////////////
	//////////////////////////////////////////////////////////////////////////


	PerspectiveCamera::PerspectiveCamera(const glm::mat4& projection)
		: m_Projection(projection)
	{
		m_Rotation = { 90.0f, 0.0f, 0.0f };
		m_FocalPoint = { 0.0f, 0.0f, 0.0f };

		m_Distance = glm::distance({ -5, 5, 5 }, m_FocalPoint);

		m_Pitch = M_PI / 4.0f;
		m_Yaw = 3.0f * (float)M_PI / 4.0f;


		RecalculateMatrix();
	}

	void PerspectiveCamera::SetPosition(const glm::vec3& position)
	{
		RecalculateMatrix();
	}


	void PerspectiveCamera::SetRotation(const glm::vec3& rotation)
	{
		m_Rotation = rotation;
	}


	void PerspectiveCamera::SetFocalPoint(const glm::vec3& point)
	{
		m_FocalPoint = point;
		RecalculateMatrix();
	}

	void PerspectiveCamera::OffsetFocalPoint(const glm::vec3& point)
	{
		m_FocalPoint += point;
	}

	void PerspectiveCamera::SetDistance(float value)
	{
		m_Distance = value;
		RecalculateMatrix();
	}

	void PerspectiveCamera::OffsetDistance(float value)
	{
		m_Distance += value;
		RecalculateMatrix();
	}

	void PerspectiveCamera::Move(const glm::vec3& dir)
	{
		m_Position += dir;
		RecalculateMatrix();
	}


	void PerspectiveCamera::Rotate(const glm::vec3& dir)
	{
		m_Rotation += dir;
		RecalculateMatrix();
	}

	void PerspectiveCamera::OffsetYaw(float value)
	{
		m_Yaw += value;
	}

	void PerspectiveCamera::OffsetPitch(float value)
	{
		m_Pitch += value;
	}

	glm::vec3 PerspectiveCamera::GetForwardDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 PerspectiveCamera::GetRightDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 PerspectiveCamera::GetUpDirection()
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::quat PerspectiveCamera::GetOrientation()
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

	void PerspectiveCamera::RecalculateMatrix()
	{
		m_Position = m_FocalPoint - GetForwardDirection() * m_Distance;
		auto orientation = GetOrientation();

		m_Rotation = glm::eulerAngles(orientation) * (180.0f / (float)M_PI);

		m_View = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		//m_View = glm::inverse(m_View);
		m_ProjView = m_Projection * glm::inverse(m_View);

	}





	//////////////////////////////////////////////////////////////////////////
	///////////////////// PerspectiveCameraController ////////////////////////
	//////////////////////////////////////////////////////////////////////////





	PerspectiveCameraController::PerspectiveCameraController(const PerspectiveCamera& camera)
		: m_Camera(camera)
	{
		m_RotationSpeed = 0.8f;
	}


	void PerspectiveCameraController::OnUpdate(float ts)
	{

		m_MouseDelta = (m_MousePosition - m_PrevMousePosition) * 0.003f;
		m_PrevMousePosition = m_MousePosition;
		if (IsLeftMouseButtonHeld)
		{

			// These values were calculated via a graph visualizer.
			// These are used to control how "smooth" the camera moves

			float graphDimensionScale = 2.4f;
			float graphStepDown = 0.0366f;
			float graphStepUp = 0.1778f;

			float x = std::min(1600.0f / 1000.0f, graphDimensionScale);
			float xFactor = graphStepDown * (x * x) - graphStepUp * x + 0.3021f;

			float y = std::min(900.0f / 1000.0f, graphDimensionScale);
			float yFactor = graphStepDown * (y * y) - graphStepUp * y + 0.3021f;

			m_Camera.OffsetFocalPoint(-m_Camera.GetRightDirection() * m_MouseDelta.x * xFactor * m_Camera.GetDistance());
			m_Camera.OffsetFocalPoint(m_Camera.GetUpDirection() * m_MouseDelta.y * yFactor * m_Camera.GetDistance());
			m_Camera.RecalculateMatrix();
		}
		else if (IsRightMouseButtonHeld)
		{

			float graphDimensionScale = 2.4f;
			float graphStepDown = 0.0366f;
			float graphStepUp = 0.1778f;

			float x = std::min(1600.0f / 1000.0f, graphDimensionScale);
			float xFactor = graphStepDown * (x * x) - graphStepUp * x + 0.3021f;

			float y = std::min(900.0f / 1000.0f, graphDimensionScale);
			float yFactor = graphStepDown * (y * y) - graphStepUp * y + 0.3021f;

			float yawSign = m_Camera.GetUpDirection().y < 0 ? -1.0f : 1.0f;
			m_Camera.OffsetYaw(yawSign * m_MouseDelta.x * m_RotationSpeed);
			m_Camera.OffsetPitch(m_MouseDelta.y * m_RotationSpeed);
			m_Camera.RecalculateMatrix();

		}
		else if (IsMiddleMouseButtonHeld)
		{
			float dist = m_Camera.GetDistance() * 0.2f;
			dist = std::max(dist, 0.0f);
			m_ZoomSpeed = dist * dist;
			m_ZoomSpeed = std::min(m_ZoomSpeed, 100.0f);
			m_Camera.OffsetDistance(-m_MouseDelta.y * m_ZoomSpeed);
			if (m_Camera.GetDistance() < 1.0f) {
				m_Camera.OffsetFocalPoint(m_Camera.GetForwardDirection());
				m_Camera.SetDistance(1.0f);
				m_Camera.SetDistance(1.0f);
			}
		}


	}


	void PerspectiveCameraController::OnKeyPressedEvent(int key, int action)
	{

	}

	void  PerspectiveCameraController::OnMouseButtonPressedEvent(int button, int action)
	{
		if (button == 0)
		{
			IsLeftMouseButtonHeld = true;

		}
		else if (button == 2)
		{
			IsMiddleMouseButtonHeld = true;
		}
		else if (button == 1)
		{
			IsRightMouseButtonHeld = true;
		}

	}

	void  PerspectiveCameraController::OnMouseButtonReleasedEvent(int button)
	{
		if (button == 0)
		{
			IsLeftMouseButtonHeld = false;

		}
		else if (button == 2)
		{
			IsMiddleMouseButtonHeld = false;
		}
		else if (button == 1)
		{
			IsRightMouseButtonHeld = false;
		}
	}

	void  PerspectiveCameraController::OnMouseScrollEvent(float x, float y)
	{
		float delta = y * .2f;

		float dist = m_Camera.GetDistance() * 0.2f;
		dist = std::max(dist, 0.0f);
		m_ZoomSpeed = dist * dist;
		m_ZoomSpeed = std::min(m_ZoomSpeed, 100.0f);
		m_Camera.OffsetDistance(-delta * m_ZoomSpeed);
		if (m_Camera.GetDistance() < 1.0f) {
			m_Camera.OffsetFocalPoint(m_Camera.GetForwardDirection());
			m_Camera.SetDistance(1.0f);
		}
	}

	void  PerspectiveCameraController::OnMouseMovedEvent(float x, float y)
	{
		m_MousePosition.x = x;
		m_MousePosition.y = y;
	}


}