#ifndef MINIONEJERCITOCOLOMBIANO_H
#define MINIONEJERCITOCOLOMBIANO_H

#include "enemigo.h"

class MinionEjercitoColombiano : public Enemigo
{
public:
    MinionEjercitoColombiano(vector2 posicion, float rangoCaptura = 1.5f);

    bool puedeCapturar(vector2 posicionMario) const { return detecta(posicionMario); }

    void actualizar(float dt) override;
};

#endif // MINIONEJERCITOCOLOMBIANO_H
