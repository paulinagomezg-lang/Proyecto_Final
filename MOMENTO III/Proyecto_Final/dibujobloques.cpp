#include "DibujoBloques.h"
#include "nivelbloques.h"
#include "bloquetierra.h"
#include "itempoder.h"
#include "foso.h"
#include "minionejercitocolombiano.h"
#include "Protagonista.h"

#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QColor>
#include <QPen>
#include <QFont>
#include <algorithm>
#include <cmath>

using namespace std;

namespace {

const float GRADOS_POR_RADIAN = 57.2957795f;

// Lo que se ve de alto: de un poco por debajo del fondo de los fosos
// hasta el cielo.
const float Y_ARRIBA = 5.5f;       // m
const float Y_ABAJO = -4.5f;       // m
const float ALTO_VISIBLE = Y_ARRIBA - Y_ABAJO;
const float POSICION_MARIO = 0.4f; // Mario va al 40 % del ancho (ve más hacia adelante)
const float AVISO_DEBILITAR = 1.5f;// s antes del evento en que parpadean los bloques

// Colores (los de Mario y los Minions son los mismos del laberinto).
const QColor CIELO_ARRIBA(92, 148, 252);
const QColor CIELO_ABAJO(170, 205, 255);
const QColor NUBE(255, 255, 255, 220);
const QColor PASTO(70, 170, 40);
const QColor PASTO_OSCURO(40, 110, 20);
const QColor SUELO(150, 95, 45);
const QColor TIERRA_SANA(165, 110, 55);
const QColor TIERRA_DEBIL(215, 185, 140);
const QColor FOSO(35, 25, 20);
const QColor FOSO_PARED(70, 50, 35);
const QColor COLOR_MINION(250, 215, 40);
const QColor CASCO(60, 95, 45);
const QColor COLOR_MARIO(220, 40, 40);
const QColor PIEL(240, 190, 150);
const QColor SOMBRERO(235, 220, 170);
const QColor PESA(70, 70, 80);

float limitar(float v, float minimo, float maximo)
{
    return v < minimo ? minimo : (v > maximo ? maximo : v);
}

// Mezcla dos colores: t = 0 da 'a', t = 1 da 'b'.
QColor mezclar(const QColor& a, const QColor& b, float t)
{
    t = limitar(t, 0.0f, 1.0f);
    return QColor(static_cast<int>(a.red()   + (b.red()   - a.red())   * t),
                  static_cast<int>(a.green() + (b.green() - a.green()) * t),
                  static_cast<int>(a.blue()  + (b.blue()  - a.blue())  * t));
}

} // namespace

DibujoBloques::DibujoBloques()
    : escala_(1.0f), anchoMundo_(1.0f), anchoVisible_(1.0f), camaraX_(0.0f)
{
}

// ------------------------------------------------------------------
// Escala y cámara
// ------------------------------------------------------------------

void DibujoBloques::prepararEscala(const NivelBloques& nivel, const QRect& area)
{
    escala_ = max(1.0f, static_cast<float>(area.height()) / ALTO_VISIBLE);
    anchoVisible_ = static_cast<float>(area.width()) / escala_;

    // El mundo va desde x = 0 hasta el final del último bloque.
    anchoMundo_ = nivel.xMeta() + 2.0f;
    for (const BloqueTierra& b : nivel.bloques())
        anchoMundo_ = max(anchoMundo_, b.xFin());
    area_ = area;
}

// La cámara sigue a Mario, pero no se sale del mundo.
void DibujoBloques::moverCamara(const NivelBloques& nivel)
{
    const float xMario = nivel.protagonista()->posicion().x;
    const float maximo = max(0.0f, anchoMundo_ - anchoVisible_);
    camaraX_ = limitar(xMario - anchoVisible_ * POSICION_MARIO, 0.0f, maximo);
}

float DibujoBloques::aPixelX(float x) const
{
    return static_cast<float>(area_.x()) + (x - camaraX_) * escala_;
}

float DibujoBloques::aPixelY(float y) const
{
    return static_cast<float>(area_.y()) + (Y_ARRIBA - y) * escala_;
}

QPointF DibujoBloques::aPixel(vector2 posicion) const
{
    return QPointF(aPixelX(posicion.x), aPixelY(posicion.y));
}

// Lo que queda fuera de la pantalla no se dibuja.
bool DibujoBloques::esVisible(float xInicio, float xFin) const
{
    return xFin >= camaraX_ - 1.0f && xInicio <= camaraX_ + anchoVisible_ + 1.0f;
}

// ------------------------------------------------------------------
// Fotograma completo
// ------------------------------------------------------------------

void DibujoBloques::dibujar(QPainter& p, const NivelBloques& nivel, const QRect& area, float tiempo)
{
    if (area_ != area) prepararEscala(nivel, area);
    moverCamara(nivel);

    p.save();
    p.setClipRect(area);          // nada se sale del área del juego
    dibujarCielo(p, area);
    p.setRenderHint(QPainter::Antialiasing, true);
    dibujarFosos(p, nivel);
    dibujarMinions(p, nivel);
    dibujarBloques(p, nivel, tiempo);
    dibujarMeta(p, nivel);
    dibujarItems(p, nivel);
    dibujarMario(p, nivel, tiempo);
    p.restore();
}

// Cielo con degradado y nubes que se mueven más lento que la cámara
// (parallax: parece que están lejos).
void DibujoBloques::dibujarCielo(QPainter& p, const QRect& area) const
{
    QLinearGradient cielo(area.topLeft(), area.bottomLeft());
    cielo.setColorAt(0.0, CIELO_ARRIBA);
    cielo.setColorAt(1.0, CIELO_ABAJO);
    p.fillRect(area, cielo);

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(NUBE);
    const float separacion = 9.0f;                     // m entre nubes
    const float lejania = 0.3f;                        // se mueven al 30 %
    const float desplazamiento = camaraX_ * lejania;
    const int primera = static_cast<int>(floor(desplazamiento / separacion)) - 1;
    for (int i = primera; i < primera + static_cast<int>(anchoVisible_ / separacion) + 3; ++i) {
        const float xNube = (i * separacion - desplazamiento) * escala_ + area.x();
        const float yNube = aPixelY(i % 2 == 0 ? 4.4f : 3.6f);
        const float s = escala_ * 0.45f;
        p.drawEllipse(QPointF(xNube - s, yNube + 0.2 * s), s * 0.8, s * 0.6);
        p.drawEllipse(QPointF(xNube, yNube - 0.2 * s), s, s * 0.8);
        p.drawEllipse(QPointF(xNube + s, yNube + 0.2 * s), s * 0.8, s * 0.6);
    }
}

// ------------------------------------------------------------------
// Fosos y el ejército que espera abajo
// ------------------------------------------------------------------

void DibujoBloques::dibujarFosos(QPainter& p, const NivelBloques& nivel) const
{
    for (const Foso& f : nivel.fosos()) {
        if (!esVisible(f.xInicio, f.xFin)) continue;
        const QRectF hueco(QPointF(aPixelX(f.xInicio), aPixelY(0.0f)),
                           QPointF(aPixelX(f.xFin), aPixelY(Y_ABAJO)));
        QLinearGradient oscuro(hueco.topLeft(), hueco.bottomLeft());
        oscuro.setColorAt(0.0, FOSO_PARED);
        oscuro.setColorAt(1.0, FOSO);
        p.setPen(Qt::NoPen);
        p.setBrush(oscuro);
        p.drawRect(hueco);

        // Fondo del foso (donde el ejército captura sí o sí).
        p.setBrush(FOSO.darker(150));
        p.drawRect(QRectF(QPointF(aPixelX(f.xInicio), aPixelY(f.fondo())),
                          QPointF(aPixelX(f.xFin), aPixelY(Y_ABAJO))));
    }
}

/*
 * Cada Minion del ejército: su zona de captura (el semicírculo hacia
 * arriba del cono de Enemigo) y su cuerpo amarillo con casco militar.
 * Ángulos: igual que en DibujoLaberinto, en Qt 0° apunta a la derecha y
 * los positivos giran antihorario, como en la lógica.
 */
void DibujoBloques::dibujarMinions(QPainter& p, const NivelBloques& nivel) const
{
    const float radioCuerpo = 0.35f * escala_;
    for (const MinionEjercitoColombiano* m : nivel.minions()) {
        const vector2 pos = m->posicion();
        if (!esVisible(pos.x - 2.0f, pos.x + 2.0f)) continue;
        const QPointF c = aPixel(pos);

        // Zona de captura.
        const float r = m->radioVision() * escala_;
        QPainterPath zona;
        zona.moveTo(c);
        zona.arcTo(QRectF(c.x() - r, c.y() - r, 2 * r, 2 * r),
                   (m->anguloVision() - m->aperturaCono() * 0.5f) * GRADOS_POR_RADIAN,
                   m->aperturaCono() * GRADOS_POR_RADIAN);
        zona.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 60, 40, 45));
        p.drawPath(zona);

        // Cuerpo, casco y ojos mirando hacia arriba.
        p.setPen(QPen(Qt::black, 1.5));
        p.setBrush(COLOR_MINION);
        p.drawEllipse(c, radioCuerpo, radioCuerpo);
        p.setBrush(CASCO);
        p.drawChord(QRectF(c.x() - radioCuerpo, c.y() - radioCuerpo, 2 * radioCuerpo, 2 * radioCuerpo), 0, 180 * 16);
        p.setPen(Qt::NoPen);
        p.setBrush(Qt::black);
        p.drawEllipse(QPointF(c.x() - 0.35 * radioCuerpo, c.y() + 0.15 * radioCuerpo), 0.12 * radioCuerpo, 0.12 * radioCuerpo);
        p.drawEllipse(QPointF(c.x() + 0.35 * radioCuerpo, c.y() + 0.15 * radioCuerpo), 0.12 * radioCuerpo, 0.12 * radioCuerpo);
    }
}

// ------------------------------------------------------------------
// Bloques
// ------------------------------------------------------------------

void DibujoBloques::dibujarBloques(QPainter& p, const NivelBloques& nivel, float tiempo) const
{
    // Evento por tiempo: el último segundo y medio, parpadeo rojo.
    const bool aviso = !nivel.bloquesRotos() &&
                       nivel.tiempoHastaDebilitar() <= AVISO_DEBILITAR &&
                       static_cast<int>(tiempo * 8.0f) % 2 == 0;
    for (const BloqueTierra& b : nivel.bloques()) {
        if (b.roto() || !esVisible(b.xInicio(), b.xFin())) continue;
        dibujarBloque(p, b, nivel.rigidezInicial(), aviso);
    }
}

void DibujoBloques::dibujarBloque(QPainter& p, const BloqueTierra& b, float rigidezInicial, bool aviso) const
{
    // superficie() ya incluye el hundimiento del resorte.
    const QRectF caja(QPointF(aPixelX(b.xInicio()), aPixelY(b.superficie())),
                      QPointF(aPixelX(b.xFin()), aPixelY(b.yAbajo())));

    if (b.esFirme()) {
        // Suelo firme: tierra con pasto encima. El suelo llega hasta abajo
        // de la pantalla para que no se vea flotando.
        const QRectF hastaAbajo(caja.topLeft(), QPointF(caja.right(), aPixelY(Y_ABAJO)));
        p.setPen(Qt::NoPen);
        p.setBrush(SUELO);
        p.drawRect(hastaAbajo);
        p.setBrush(PASTO);
        p.drawRect(QRectF(caja.left(), caja.top(), caja.width(), 0.18 * escala_));
        p.setPen(QPen(PASTO_OSCURO, 2));
        p.drawLine(QPointF(caja.left(), caja.top() + 0.18 * escala_),
                   QPointF(caja.right(), caja.top() + 0.18 * escala_));
        return;
    }

    // Tierra: más clara mientras más débil (k / k0 baja).
    const float salud = limitar(b.rigidez() / rigidezInicial, 0.0f, 1.0f);
    p.setPen(QPen(aviso ? QColor(230, 30, 30) : TIERRA_SANA.darker(160), aviso ? 3 : 2));
    p.setBrush(mezclar(TIERRA_DEBIL, TIERRA_SANA, salud));
    p.drawRect(caja);

    // Grietas cuando ya perdió más del 40 % de su rigidez.
    if (b.agrietado() || salud < 0.6f) {
        p.setPen(QPen(TIERRA_SANA.darker(200), 1.5));
        const float cx = static_cast<float>(caja.center().x());
        const float w = static_cast<float>(caja.width());
        const float h = static_cast<float>(caja.height());
        const float top = static_cast<float>(caja.top());
        p.drawLine(QPointF(cx - 0.15f * w, top), QPointF(cx, top + 0.5f * h));
        p.drawLine(QPointF(cx, top + 0.5f * h), QPointF(cx + 0.1f * w, top + h));
        if (salud < 0.3f) {
            p.drawLine(QPointF(cx + 0.25f * w, top), QPointF(cx + 0.15f * w, top + 0.6f * h));
            p.drawLine(QPointF(cx - 0.3f * w, top + h), QPointF(cx - 0.2f * w, top + 0.4f * h));
        }
    }
}

// ------------------------------------------------------------------
// Ítems y meta
// ------------------------------------------------------------------

// El ítem del poder pesado es una pesa con "kg" (sube y baja: flotación).
void DibujoBloques::dibujarItems(QPainter& p, const NivelBloques& nivel) const
{
    const float lado = 0.5f * escala_;
    QFont fuente = p.font();
    fuente.setBold(true);
    fuente.setPixelSize(max(8, static_cast<int>(lado * 0.45f)));
    p.setFont(fuente);

    for (const ItemPoder& item : nivel.items()) {
        if (item.recogido()) continue;
        const vector2 pos = item.posicion();
        if (!esVisible(pos.x - 1.0f, pos.x + 1.0f)) continue;
        const QPointF c = aPixel(pos);

        // Brillo alrededor.
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 240, 120, 90));
        p.drawEllipse(c, lado * 0.95, lado * 0.95);

        // Pesa: trapecio con un asa arriba.
        QPainterPath pesa;
        pesa.moveTo(c.x() - 0.30 * lado, c.y() - 0.25 * lado);
        pesa.lineTo(c.x() + 0.30 * lado, c.y() - 0.25 * lado);
        pesa.lineTo(c.x() + 0.45 * lado, c.y() + 0.40 * lado);
        pesa.lineTo(c.x() - 0.45 * lado, c.y() + 0.40 * lado);
        pesa.closeSubpath();
        p.setPen(QPen(Qt::black, 1.5));
        p.setBrush(PESA);
        p.drawPath(pesa);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(QPointF(c.x(), c.y() - 0.40 * lado), 0.15 * lado, 0.15 * lado);
        p.setPen(Qt::white);
        p.drawText(QRectF(c.x() - 0.45 * lado, c.y() - 0.2 * lado, 0.9 * lado, 0.6 * lado), Qt::AlignCenter, "kg");
    }
}

void DibujoBloques::dibujarMeta(QPainter& p, const NivelBloques& nivel) const
{
    const float x = nivel.xMeta();
    if (!esVisible(x - 1.0f, x + 1.0f)) return;
    const QPointF base = aPixel(vector2(x, 0.0f));
    const QPointF punta = aPixel(vector2(x, 3.0f));

    p.setPen(QPen(QColor(90, 90, 90), 0.08 * escala_));
    p.drawLine(base, punta);

    QPainterPath bandera;
    bandera.moveTo(punta);
    bandera.lineTo(punta.x() + 0.9 * escala_, punta.y() + 0.3 * escala_);
    bandera.lineTo(punta.x(), punta.y() + 0.6 * escala_);
    bandera.closeSubpath();
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(QColor(255, 255, 255));
    p.drawPath(bandera);

    QFont fuente = p.font();
    fuente.setBold(true);
    fuente.setPixelSize(max(10, static_cast<int>(0.35f * escala_)));
    p.setFont(fuente);
    p.setPen(Qt::white);
    p.drawText(QRectF(base.x() - escala_, base.y() - 3.8 * escala_, 2 * escala_, 0.6 * escala_),
               Qt::AlignCenter, "META");
}

// ------------------------------------------------------------------
// Mario
// ------------------------------------------------------------------

/*
 * Mario con figuras: cuerpo rojo (más oscuro y ancho si está pesado),
 * cabeza, sombrero y un ojo hacia donde mira. Su posición es la de los
 * PIES; mide NivelBloques::ALTO_MARIO de alto. Pesado: una barra encima
 * con el tiempo que le queda (parpadea los últimos 2 s). Capturado: una X.
 */
void DibujoBloques::dibujarMario(QPainter& p, const NivelBloques& nivel, float tiempo) const
{
    const Protagonista& m = *nivel.protagonista();
    const QPointF pies = aPixel(m.posicion());
    const float alto = NivelBloques::ALTO_MARIO * escala_;
    const float medio = NivelBloques::MEDIO_ANCHO_MARIO * escala_ * (m.esPesado() ? 1.15f : 1.0f);
    const float r = 0.2f * escala_;                    // radio de la cabeza
    const float altoCuerpo = alto - 2.0f * r;

    // Cuerpo
    p.setPen(QPen(Qt::black, m.esPesado() ? 2.5 : 1.5));
    p.setBrush(m.esPesado() ? COLOR_MARIO.darker(160) : COLOR_MARIO);
    p.drawRoundedRect(QRectF(pies.x() - medio, pies.y() - altoCuerpo, 2 * medio, altoCuerpo), 4, 4);

    // Cabeza
    const QPointF cabeza(pies.x(), pies.y() - altoCuerpo - r);
    p.setBrush(PIEL);
    p.drawEllipse(cabeza, r, r);

    // Ojo hacia donde mira (en el nivel 1 solo derecha o izquierda).
    const float lado = (m.mirada() == Mirada::IZQUIERDA) ? -1.0f : 1.0f;
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.drawEllipse(QPointF(cabeza.x() + lado * 0.45 * r, cabeza.y()), 0.18 * r, 0.18 * r);

    // Sombrero: ala ancha y copa con dos franjas.
    p.setPen(QPen(Qt::black, 1));
    p.setBrush(SOMBRERO);
    p.drawEllipse(QPointF(cabeza.x(), cabeza.y() - 0.75 * r), 1.6 * r, 0.35 * r);
    const QRectF copa(cabeza.x() - 0.7 * r, cabeza.y() - 1.6 * r, 1.4 * r, 0.85 * r);
    p.drawRect(copa);
    p.setPen(QPen(Qt::black, 2));
    p.drawLine(QPointF(copa.left(), copa.top() + 0.3 * copa.height()), QPointF(copa.right(), copa.top() + 0.3 * copa.height()));
    p.drawLine(QPointF(copa.left(), copa.top() + 0.65 * copa.height()), QPointF(copa.right(), copa.top() + 0.65 * copa.height()));

    // Barra del poder pesado.
    if (m.esPesado() && m.duracionPesado() > 0.0f) {
        const float fraccion = limitar(m.tiempoPesadoRestante() / m.duracionPesado(), 0.0f, 1.0f);
        const bool parpadea = m.tiempoPesadoRestante() < 2.0f && static_cast<int>(tiempo * 8.0f) % 2 == 0;
        const QRectF marco(pies.x() - 0.5 * escala_, cabeza.y() - 2.3 * r, escala_, 0.12 * escala_);
        p.setPen(QPen(Qt::black, 1));
        p.setBrush(QColor(0, 0, 0, 120));
        p.drawRect(marco);
        p.setPen(Qt::NoPen);
        p.setBrush(parpadea ? QColor(255, 255, 255) : QColor(150, 90, 255));
        p.drawRect(QRectF(marco.left(), marco.top(), marco.width() * fraccion, marco.height()));
    }

    if (m.estaCapturado()) {
        const QPointF centro(pies.x(), pies.y() - alto * 0.5f);
        const float s = alto * 0.5f;
        p.setPen(QPen(Qt::white, 3));
        p.drawLine(centro + QPointF(-s, -s), centro + QPointF(s, s));
        p.drawLine(centro + QPointF(-s, s), centro + QPointF(s, -s));
    }
}
