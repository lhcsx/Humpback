// (c) Li Hongcheng
// 2021-10-28


#pragma once


#include <DirectXMath.h>


namespace Humpback
{

	class Light
	{

	public:

		Light() = default;
		virtual ~Light() = default;

		Light(const Light& rhs) = delete;
		Light& operator=(const Light& rhs) = delete;

		DirectX::XMFLOAT3 GetIntensity() { return m_intensity; }
		
		void SetIntensity(DirectX::XMFLOAT3 intensity) { m_intensity = intensity; }
		void SetIntensity(float r, float g, float b) { m_intensity = DirectX::XMFLOAT3(r, g, b); }

	private:
		DirectX::XMFLOAT3 m_intensity = { 0.5f, 0.5f, 0.5f };
	};


	class DirectionalLight : public Light
	{

	public:
		DirectionalLight() = default;
		~DirectionalLight() = default;

		DirectionalLight(const DirectionalLight& rhs) = delete;
		DirectionalLight& operator=(const DirectionalLight& rhs) = delete;
		
		DirectX::XMFLOAT3 GetDirection() { return m_direction; }
		void SetDirection(DirectX::XMFLOAT3 direction) { m_direction = direction; }
		void SetDirection(float x, float y, float z) { m_direction = DirectX::XMFLOAT3(x, y, z); }


	private:
		DirectX::XMFLOAT3 m_direction = {0, -1, 0};
	};
}
