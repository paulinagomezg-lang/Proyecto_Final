#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "vector2.h"

class Personaje
{
public:
    Personaje(vector2 posicion, float masa);

    virtual ~Personaje() {}

    // Copia prohibida
    Personaje(const Personaje&) = delete;

    Personaje& operator=(const Personaje&) = delete;

    // Cada clase hija define su comportamiento en cada fotograma.
    virtual void actualizar(float dt) = 0;

    // Suma una fuerza que se aplicará en el próximo integrar(dt).
    void aplicarFuerza(vector2 fuerza) { fuerzaAcumulada_ = fuerzaAcumulada_ + fuerza; }

    // Consultas (inline: se llaman muchas veces por fotograma)
    vector2 posicion() const  { return posicion_; }

    vector2 velocidad() const { return velocidad_; }

    float   masa() const      { return masa_; }

    // Modificadores
    void setPosicion(vector2 p)  { posicion_ = p; }

    void setVelocidad(vector2 v) { velocidad_ = v; }

    void setMasa(float masa);    // valida masa > 0

protected:
    // Avanza el personaje dt segundos (Euler semi-implícito).
    // Las clases hijas lo llaman dentro de su actualizar(dt).
    void integrar(float dt);

    vector2 posicion_;

    vector2 velocidad_;

    vector2 fuerzaAcumulada_;

    float   masa_;

    float   inversaMasa_;   // 1 / masa, precalculada
};

#endif // PERSONAJE_H