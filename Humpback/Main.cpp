#include <iostream>
#include <windows.h>
#include "Vector4.h"

using namespace std;
using namespace Humpback;

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    Vector4 v1(1.1f, 2.2f, 3.3f, 0);
    Vector4 v2(2.0f, 2.0f, 2.0f, 1.0f);
    Vector4 v3 = v2 / 2;
    
    return 0;
}