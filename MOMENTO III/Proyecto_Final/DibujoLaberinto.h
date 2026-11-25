#ifndef DIBUJOLABERINTO_H
#define DIBUJOLABERINTO_H

#include "vector2.h"
#include <QPixmap>
#include <QPointF>
#include <QRect>

class QPainter;
class NivelLaberinto;
class MapaLaberinto;

class DibujoLaberinto
{
public:
    DibujoLaberinto();

    // area: rectángulo de la ventana donde va el mapa.
    // tiempo: segundos desde que empezó el programa (para parpadeos).
    void dibujar(QPainter& p, const NivelLaberinto& nivel, const QRect& area, float tiempo);

private:
    void prepararEscala(const MapaLaberinto& mapa, const QRect& area);
    void prepararFondo(const MapaLaberinto& mapa);
    QPointF aPixel(vector2 posicion) const;

    void dibujarBloques(QPainter& p, const NivelLaberinto& nivel) const;
    void dibujarGuardias(QPainter& p, const NivelLaberinto& nivel) const;
    void dibujarMario(QPainter& p, const NivelLaberinto& nivel, float tiempo) const;
    void dibujarApagon(QPainter& p, const NivelLaberinto& nivel, const QRect& area, float tiempo) const;

    QPixmap fondo_;                 // paredes y piso, dibujados una sola vez
    const MapaLaberinto* mapaFondo_; // de qué mapa es el fondo guardado (sin dueño)
    QRect areaFondo_;               // y para qué área
    float escala_;                  // píxeles por metro
    QPointF origen_;                // esquina superior izquierda del mapa en pantalla
    float altoMundo_;               // alto del mapa en metros
};

#endif // DIBUJOLABERINTO_H
