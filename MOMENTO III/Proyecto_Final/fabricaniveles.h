#ifndef FABRICANIVELES_H
#define FABRICANIVELES_H

#include "NivelDificultad.h"

class Nivel;


// Cuántos niveles tiene el juego.
int totalNiveles();

// Crea el nivel 'indice' (0 = primero). Quien lo recibe (gestorlogico)
// es su dueño y lo borra. Lanza Excepciones_de_Recursos si no se puede
// leer el mapa, o Excepciones_del_Mapa si está mal escrito.
Nivel* crearNivel(int indice, NivelDificultad dificultad);

#endif // FABRICANIVELES_H