#include "DibujoLaberinto.h"
#include "NivelLaberinto.h"
#include "MapaLaberinto.h"
#include "MinionGuardia.h"
#include "BloqueMovil.h"
#include "protagonista.h"

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QFont>
#include <algorithm>
#include <cmath>

using namespace std;

namespace {

const float GRADOS_POR_RADIAN = 57.2957795f;

// Colores (cuando haya sprites, la mayoría de estos desaparecen).
const QColor COLOR_PISO(43, 43, 58);
const QColor COLOR_PARED(96, 96, 120);
const QColor COLOR_BORDE_PARED(70, 70, 90);
const QColor COLOR_SALIDA(63, 191, 95);
const QColor COLOR_BLOQUE(139, 90, 43);
const QColor COLOR_MINION(250, 215, 40);
const QColor COLOR_MARIO(220, 40, 40);

// Color del cono según lo que está haciendo el agente.
QColor colorCono(EstadoAgente estado)
{
    switch (estado) {
    case EstadoAgente::PATRULLAR: return QColor(120, 220, 120, 45);
    case EstadoAgente::SOSPECHAR: return QColor(255, 170, 0, 70);
    case EstadoAgente::PERSEGUIR: return QColor(255, 40, 40, 80);
    case EstadoAgente::BUSCAR:    return QColor(170, 90, 255, 60);
    }
    return QColor(120, 220, 120, 45);
}

} // namespace

DibujoLaberinto::DibujoLaberinto()
    : mapaFondo_(nullptr), escala_(1.0f), origen_(0.0, 0.0), altoMundo_(1.0f)
{
}


// Conversión mundo (metros, y arriba) -> pantalla (píxeles, y abajo)


void DibujoLaberinto::prepararEscala(const MapaLaberinto& mapa, const QRect& area)
{
    const float anchoMundo = static_cast<float>(mapa.ancho()) * mapa.tamCelda();
    altoMundo_ = static_cast<float>(mapa.alto()) * mapa.tamCelda();
    escala_ = min(static_cast<float>(area.width()) / anchoMundo,
                  static_cast<float>(area.height()) / altoMundo_);
    // Centrado en el área.
    const float sobraX = static_cast<float>(area.width()) - anchoMundo * escala_;
    const float sobraY = static_cast<float>(area.height()) - altoMundo_ * escala_;
    origen_ = QPointF(area.x() + sobraX * 0.5f, area.y() + sobraY * 0.5f);
}

QPointF DibujoLaberinto::aPixel(vector2 posicion) const
{
    return QPointF(origen_.x() + posicion.x * escala_,
                   origen_.y() + (altoMundo_ - posicion.y) * escala_);
}


// Fondo: piso, paredes y salida (se dibuja una sola vez)

void DibujoLaberinto::prepararFondo(const MapaLaberinto& mapa)
{
    const float lado = mapa.tamCelda() * escala_;   // píxeles por celda
    const int ancho = static_cast<int>(ceil(mapa.ancho() * lado));
    const int alto = static_cast<int>(ceil(mapa.alto() * lado));
    fondo_ = QPixmap(max(ancho, 1), max(alto, 1));
    fondo_.fill(COLOR_PISO);

    QPainter p(&fondo_);
    p.setPen(QPen(COLOR_BORDE_PARED, 1));
    p.setBrush(COLOR_PARED);
    for (int f = 0; f < mapa.alto(); ++f)
        for (int c = 0; c < mapa.ancho(); ++c)
            if (mapa.esPared(c, f))
                p.drawRect(QRectF(c * lado, f * lado, lado, lado));

    // La salida.
    const MapaLaberinto::Celda s = mapa.celdaSalida();
    p.setPen(Qt::NoPen);
    p.setBrush(COLOR_SALIDA);
    p.drawRect(QRectF(s.columna * lado, s.fila * lado, lado, lado));
    p.setPen(Qt::white);
    QFont fuente = p.font();
    fuente.setPixelSize(max(8, static_cast<int>(lado * 0.5f)));
    fuente.setBold(true);
    p.setFont(fuente);
    p.drawText(QRectF(s.columna * lado, s.fila * lado, lado, lado), Qt::AlignCenter, "S");
}


// Dibujo completo de un fotograma


void DibujoLaberinto::dibujar(QPainter& p, const NivelLaberinto& nivel, const QRect& area, float tiempo)
{
    const MapaLaberinto& mapa = nivel.mapa();

    // Solo se recalcula la escala y el fondo si cambió el área o el mapa.
    if (mapaFondo_ != &mapa || areaFondo_ != area) {
        prepararEscala(mapa, area);
        prepararFondo(mapa);
        mapaFondo_ = &mapa;
        areaFondo_ = area;
    }

    p.drawPixmap(origen_, fondo_);
    p.setRenderHint(QPainter::Antialiasing, true);
    dibujarBloques(p, nivel);
    dibujarGuardias(p, nivel);
    dibujarMario(p, nivel, tiempo);
    dibujarApagon(p, nivel, area, tiempo);
    p.setRenderHint(QPainter::Antialiasing, false);
}

void DibujoLaberinto::dibujarBloques(QPainter& p, const NivelLaberinto& nivel) const
{
    const float lado = 2.0f * NivelLaberinto::RADIO_BLOQUE * escala_;
    p.setPen(QPen(COLOR_BLOQUE.darker(150), 2));
    p.setBrush(COLOR_BLOQUE);
    for (const BloqueMovil* b : nivel.bloques()) {
        const QPointF c = aPixel(b->posicion());
        p.drawRect(QRectF(c.x() - lado * 0.5f, c.y() - lado * 0.5f, lado, lado));
    }
}

//Cada Minion: su cono de visión (el color dice qué está haciendo el agente), su cuerpo y, si está alerta, un "!" que se llena.
//Ángulos: en Qt 0° apunta a la derecha y los ángulos positivos giran en sentido antihorario en la pantalla, igual que en la lógica (y arriba), así que el ángulo del Minion se usa tal cual, pasado a grados.

void DibujoLaberinto::dibujarGuardias(QPainter& p, const NivelLaberinto& nivel) const
{
    const float radioCuerpo = NivelLaberinto::RADIO_MINION * escala_;
    QFont fuente = p.font();
    fuente.setBold(true);
    fuente.setPixelSize(max(10, static_cast<int>(radioCuerpo * 1.6f)));

    for (const MinionGuardia* g : nivel.guardias()) {
        const QPointF c = aPixel(g->posicion());

        // Cono de visión.
        const float r = g->radioVision() * escala_;
        const float inicio = (g->anguloVision() - g->aperturaCono() * 0.5f) * GRADOS_POR_RADIAN;
        const float barrido = g->aperturaCono() * GRADOS_POR_RADIAN;
        QPainterPath cono;
        cono.moveTo(c);
        cono.arcTo(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r), inicio, barrido);
        cono.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(colorCono(g->estado()));
        p.drawPath(cono);

        // Cuerpo.
        p.setPen(QPen(Qt::black, 1.5));
        p.setBrush(COLOR_MINION);
        p.drawEllipse(c, radioCuerpo, radioCuerpo);

        // Alerta: "!" que se pone rojo a medida que se llena.
        if (g->alerta() > 0.0f) {
            const int rojo = 255;
            const int verde = static_cast<int>(200 * (1.0f - g->alerta()));
            p.setPen(QColor(rojo, verde, 0));
            p.setFont(fuente);
            p.drawText(QRectF(c.x() - radioCuerpo, c.y() - 3.2f * radioCuerpo, 2 * radioCuerpo, 2 * radioCuerpo),
                       Qt::AlignCenter, "!");
        }
    }
}

//Mario: un círculo rojo con un punto blanco hacia donde mira (cuando haya sprites, aquí se elige la imagen según mirada()).
//Dormido: más oscuro y con "Zzz". Capturado: una X.

void DibujoLaberinto::dibujarMario(QPainter& p, const NivelLaberinto& nivel, float tiempo) const
{
    const Protagonista& m = *nivel.protagonista();
    const QPointF c = aPixel(m.posicion());
    const float r = NivelLaberinto::RADIO_MARIO * escala_;

    p.setPen(QPen(Qt::black, 1.5));
    p.setBrush(m.estaDormido() ? COLOR_MARIO.darker(170) : COLOR_MARIO);
    p.drawEllipse(c, r, r);

    // Hacia dónde mira (en la pantalla y crece hacia abajo).
    QPointF ojo(0.0, 0.0);
    switch (m.mirada()) {
    case Mirada::DERECHA:   ojo = QPointF(0.55 * r, 0.0);  break;
    case Mirada::IZQUIERDA: ojo = QPointF(-0.55 * r, 0.0); break;
    case Mirada::ARRIBA:    ojo = QPointF(0.0, -0.55 * r); break;
    case Mirada::ABAJO:     ojo = QPointF(0.0, 0.55 * r);  break;
    }
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    p.drawEllipse(c + ojo, 0.25 * r, 0.25 * r);

    if (m.estaDormido()) {
        // "Zzz" que sube y baja.
        const float sube = 0.3f * r * sin(tiempo * 3.0f);
        QFont fuente = p.font();
        fuente.setBold(true);
        fuente.setPixelSize(max(10, static_cast<int>(r * 1.3f)));
        p.setFont(fuente);
        p.setPen(QColor(200, 220, 255));
        p.drawText(QPointF(c.x() + r, c.y() - r - sube), "Zzz");
    }
    if (m.estaCapturado()) {
        p.setPen(QPen(Qt::white, 3));
        p.drawLine(c + QPointF(-r, -r), c + QPointF(r, r));
        p.drawLine(c + QPointF(-r, r), c + QPointF(r, -r));
    }
}

//Apagón: todo queda negro menos un círculo alrededor de Mario (radioVisibleMario metros). Se hace con un camino que tiene el
//rectángulo completo y el círculo, rellenado con la regla "par-impar", lo que está dentro de los dos (el círculo) queda sin pintar.
//Antes del apagón, la pantalla parpadea como aviso.

void DibujoLaberinto::dibujarApagon(QPainter& p, const NivelLaberinto& nivel, const QRect& area, float tiempo) const
{
    if (nivel.lucesApagadas()) {
        const QPointF c = aPixel(nivel.protagonista()->posicion());
        const float r = nivel.radioVisibleMario() * escala_;
        QPainterPath oscuridad;
        oscuridad.setFillRule(Qt::OddEvenFill);
        oscuridad.addRect(QRectF(area));
        oscuridad.addEllipse(c, r, r);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 235));
        p.drawPath(oscuridad);
    } else if (nivel.avisoApagon()) {
        // Parpadeo: 4 veces por segundo.
        if (static_cast<int>(tiempo * 8.0f) % 2 == 0)
            p.fillRect(area, QColor(0, 0, 0, 120));
    }
}
