#ifndef ENEMIGO_H
#define ENEMIGO_H

#include "Personaje.h"

class Enemigo : public Personaje
{
public:
    Enemigo(vector2 posicion, float masa,
            float radioVision, float anguloVision, float aperturaCono);
    virtual ~Enemigo() {}

    // ¿El punto objetivo está dentro del cono de visión?
    bool detecta(vector2 objetivo) const;

    // Consultas
    float radioVision() const  { return radioVision_; }

    float anguloVision() const { return anguloVision_; }

    float aperturaCono() const { return aperturaCono_; }

    // Modificadores (recalculan los valores precalculados)
    void setRadioVision(float radio);

    void setAnguloVision(float angulo);     // se llama cada fotograma en el nivel 2

    void setAperturaCono(float apertura);

protected:
    float radioVision_;

    float anguloVision_;    // hacia dónde mira (radianes)

    float aperturaCono_;    // ángulo total del cono (radianes)

private:
    // Valores precalculados para que detecta() sea barato

    float   radioCuadrado_;

    vector2 direccion_;         // (cos anguloVision, sin anguloVision)

    float   cosMediaApertura_;  // cos(aperturaCono / 2)
};

#endif // ENEMIGO_H