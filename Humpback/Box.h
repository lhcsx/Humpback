// Li Hongcheng
// 2022/03/09

#pragma once

#include "Renderable.h"
#include "Mesh.h"

namespace Humpback
{

	class Box : public Renderable
	{

	public:
		Box();
		~Box();

		virtual void Initialize() override;
		virtual void Render() override;


	private:

		void _generateMesh(float width, float height, float depth);


		Mesh* m_pMesh;
	};
}