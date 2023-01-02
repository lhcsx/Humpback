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

	DirectX::XMFLOAT3 Camera::GetForward()
	{
		return m_forward;
	}

	DirectX::XMVECTOR Camera::GetForwardVector()
	{
		return XMLoadFloat3(&m_forward);
	}

	DirectX::XMVECTOR Camera::GetRightVector()
	{
		return XMLoadFloat3(&m_right);
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

		m_viewDirty = true;
	}

	void Camera::SetPosition(DirectX::XMVECTOR pos)
	{
		XMStoreFloat3(&m_position, pos);
		m_viewDirty = true;
	}

	void Camera::SetForward(float x, float y, float z)
	{
		m_forward.x = x;
		m_forward.y = y;
		m_forward.z = z;

		m_viewDirty = true;
	}

	void Camera::SetRight(DirectX::XMVECTOR right)
	{
		XMStoreFloat3(&m_right, right);

		m_viewDirty = true;
	}

	void Camera::Update()
	{
		UpdateViewMatrix();
	}

	void Camera::UpdateViewMatrix()
	{
		if (m_viewDirty == false)
		{
			return;
		}

		XMVECTOR right = XMLoadFloat3(&m_right);
		XMVECTOR forward = XMLoadFloat3(&m_forward);
		XMVECTOR up = XMLoadFloat3(&m_up);
		XMVECTOR pos = XMLoadFloat3(&m_position);

		forward = XMVector3Normalize(forward);
		up = XMVector3Normalize(XMVector3Cross(forward, right));

		right = XMVector3Cross(up, forward);

		XMStoreFloat3(&m_forward, forward);
		XMStoreFloat3(&m_right, right);
		XMStoreFloat3(&m_up, up);

		float x = -XMVectorGetX(XMVector3Dot(pos, right));
		float y = -XMVectorGetX(XMVector3Dot(pos, up));
		float z = -XMVectorGetX(XMVector3Dot(pos, forward));

		m_viewMatrix(0, 0) = m_right.x;
		m_viewMatrix(1, 0) = m_right.y;
		m_viewMatrix(2, 0) = m_right.z;
		m_viewMatrix(3, 0) = x;

		m_viewMatrix(0, 1) = m_up.x;
		m_viewMatrix(1, 1) = m_up.y;
		m_viewMatrix(2, 1) = m_up.z;
		m_viewMatrix(3, 1) = y;

		m_viewMatrix(0, 2) = m_forward.x;
		m_viewMatrix(1, 2) = m_forward.y;
		m_viewMatrix(2, 2) = m_forward.z;
		m_viewMatrix(3, 2) = z;

		m_viewMatrix(0, 3) = 0.0f;
		m_viewMatrix(1, 3) = 0.0f;
		m_viewMatrix(2, 3) = 0.0f;
		m_viewMatrix(3, 3) = 1.0f;

	}

	void Camera::UpdateProjectionMatrix(float fov, float aspectRatio, float near, float far)
	{
		XMMATRIX p = XMMatrixPerspectiveFovLH(fov, aspectRatio, near, far);
		XMStoreFloat4x4(&m_projectionMatrix, p);
	}

}