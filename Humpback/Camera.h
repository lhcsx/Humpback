// (c) Li Hongcheng
// 2022/02/06

#pragma once

#include <DirectXMath.h>
#include <DirectXCollision.h>

#include "HMathHelper.h"

namespace Humpback
{

	class Camera
	{
	public:

		Camera();
		Camera(const Camera&) = delete;
		Camera& operator= (const Camera&) = delete;
		~Camera() = default;

		void Pitch(float radians);
		void RotateY(float radians);

		DirectX::XMFLOAT3 GetPosition();
		DirectX::XMFLOAT3 GetForward();
		DirectX::XMVECTOR GetForwardVector();
		DirectX::XMVECTOR GetRightVector();

		DirectX::XMMATRIX GetViewMatrix();
		DirectX::XMMATRIX GetProjectionMatrix();

		DirectX::BoundingFrustum& GetFrustum();

		void SetPosition(float x, float y, float z);
		void SetPosition(DirectX::XMVECTOR pos);
		void SetForward(float x, float y, float z);
		void SetRight(DirectX::XMVECTOR right);

		void SetFrustum(float fovY, float aspect, float near, float far);

		float GetNearZ();
		float GetFarZ();

		void Update();
		void UpdateViewMatrix();


	private:
		void _updateProjectionMatrix();
		
		bool m_viewDirty = false;

		float m_near = 0.0f;
		float m_far = 0.0f;
		float m_fovY = 0.0f;
		float m_aspect = 0.0f;

		DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT3 m_forward = { 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 m_right = { 1.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4X4 m_viewMatrix = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 m_projectionMatrix = HMathHelper::Identity4x4();

		DirectX::XMFLOAT3 m_up = { 0.0f, 1.0f, 0.0f};

		DirectX::BoundingFrustum m_frustum;
	};
}
