#include "BloqueMovil.h"
#include "Excepciones_del_Juego.h"

using namespace std;

BloqueMovil::BloqueMovil(vector2 centro, vector2 eje, float amplitud, float masa)
    : Personaje(centro, masa),          // Personaje valida masa > 0
    centro_(centro),
    eje_(eje.normalizado()),
    amplitud_(amplitud)
{
    Excepciones_de_Parametros_Fisicos::exigirNoNegativo("amplitud", amplitud);
    if (eje_.normaCuadrada() == 0.0f)
        throw Excepciones_de_Parametros_Fisicos("Eje del bloque", 0.0f, "no puede ser (0,0)");

    // Empieza en un extremo, quieto: así oscila entre -amplitud y +amplitud
    posicion_ = centro_ + eje_ * amplitud_;
}

void BloqueMovil::ajustarAlRiel()
{
    float s = desplazamiento();
    float v = velocidad_.punto(eje_);
    // Un golpe no puede sacarlo de su riel: el riel mide 'amplitud' a cada
    // lado (el nivel la calculó para que no se meta en las paredes)
    if (s > amplitud_)       { s = amplitud_;  if (v > 0.0f) v = 0.0f; }
    else if (s < -amplitud_) { s = -amplitud_; if (v < 0.0f) v = 0.0f; }
    posicion_ = centro_ + eje_ * s;
    velocidad_ = eje_ * v;
}