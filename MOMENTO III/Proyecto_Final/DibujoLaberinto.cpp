#include "DibujoLaberinto.h"
#include "NivelLaberinto.h"
#include "MapaLaberinto.h"
#include "MinionGuardia.h"
#include "BloqueMovil.h"
#include "protagonista.h"
#include "Excepciones_del_Juego.h"

#include <QPainter>
#include <QPainterPath>
#include <QColor>
#include <QFont>
#include <algorithm>
#include <cmath>
#include <string>

using namespace std;

namespace {

const int RAYOS_CONO = 24;          // rayos por cono (más = borde más suave)


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

// Lee un sprite del .qrc; si falta, lanza (lo atrapa main y muestra el error).
QPixmap leerSprite(const string& nombre)
{
    const string ruta = ":/sprites/" + nombre + ".png";
    QPixmap imagen(QString::fromStdString(ruta));
    if (imagen.isNull())
        throw Excepciones_de_Recursos(ruta, "no está en Recursos.qrc");
    return imagen;
}

// Elige la variante de baldosa de cada celda: siempre la misma para la misma
// celda (no parpadea) pero sin un patrón visible. La sucia (petróleo, escombros)
// sale menos que la normal.
int varianteCelda(int fila, int columna, const int* tabla, int tam)
{
    const unsigned int h = (static_cast<unsigned int>(fila) * 73856093u) ^
                           (static_cast<unsigned int>(columna) * 19349663u);
    return tabla[h % static_cast<unsigned int>(tam)];
}
const int TABLA_PISO[8]  = {0, 0, 0, 0, 0, 1, 2, 3};
const int TABLA_PARED[7] = {0, 0, 0, 0, 1, 1, 2};

const char* const NOMBRE_DIRECCION[4] = {"derecha", "izquierda", "arriba", "abajo"};

// Índice de dirección a partir de un ángulo (radianes, y hacia arriba).
int direccionDeAngulo(float angulo)
{
    const float c = cos(angulo), s = sin(angulo);
    if (fabs(c) >= fabs(s)) return c >= 0.0f ? 0 : 1;
    return s >= 0.0f ? 2 : 3;
}

int direccionDeMirada(Mirada mirada)
{
    switch (mirada) {
    case Mirada::DERECHA:   return 0;
    case Mirada::IZQUIERDA: return 1;
    case Mirada::ARRIBA:    return 2;
    case Mirada::ABAJO:     return 3;
    }
    return 3;
}

// Cuadro de la animación: quieto = 0; caminando = 1, 0, 2, 0... (8 por segundo).
int cuadroAnimacion(bool seMueve, float tiempo)
{
    if (!seMueve) return 0;
    static const int CICLO[4] = {1, 0, 2, 0};
    return CICLO[static_cast<int>(tiempo * 8.0f) & 3];
}

} // namespace

DibujoLaberinto::DibujoLaberinto()
    : mapaFondo_(nullptr), escala_(1.0f), origen_(0.0, 0.0), altoMundo_(1.0f), ladoSprite_(0)
{
    cargarSprites();
}

void DibujoLaberinto::cargarSprites()
{
    for (int d = 0; d < DIRECCIONES; ++d)
        for (int k = 0; k < CUADROS; ++k) {
            const string sufijo = string("_") + NOMBRE_DIRECCION[d] + "_" + to_string(k);
            protagonistaOriginal_[d][k] = leerSprite("protagonista" + sufijo);
            enemigoOriginal_[d][k] = leerSprite("enemigo" + sufijo);
        }
    dormidoOriginal_ = leerSprite("protagonista_dormido");
    capturadoOriginal_ = leerSprite("protagonista_capturado");
    cajaOriginal_ = leerSprite("caja");
    for (int v = 0; v < VARIANTES_PARED; ++v) paredOriginal_[v] = leerSprite("pared_" + to_string(v));
    for (int v = 0; v < VARIANTES_PISO; ++v)  pisoOriginal_[v] = leerSprite("piso_" + to_string(v));
    salidaOriginal_ = leerSprite("salida");
}

// Los personajes miden un poco más que su círculo de choque (se ven mejor).
void DibujoLaberinto::prepararSprites()
{
    const int lado = max(8, static_cast<int>(3.6f * NivelLaberinto::RADIO_MARIO * escala_));
    if (lado == ladoSprite_) return;
    ladoSprite_ = lado;
    auto escalar = [lado](const QPixmap& original) {
        return original.scaled(lado, lado, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    };
    for (int d = 0; d < DIRECCIONES; ++d)
        for (int k = 0; k < CUADROS; ++k) {
            protagonista_[d][k] = escalar(protagonistaOriginal_[d][k]);
            enemigo_[d][k] = escalar(enemigoOriginal_[d][k]);
        }
    dormido_ = escalar(dormidoOriginal_);
    capturado_ = escalar(capturadoOriginal_);
    const int ladoCaja = max(4, static_cast<int>(2.0f * NivelLaberinto::RADIO_BLOQUE * escala_));
    caja_ = cajaOriginal_.scaled(ladoCaja, ladoCaja, Qt::IgnoreAspectRatio, Qt::FastTransformation);
}

// Sprite con los pies en el borde de abajo del círculo del personaje.
void DibujoLaberinto::dibujarSprite(QPainter& p, const QPixmap& sprite, QPointF centro, float radio) const
{
    p.drawPixmap(QPointF(centro.x() - sprite.width() * 0.5, centro.y() + radio - sprite.height()), sprite);
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
    fondo_.fill(Qt::black);

    // Baldosas escaladas una vez al tamaño de una celda (+1 para que no queden rayas).
    const int ladoBaldosa = static_cast<int>(ceil(lado)) + 1;
    QPixmap pared[VARIANTES_PARED], piso[VARIANTES_PISO];
    for (int v = 0; v < VARIANTES_PARED; ++v)
        pared[v] = paredOriginal_[v].scaled(ladoBaldosa, ladoBaldosa, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    for (int v = 0; v < VARIANTES_PISO; ++v)
        piso[v] = pisoOriginal_[v].scaled(ladoBaldosa, ladoBaldosa, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    const QPixmap salida = salidaOriginal_.scaled(ladoBaldosa, ladoBaldosa, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    QPainter p(&fondo_);
    for (int f = 0; f < mapa.alto(); ++f)
        for (int c = 0; c < mapa.ancho(); ++c)
            p.drawPixmap(QPointF(c * lado, f * lado), mapa.esPared(c, f)
                                                          ? pared[varianteCelda(f, c, TABLA_PARED, 7)]
                                                          : piso[varianteCelda(f, c, TABLA_PISO, 8)]);

    const MapaLaberinto::Celda s = mapa.celdaSalida();
    p.drawPixmap(QPointF(s.columna * lado, s.fila * lado), salida);
}


// Dibujo completo de un fotograma


void DibujoLaberinto::dibujar(QPainter& p, const NivelLaberinto& nivel, const QRect& area, float tiempo)
{
    const MapaLaberinto& mapa = nivel.mapa();

    // Solo se recalcula la escala y el fondo si cambió el área o el mapa.
    if (mapaFondo_ != &mapa || areaFondo_ != area) {
        prepararEscala(mapa, area);
        prepararFondo(mapa);
        prepararSprites();
        mapaFondo_ = &mapa;
        areaFondo_ = area;
    }

    p.drawPixmap(origen_, fondo_);
    p.setRenderHint(QPainter::Antialiasing, true);
    dibujarBloques(p, nivel);
    dibujarGuardias(p, nivel, tiempo);
    dibujarMario(p, nivel, tiempo);
    dibujarApagon(p, nivel, area, tiempo);
    p.setRenderHint(QPainter::Antialiasing, false);
}

void DibujoLaberinto::dibujarBloques(QPainter& p, const NivelLaberinto& nivel) const
{
    for (const BloqueMovil* b : nivel.bloques()) {
        const QPointF c = aPixel(b->posicion());
        p.drawPixmap(QPointF(c.x() - caja_.width() * 0.5, c.y() - caja_.height() * 0.5), caja_);
    }
}

//Cada Minion: su cono de visión (el color dice qué está haciendo el agente), su cuerpo y, si está alerta, un "!" que se llena.
//Ángulos: en Qt 0° apunta a la derecha y los ángulos positivos giran en sentido antihorario en la pantalla, igual que en la lógica (y arriba), así que el ángulo del Minion se usa tal cual, pasado a grados.

void DibujoLaberinto::dibujarGuardias(QPainter& p, const NivelLaberinto& nivel, float tiempo) const
{
    const float radioCuerpo = NivelLaberinto::RADIO_MINION * escala_;
    QFont fuente = p.font();
    fuente.setBold(true);
    fuente.setPixelSize(max(10, static_cast<int>(radioCuerpo * 1.6f)));

    for (const MinionGuardia* g : nivel.guardias()) {
        const QPointF c = aPixel(g->posicion());

        // Cono de visión (linterna): se lanzan RAYOS_CONO rayos dentro del cono y
        // cada uno se corta en la primera pared, igual que la visión de la lógica
        // (hayLineaDeVision): la luz no atraviesa paredes ni sale por detrás.
        const MapaLaberinto& mapa = nivel.mapa();
        const vector2 ojo = g->posicion();
        const float paso = 0.1f * mapa.tamCelda();
        const float inicio = g->anguloVision() - g->aperturaCono() * 0.5f;
        const float delta = g->aperturaCono() / static_cast<float>(RAYOS_CONO);
        QPainterPath cono;
        cono.moveTo(c);
        for (int k = 0; k <= RAYOS_CONO; ++k) {
            const float a = inicio + delta * static_cast<float>(k);
            const vector2 dir(cos(a), sin(a));
            float d = paso;
            while (d < g->radioVision() && !mapa.esParedEn(ojo + dir * d))
                d += paso;
            if (d > g->radioVision()) d = g->radioVision();
            cono.lineTo(aPixel(ojo + dir * d));
        }
        cono.closeSubpath();
        p.setPen(Qt::NoPen);
        p.setBrush(colorCono(g->estado()));
        p.drawPath(cono);

        // Cuerpo: sprite según hacia dónde mira y si camina.
        const int dir = direccionDeAngulo(g->anguloVision());
        const int cuadro = cuadroAnimacion(g->velocidad().normaCuadrada() > 0.01f, tiempo);
        dibujarSprite(p, enemigo_[dir][cuadro], c, radioCuerpo);

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

    // Sprite: capturado > dormido > según la mirada (caminando se anima).
    if (m.estaCapturado()) {
        dibujarSprite(p, capturado_, c, r);
    } else if (m.estaDormido()) {
        dibujarSprite(p, dormido_, c, r);
    } else {
        const int dir = direccionDeMirada(m.mirada());
        const int cuadro = cuadroAnimacion(m.velocidad().normaCuadrada() > 0.04f, tiempo);
        dibujarSprite(p, protagonista_[dir][cuadro], c, r);
    }

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