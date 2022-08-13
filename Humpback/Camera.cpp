// (c) Li Hongcheng
// 2022/02/06

//#include "Camera.h"
//
//using namespace DirectX;
//
//namespace Humpback 
//{
//	Camera::Camera()
//	{
//	}
//
//	Camera::~Camera()
//	{
//	}
//
//	XMVECTOR Camera::GetPosition() const
//	{
//		return XMLoadFloat3(&m_position);
//	}
//	
//	XMFLOAT3 Camera::GetPosition3f() const
//	{
//		return m_position;
//	}
//
//	void Camera::SetPosition(const XMFLOAT3& position)
//	{
//		m_position = position;
//
//		m_viewDirty = true;
//	}
//
//	void Camera::SetPosition(float x, float y, float z)
//	{
//		m_position = XMFLOAT3(x, y, z);
//
//		m_viewDirty = true;
//	}
//
//	DirectX::XMVECTOR Camera::GetRight() const
//	{
//		return XMLoadFloat3(&m_right);
//	}
//
//	DirectX::XMFLOAT3 Camera::GetRight3f() const
//	{
//		return m_right;
//	}
//
//	DirectX::XMVECTOR Camera::GetUp() const
//	{
//		return XMLoadFloat3(&m_up);
//	}
//
//	DirectX::XMFLOAT3 Camera::GetUp3f() const
//	{
//		return m_up;
//	}
//
//	DirectX::XMVECTOR Camera::GetForward() const
//	{
//		return XMLoadFloat3(&m_forward);
//	}
//
//	DirectX::XMFLOAT3 Camera::GetForward3f() const
//	{
//		return m_forward;
//	}
//
//	void Camera::SetFrustum(float fovY, float aspect, float near, float far)
//	{
//		m_fovY = fovY;
//		m_aspect = aspect;
//		m_near = near;
//		m_far = far;
//
//		XMMATRIX pm = XMMatrixPerspectiveFovLH(fovY, aspect, near, far);
//		XMStoreFloat4x4(&m_projectionMatrix, pm);
//	}
//
//	void Camera::LookAt(FXMVECTOR position, FXMVECTOR target, FXMVECTOR worldUp)
//	{
//		XMVECTOR forward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, position));
//		XMVECTOR right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, forward));
//		XMVECTOR up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(forward, right));
//
//		XMStoreFloat3(&m_position, position);
//		XMStoreFloat3(&m_forward, forward);
//		XMStoreFloat3(&m_right, right);
//		XMStoreFloat3(&m_up, up);
//
//		m_viewDirty = true;
//	}
//
//	void Camera::LookAt(const DirectX::XMFLOAT3& positon, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& worldUp)
//	{
//		XMVECTOR p = XMLoadFloat3(&positon);
//		XMVECTOR t = XMLoadFloat3(&target);
//		XMVECTOR u = XMLoadFloat3(&worldUp);
//
//		LookAt(p, t, u);
//
//		m_viewDirty = true;
//	}
//
//	DirectX::XMMATRIX Camera::GetViewMatrix() const
//	{
//		return XMLoadFloat4x4(&m_viewMatrix);
//	}
//
//	XMMATRIX Camera::GetProjectionMatrix() const
//	{
//		return XMLoadFloat4x4(&m_projectionMatrix);
//	}
//
//	DirectX::XMFLOAT4X4 Camera::GetViewMatrix4x4() const
//	{
//		return m_viewMatrix;
//	}
//
//	DirectX::XMFLOAT4X4 Camera::GetProjectionMatrix4x4() const
//	{
//		return m_projectionMatrix;
//	}
//
//	void Camera::UpdateViewMatrix()
//	{
//		if (m_viewDirty)
//		{
//			XMVECTOR pos = XMLoadFloat3(&m_position);
//			XMVECTOR forward = XMLoadFloat3(&m_forward);
//			XMVECTOR up = XMLoadFloat3(&m_up);
//			XMVECTOR right = XMLoadFloat3(&m_right);
//
//			// Keep camera's axes orthogonal to each other and of unit length.
//			forward = XMVector3Normalize(forward);
//			up = XMVector3Normalize(up);
//
//			// Forward and up is already ortho-normal, so no need to normalize cross product.
//			right = XMVector3Cross(forward, up);
//
//			float x = -XMVectorGetX(XMVector3Dot(pos, right));
//			float y = -XMVectorGetX(XMVector3Dot(pos, up));
//			float z = -XMVectorGetX(XMVector3Dot(pos, forward));
//
//			XMStoreFloat3(&m_right, right);
//			XMStoreFloat3(&m_forward, forward);
//			XMStoreFloat3(&m_up, up);
//
//			m_viewMatrix(0, 0) = m_right.x;
//			m_viewMatrix(1, 0) = m_right.y;
//			m_viewMatrix(2, 0) = m_right.z;
//			m_viewMatrix(3, 0) = x;
//
//			m_viewMatrix(0, 1) = m_up.x;
//			m_viewMatrix(1, 1) = m_up.y;
//			m_viewMatrix(2, 1) = m_up.z;
//			m_viewMatrix(3, 1) = y;
//
//			m_viewMatrix(0, 2) = m_forward.x;
//			m_viewMatrix(1, 2) = m_forward.y;
//			m_viewMatrix(2, 2) = m_forward.z;
//			m_viewMatrix(3, 2) = z;
//
//			m_viewMatrix(0, 3) = m_right.x;
//			m_viewMatrix(1, 3) = m_right.y;
//			m_viewMatrix(2, 3) = m_right.z;
//			m_viewMatrix(3, 3) = 1.0f;
//
//			m_viewDirty = false;
//		}
//	}
//
//	void Camera::Walk(float d)
//	{
//		XMVECTOR distance = XMVectorReplicate(d);
//		XMVECTOR forward = XMLoadFloat3(&m_forward);
//		XMVECTOR pos = XMLoadFloat3(&m_position);
//
//		XMVECTOR newPos = XMVectorMultiplyAdd(forward, distance, pos);
//		XMStoreFloat3(&m_position, newPos);
//
//		m_viewDirty = true;
//	}
//
//	void Camera::Strafe(float d)
//	{
//		XMVECTOR distance = XMVectorReplicate(d);
//		XMVECTOR right = XMLoadFloat3(&m_right);
//		XMVECTOR pos = XMLoadFloat3(&m_position);
//
//		XMVECTOR newPos = XMVectorMultiplyAdd(right, distance, pos);
//		XMStoreFloat3(&m_position, newPos);
//
//		m_viewDirty = true;
//	}
//
//	void Camera::Pitch(float angle)
//	{
//		XMVECTOR right = XMLoadFloat3(&m_right);
//		XMVECTOR forward = XMLoadFloat3(&m_forward);
//		XMVECTOR up = XMLoadFloat3(&m_up);
//
//		XMMATRIX rotationMatrix = XMMatrixRotationAxis(right, angle);
//		XMStoreFloat3(&m_forward, XMVector3TransformNormal(forward, rotationMatrix));
//		XMStoreFloat3(&m_up, XMVector3TransformNormal(up, rotationMatrix));
//
//		m_viewDirty = true;
//	}
//
//	void Camera::RotateY(float angle)
//	{
//		XMMATRIX rotationMatrix = XMMatrixRotationY(angle);
//
//		XMVECTOR right = XMLoadFloat3(&m_right);
//		XMVECTOR forward = XMLoadFloat3(&m_forward);
//		XMVECTOR up = XMLoadFloat3(&m_up);
//
//		XMStoreFloat3(&m_forward, XMVector3TransformNormal(forward, rotationMatrix));
//		XMStoreFloat3(&m_up, XMVector3TransformNormal(up, rotationMatrix));
//		XMStoreFloat3(&m_right, XMVector3TransformNormal(right, rotationMatrix));
//
//		m_viewDirty = true;
//	}
//}