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

    // Sprites: se leen del .qrc UNA vez (constructor) y se re-escalan solo
    // cuando cambia el tamaño de la ventana (prepararSprites), nunca por fotograma.
    void cargarSprites();
    void prepararSprites();
    void dibujarSprite(QPainter& p, const QPixmap& sprite, QPointF centro, float radio) const;

    void dibujarBloques(QPainter& p, const NivelLaberinto& nivel) const;
    void dibujarGuardias(QPainter& p, const NivelLaberinto& nivel, float tiempo) const;
    void dibujarMario(QPainter& p, const NivelLaberinto& nivel, float tiempo) const;
    void dibujarApagon(QPainter& p, const NivelLaberinto& nivel, const QRect& area, float tiempo) const;

    QPixmap fondo_;                 // paredes y piso, dibujados una sola vez
    const MapaLaberinto* mapaFondo_; // de qué mapa es el fondo guardado (sin dueño)
    QRect areaFondo_;               // y para qué área
    float escala_;                  // píxeles por metro
    QPointF origen_;                // esquina superior izquierda del mapa en pantalla
    float altoMundo_;               // alto del mapa en metros

    // [direccion][cuadro]: direccion 0 derecha, 1 izquierda, 2 arriba, 3 abajo;
    // cuadro 0 quieto, 1 y 2 pasos al caminar.
    static const int DIRECCIONES = 4;
    static const int CUADROS = 3;
    QPixmap protagonistaOriginal_[DIRECCIONES][CUADROS];
    QPixmap enemigoOriginal_[DIRECCIONES][CUADROS];
    QPixmap dormidoOriginal_, capturadoOriginal_, cajaOriginal_;
    // Varias versiones de pared y piso para que el fondo no se vea repetido.
    static const int VARIANTES_PARED = 3;
    static const int VARIANTES_PISO = 4;
    QPixmap paredOriginal_[VARIANTES_PARED];
    QPixmap pisoOriginal_[VARIANTES_PISO];
    QPixmap salidaOriginal_;

    // Las mismas imágenes ya escaladas al tamaño de la pantalla actual.
    QPixmap protagonista_[DIRECCIONES][CUADROS];
    QPixmap enemigo_[DIRECCIONES][CUADROS];
    QPixmap dormido_, capturado_, caja_;
    int ladoSprite_;                // píxeles del sprite escalado
};

#endif // DIBUJOLABERINTO_H