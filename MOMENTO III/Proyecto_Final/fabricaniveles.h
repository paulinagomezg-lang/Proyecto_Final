#ifndef FABRICANIVELES_H
#define FABRICANIVELES_H
#include "NivelDificultad.h"

class Nivel;

// Niveles implementados por ahora. Cuando esté NivelLaberinto, pasa a 2.
const int TOTAL_NIVELES = 1;

Nivel* crearNivel(int indice, NivelDificultad dificultad);

#endif // FABRICANIVELES_H
