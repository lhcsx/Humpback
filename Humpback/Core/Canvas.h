#pragma once

#include <vector>
#include <memory>

#include "Color.h"

namespace Humpback 
{
	class Canvas
	{
	public:
		Canvas(int, int);
		Canvas(const Canvas&) = delete;
		Canvas& operator=(const Canvas&) = delete;

		void ResetCanvas(const Color&);
		void WriteColor(int x, int y, const Color&);
		void Canvas2PPM();

	private:
		int mWidth;
		int mHeight;

		std::unique_ptr<std::vector<std::vector<Color>>> mpPixels;
	};
}

