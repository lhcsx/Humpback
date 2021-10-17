#pragma once

#include<d3d12.h>
#include<wrl.h>


namespace Humpback {
	class Renderer
	{
	public:

		Renderer();
		~Renderer()
		{

		}

		void Initialize();
		void Update();
		void Render();
		void Tick();
		void ShutDown();

	private:
		void Clear();
		void LoadPipeline();

		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	};
}

