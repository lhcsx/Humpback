// (c) Li Hongcheng
// 2022/02/06

#pragma once

#include <DirectXMath.h>

#include "HMathHelper.h"

namespace Humpback{

	class Camera
	{
	public:

		Camera(float near, float far);
		Camera(const Camera&) = delete;
		Camera& operator= (const Camera&) = delete;
		~Camera() = default;


		DirectX::XMFLOAT3 GetPosition();


		DirectX::XMMATRIX GetViewMatrix();
		DirectX::XMMATRIX GetProjectionMatrix();

		void SetPosition(float x, float y, float z);
		void UpdateViewMatrix(DirectX::XMVECTOR cameraTarget);
		void UpdateProjectionMatrix(float fov, float aspectRatio, float near, float far);


	private:
		float m_near = 0;
		float m_far = 0;

		DirectX::XMFLOAT4X4 m_viewMatrix = HMathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 m_projectionMatrix = HMathHelper::Identity4x4();
		DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };

		DirectX::XMFLOAT4 m_up = { 0.0f, 1.0f, 0.0f, 0.0f };
	};
}
