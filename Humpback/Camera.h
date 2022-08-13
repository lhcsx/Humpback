// (c) Li Hongcheng
// 2022/02/06

//#pragma once
//
//#include <DirectXMath.h>
//
//#include "HMathHelper.h"
//
//namespace Humpback{
//
//	class Camera
//	{
//	public:
//
//		Camera();
//		~Camera();
//
//		// Get/Set world camera position.
//		DirectX::XMVECTOR GetPosition() const;
//		DirectX::XMFLOAT3 GetPosition3f() const;
//		void SetPosition(const DirectX::XMFLOAT3& position);
//		void SetPosition(float x, float y, float z);
//
//		// Get camera basis vectors.
//		DirectX::XMVECTOR GetRight() const;
//		DirectX::XMFLOAT3 GetRight3f() const;
//		DirectX::XMVECTOR GetUp() const;
//		DirectX::XMFLOAT3 GetUp3f() const;
//		DirectX::XMVECTOR GetForward() const;
//		DirectX::XMFLOAT3 GetForward3f() const;
//
//		// Get frustum properties.
//		float GetNear() const { return m_near; }
//		float GetFar() const { return m_far; }
//		float GetAspect() const { return m_aspect; }
//		float GetFovY() const { return m_fovY; }
//
//		// Set frustum.
//		void SetFrustum(float fovY, float aspect, float near, float far);
//
//		// Define camera space via lookat parameters.
//		void LookAt(DirectX::FXMVECTOR position, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
//		void LookAt(const DirectX::XMFLOAT3& positon, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& worldUp);
//
//		// Get view/projection matrix.
//		DirectX::XMMATRIX GetViewMatrix() const;
//		DirectX::XMMATRIX GetProjectionMatrix() const;
//
//		DirectX::XMFLOAT4X4 GetViewMatrix4x4() const;
//		DirectX::XMFLOAT4X4 GetProjectionMatrix4x4() const;
//
//		// After modifying camera opsition/orientation, call this function to rebuild the view matrix.
//		void UpdateViewMatrix();
//
//		// Walk/Strade the camera of distance d.
//		void Walk(float d);
//		void Strafe(float d);
//
//		// Rotate the camera.
//		void Pitch(float angle);
//		void RotateY(float angle);
//
//	private:
//
//		// Camera coordinate system with coordinates relative to world space.
//		DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
//		DirectX::XMFLOAT3 m_right = { 1.0f, 0.0f, 0.0f };
//		DirectX::XMFLOAT3 m_up = { 0.0f, 1.0f, 0.0f };
//		DirectX::XMFLOAT3 m_forward = {0.0f, 0.0f, 1.0f};
//
//		// Cache frustum properties.
//		float m_near = 0.0f;
//		float m_far = 0.0f;
//		float m_aspect = 0.0f;
//		float m_fovY = 0.0f;
//
//		bool m_viewDirty = true;
//
//		DirectX::XMFLOAT4X4 m_viewMatrix = HMathHelper::Identity4x4();
//		DirectX::XMFLOAT4X4 m_projectionMatrix = HMathHelper::Identity4x4();
//	};
//}
