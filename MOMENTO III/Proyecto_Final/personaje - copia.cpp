#include "Personaje.h"
#include "Excepciones_del_Juego.h"

using namespace std;

Personaje::Personaje(vector2 posicion, float masa)
    : posicion_(posicion),

    velocidad_(0.0f, 0.0f),

    fuerzaAcumulada_(0.0f, 0.0f),

    masa_(1.0f),

    inversaMasa_(1.0f)
{
    setMasa(masa);   // valida y precalcula 1/masa
}

void Personaje::setMasa(float masa)
{
    Excepciones_de_Parametros_Fisicos::exigirPositivo("masa", masa);
    masa_ = masa;
    inversaMasa_ = 1.0f / masa;   // seguro: ya se sabe que masa > 0
}

void Personaje::integrar(float dt)
{
    // a = F / m  (se multiplica por la inversa en vez de dividir)
    vector2 aceleracion = fuerzaAcumulada_ * inversaMasa_;

    // Euler semi-implícito: primero la velocidad...
    velocidad_ = velocidad_ + aceleracion * dt;

    // ...y la posición con la velocidad ya actualizada.
    posicion_ = posicion_ + velocidad_ * dt;

    // Las fuerzas valen solo para este fotograma.
    fuerzaAcumulada_ = vector2(0.0f, 0.0f);
}