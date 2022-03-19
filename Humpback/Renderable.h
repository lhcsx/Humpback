// Li Hongcheng
// 2022/03/09



#pragma once

namespace Humpback
{
	class Renderable
	{
	public:
		Renderable();
		virtual ~Renderable();
		

		virtual void Initialize() = 0;
		virtual void Render() = 0;
	};
}

