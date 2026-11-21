#ifndef FOSO_H
#define FOSO_H

struct Foso
{
    float xInicio;
    float xFin;
    float profundidad;

    bool contieneX(float x) const { return x > xInicio && x < xFin; }
    float fondo() const { return -profundidad; }
};

#endif // FOSO_H
