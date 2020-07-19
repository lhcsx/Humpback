#include <iostream>
#include <windows.h>
#include "Vector4.h"
#include "Color.h"

using namespace std;
using namespace Humpback;

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    Color c1 = Color(1.0f, 2.0f, 3.0f, 1.0f);
    Color c2 = Color(4.0f, 4.0f, 4.0f, 1.0f);
    Color c3 = c1 * c2;
    
    return 0;
}