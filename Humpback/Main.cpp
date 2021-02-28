#include <iostream>
#include <windows.h>

#include "Canvas.h"
#include "Matrix4x4.h"
#include "Ray.h"
#include "Sphere.h"

using namespace std;
using namespace Humpback;

#define PI 3.1415926


INT WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR lpCmdLine, INT nCmdShow)
{
    /*Canvas canvas(20, 10);
    Color red(1.0f, .0f, .0f);
    canvas.WriteColor(0, 0, red);

    canvas.Canvas2PPM();*/

    Ray ray(Vector4(0, .9f, -5, 1), Vector4(0, 0, 1));

    Sphere sp;
    HitResult hitResult;
    bool bHit = sp.Intersect(ray, hitResult);

    return 0;
}