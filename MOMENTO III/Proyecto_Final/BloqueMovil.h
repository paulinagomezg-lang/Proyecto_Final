#ifndef BLOQUEMOVIL_H
#define BLOQUEMOVIL_H

#include "personaje.h"


class BloqueMovil : public Personaje
{
public:
    // centro: punto de equilibrio del resorte
    // eje: dirección del riel (se normaliza). (1,0) horizontal, (0,1) vertical
    // amplitud: qué tan lejos del centro empieza (m). Empieza quieto en
    //           centro + eje * amplitud, así que oscila entre -amplitud y +amplitud
    // Lanza Excepciones_de_Parametros_Fisicos si masa <= 0, amplitud < 0 o el eje es (0,0)
    BloqueMovil(vector2 centro, vector2 eje, float amplitud, float masa);

    void actualizar(float dt) override { integrar(dt); }

    vector2 centro() const { return centro_; }
    vector2 eje() const { return eje_; }
    float amplitud() const { return amplitud_; }
    // Desplazamiento sobre el riel respecto al centro (m, con signo).
    float desplazamiento() const { return (posicion_ - centro_).punto(eje_); }

    // Lo deja sobre su riel: quita lo que se haya movido de lado (por un
    // choque) en posición y velocidad, y no lo deja pasar de los extremos
    // Lo llama el nivel después de un choque (no en cada fotograma)
    void ajustarAlRiel();

private:
    vector2 centro_;
    vector2 eje_;
    float amplitud_;
};

#endif // BLOQUEMOVIL_H
