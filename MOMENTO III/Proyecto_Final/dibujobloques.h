#ifndef DIBUJOBLOQUES_H
#define DIBUJOBLOQUES_H

#include "vector2.h"
#include <QPointF>
#include <QRect>
#include <QPixmap>

class QPainter;
class NivelBloques;
class BloqueTierra;
class DibujoBloques
{
public:
    DibujoBloques();

    // area: rectángulo de la ventana donde va el nivel.
    // tiempo: segundos desde que abrió el programa (para parpadeos).
    void dibujar(QPainter& p, const NivelBloques& nivel, const QRect& area, float tiempo);

private:
    void prepararEscala(const NivelBloques& nivel, const QRect& area);
    void moverCamara(const NivelBloques& nivel);
    QPointF aPixel(vector2 posicion) const;
    float aPixelX(float x) const;
    float aPixelY(float y) const;
    bool esVisible(float xInicio, float xFin) const;

    void dibujarCielo(QPainter& p, const QRect& area) const;
    void dibujarFosos(QPainter& p, const NivelBloques& nivel) const;
    void dibujarMinions(QPainter& p, const NivelBloques& nivel, float tiempo) const;
    void cargarSprites();
    void prepararSprites();
    void dibujarBloques(QPainter& p, const NivelBloques& nivel, float tiempo) const;
    void dibujarBloque(QPainter& p, const BloqueTierra& b, float rigidezInicial, bool aviso) const;
    void dibujarItems(QPainter& p, const NivelBloques& nivel) const;
    void dibujarMeta(QPainter& p, const NivelBloques& nivel) const;
    void dibujarMario(QPainter& p, const NivelBloques& nivel, float tiempo) const;

    QRect area_;           // área para la que se calculó la escala
    float escala_;         // píxeles por metro
    float anchoMundo_;     // m, del primer al último bloque
    float anchoVisible_;   // m que caben a lo ancho
    float camaraX_;        // x del mundo en el borde izquierdo del área

    // Fondo de la refinería destruida: se lee una vez del .qrc y se escala
    // al alto del área solo cuando el área cambia (prepararEscala).
    QPixmap fondoOriginal_;
    QPixmap fondo_;

    // Sprites de los personajes (los mismos del laberinto). Originales leídos
    // una vez; escalados solo cuando cambia el área.
    static const int CUADROS = 3;
    QPixmap marioOriginal_[2][CUADROS];     // [0 derecha, 1 izquierda][cuadro]
    QPixmap minionOriginal_[CUADROS];       // de frente, mirando a Mario
    QPixmap capturadoOriginal_;
    QPixmap mario_[2][CUADROS];
    QPixmap marioPesado_[2][CUADROS];       // un poco más grande con la pesa
    QPixmap minion_[CUADROS];
    QPixmap capturado_;
};

#endif // DIBUJOBLOQUES_H