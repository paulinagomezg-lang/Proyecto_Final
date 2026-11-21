#include "minionejercitocolombiano.h"

using namespace std;

namespace {
const float PI = 3.14159265f;
}

MinionEjercitoColombiano::MinionEjercitoColombiano(vector2 posicion, float rangoCaptura)
    : Enemigo(posicion, 1.0f, rangoCaptura, PI / 2.0f, PI)
{
}


void MinionEjercitoColombiano::actualizar(float dt)
{
    if (!(dt > 0.0f)) return;
    integrar(dt);
}
