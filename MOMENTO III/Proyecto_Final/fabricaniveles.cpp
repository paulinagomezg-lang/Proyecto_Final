#include "fabricaniveles.h"
#include "nivelbloques.h"
#include "Excepciones_del_Juego.h"
#include <string>

using namespace std;

Nivel* crearNivel(int indice, NivelDificultad dificultad)
{
    switch (indice) {
    case 0:
        return new NivelBloques(NivelBloques::mapaPorDefecto(dificultad), dificultad);
    // case 1: return new NivelLaberinto(...);   // cuando tu compañera lo termine
    default:
        throw Excepciones_del_Juego("El nivel " + to_string(indice + 1) + " todavia no existe");
    }
}