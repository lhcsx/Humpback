// (c) Li Hongcheng
// 2022/02/06

#include "Camera.h"

using namespace DirectX;

namespace Humpback 
{
	Camera::Camera(float near, float far):
		m_near(near), m_far(far)
	{
	}

	XMFLOAT3 Camera::GetPosition()
	{
		return m_position;
	}

	XMMATRIX Camera::GetViewMatrix()
	{
		return XMLoadFloat4x4(&m_viewMatrix);
	}

	XMMATRIX Camera::GetProjectionMatrix()
	{
		return XMLoadFloat4x4(&m_projectionMatrix);
	}

	void Camera::SetPosition(float x, float y, float z)
	{
		m_position.x = x;
		m_position.y = y;
		m_position.z = z;
	}

	void Camera::UpdateViewMatrix(XMVECTOR cameraTarget)
	{
		XMVECTOR pos = XMVectorSet(m_position.x, m_position.y, m_position.z, 1.0);
		XMVECTOR up = DirectX::XMLoadFloat4(&m_up);

		XMMATRIX viewM = XMMatrixLookAtLH(pos, cameraTarget, up);
		DirectX::XMStoreFloat4x4(&m_viewMatrix, viewM);
	}

	void Camera::UpdateProjectionMatrix(float fov, float aspectRatio, float near, float far)
	{
		XMMATRIX p = XMMatrixPerspectiveFovLH(fov, aspectRatio, near, far);
		XMStoreFloat4x4(&m_projectionMatrix, p);
	}

}