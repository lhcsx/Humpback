#include <string>
#include <ostream>
#include <fstream>

#include "Canvas.h"


using namespace std;

namespace Humpback 
{
	// ------------------------------------------------------------------------------------------
	Canvas::Canvas(int widht, int height)
	{
		mWidth = widht;
		mHeight = height;
		mpPixels = std::make_unique<std::vector<std::vector<Color>>>();
		Color c = Color(.5f, .5f, .5f, 1.0f);
		ResetCanvas(c);
	}
	// ------------------------------------------------------------------------------------------
	void Canvas::ResetCanvas(const Color& color)
	{
		for (size_t i = 0; i < mHeight; i++)
		{
			std::vector<Color> color_row;
			for (size_t j = 0; j < mWidth; j++)
			{
				if (mpPixels == nullptr)
				{
					break;
				}

				Color c = Color(color);
				color_row.push_back(c);
			}
			(*mpPixels).push_back(color_row);
		}
	}
	// ------------------------------------------------------------------------------------------
	void Canvas::WriteColor(int x, int y, const Color& c)
	{
		if (x < 0 || y < 0)
		{
			return;
		}

		x = x > mWidth ? mWidth : x;
		y - y > mHeight ? mHeight : y;

		(*mpPixels)[y][x] = c;
	}

	// ------------------------------------------------------------------------------------------
	void Canvas::Canvas2PPM()
	{
		/*
			P3
			width height
			255
		*/
		string ppmHeader = "P3\n" + to_string(mWidth) + " " + to_string(mHeight) + "\n" + "255\n";

		string colorData = "";
		for (size_t i = 0; i < (*mpPixels).size(); i++)
		{
			for (size_t j = 0; j < (*mpPixels)[i].size(); j++)
			{
				Color c = (*mpPixels)[i][j];
				colorData += (to_string(static_cast<int>(c.r * 255)) + " "
					+ to_string(static_cast<int>(c.g * 255)) + " "
					+ to_string(static_cast<int>(c.b * 255)) + " ");
			}
			colorData += "\n";
		}

		ofstream outStream;
		outStream.open("./Output/result.ppm");
		outStream << ppmHeader + colorData;
		outStream.close();
	}
}