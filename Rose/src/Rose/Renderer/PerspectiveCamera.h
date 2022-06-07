#pragma once

#include <glm/glm.hpp>

namespace Rose
{

	class PerspectiveCamera
	{
		public:
			PerspectiveCamera(const glm::mat4& projection);


			void SetPosition(const glm::vec3& position);
			void SetRotation(const glm::vec3& rotation);

			void SetFocalPoint(const glm::vec3& point);
			void OffsetFocalPoint(const glm::vec3& point);

			void SetDistance(float value);
			void OffsetDistance(float value);

			void Move(const glm::vec3& dir);
			void Rotate(const glm::vec3& dir);

			void OffsetYaw(float value);
			void OffsetPitch(float value);


			const glm::vec3& GetPosition() const { return m_Position; }
			const glm::vec3& GetRotation() const { return m_Rotation; }

			const float& GetDistance() const { return m_Distance; }

			glm::vec3 GetForwardDirection();
			glm::vec3 GetRightDirection();
			glm::vec3 GetUpDirection();

			glm::quat GetOrientation();

			glm::mat4& GetProjView() { return m_ProjView; }
			glm::mat4& GetView() { return m_View; }
			glm::mat4& GetProj() { return m_Projection; }

			void RecalculateMatrix();

		private:
			glm::mat4 m_Projection, m_View;
			glm::mat4 m_ProjView;

			glm::vec3 m_Position, m_Rotation, m_FocalPoint;
			float m_Pitch, m_Yaw;
			float m_Distance;


	};


	class PerspectiveCameraController
	{
		public:
			PerspectiveCameraController(const PerspectiveCamera& camera);

			void OnUpdate(float ts);

			PerspectiveCamera& GetCam() { return m_Camera; }

			void OnKeyPressedEvent(int key, int action);
			void OnMouseButtonPressedEvent(int button, int action);
			void OnMouseButtonReleasedEvent(int button);
			void OnMouseScrollEvent(float x, float y);
			void OnMouseMovedEvent(float x, float y);
		private:
			float m_TimeStep;
			float m_PanSpeed, m_RotationSpeed, m_ZoomSpeed;
			glm::vec2 m_PrevMousePosition;
			glm::vec2 m_MousePosition;

			glm::vec2 m_MouseDelta;

			PerspectiveCamera m_Camera;

			bool IsLeftMouseButtonHeld = false;
			bool IsRightMouseButtonHeld = false;
			bool IsMiddleMouseButtonHeld = false;
	};
}