#include <iostream>
#include <windows.h>

#include"Canvas.h"
#include"Matrix4x4.h"

using namespace std;
using namespace Humpback;

INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    /*Canvas canvas(20, 10);
    Color red(1.0f, .0f, .0f);
    canvas.WriteColor(0, 0, red);

    canvas.Canvas2PPM();*/

    Vector4 v0 = { 1.f, 2.f, 3.f, 4.f };
    Vector4 v1 = { 5.f, 6.f, 7.f, 8.f };
    Vector4 v2 = { 9.f, 8.f, 7.f, 6.f };
    Vector4 v3 = { 5.f, 4.f, 3.f, 2.f };

    Vector4 v4 = { 2.f, 2.f, 3.f, 4.f };


    Vector4 v5 = { 1.f, .0f, .0f, .0f };
    Vector4 v6 = { .0f, 1.0f, .0f, .0f };
    Vector4 v7 = { .0f, .0f, 1.f, .0f };
    Vector4 v8 = { .0f, .0f, .0f, 1.f };

    Vector4 v9 = { 1.f, 2.f, 3.f, 1.f };

    Matrix4x4 m0 = { v0, v1, v2, v3 };
    Matrix4x4 m1 = { v0, v1, v2, v4 };
    Matrix4x4 m2 = { 1.f, 2.f, 3.f, 4.f,
                      2.f, 4.f, 4.f, 2.f,
                      8.f, 6.f, 4.f, 1.f,
                      0.f, 0.f, 0.f, 1.f };
    bool res = (m0 == m1);

    Matrix4x4 m3 = m0 * m2;

    Matrix4x4 m = Matrix4x4::Identity();

    Matrix4x4 test_v = m2.Transpose();

    return 0;
}