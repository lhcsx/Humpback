#include <iostream>
#include <windows.h>
#include "Vector4.h"

using namespace std;
using namespace Humpback;

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    Vector4 v1(1.0f, 2.0f, 3.0f, 0);
    Vector4 v2(2.0f, 3.0f, 4.0f, .0f);
    Vector4 v3 = v2.Cross(v1);
    float f = v2.Normalized().Magnitude();
    
    return 0;
}