#include "NivelLaberinto.h"
#include "protagonista.h"
#include "MinionGuardia.h"
#include "BloqueMovil.h"
#include <algorithm>
#include <cmath>

using namespace std;

namespace {
float limitar(float v, float minimo, float maximo)
{
    return v < minimo ? minimo : (v > maximo ? maximo : v);
}
}

// Parámetros del apagón según la dificultad


ParametrosApagon ParametrosApagon::segunDificultad(NivelDificultad dificultad)
{
    switch (dificultad) {
    //                            cada  dura  Minions  jugador  aviso
    case NivelDificultad::FACIL:   return {30.0f, 4.0f, 0.5f, 4.0f, 3.0f};
    case NivelDificultad::NORMAL:  return {20.0f, 6.0f, 0.6f, 3.0f, 2.0f};
    case NivelDificultad::DIFICIL: return {12.0f, 9.0f, 0.8f, 2.0f, 1.0f};
    }
    return {20.0f, 6.0f, 0.6f, 3.0f, 2.0f};
}


// Construcción

NivelLaberinto::NivelLaberinto(const string& contenidoMapa, const string& nombreMapa,
                               NivelDificultad dificultad)
    : Nivel(),
    mapa_(contenidoMapa, nombreMapa, TAM_CELDA),
    dificultad_(dificultad),
    apagon_(ParametrosApagon::segunDificultad(dificultad)),
    lucesApagadas_(false),
    tiempoParaCambioLuz_(apagon_.intervalo)
{
    const vector<MapaLaberinto::Celda>& celdasGuardias = mapa_.guardias();
    const vector<MapaLaberinto::Celda>& celdasBloques = mapa_.bloquesMoviles();

    reservarPersonajes(1 + celdasGuardias.size() + celdasBloques.size());
    guardias_.reserve(celdasGuardias.size());
    bloques_.reserve(celdasBloques.size());

    // Mario: en el laberinto se duerme si se queda quieto y mira en 4 direcciones.
    Protagonista* mario = new Protagonista(mapa_.inicioMario(), dificultad);
    agregarPersonaje(mario);
    protagonista_ = mario;
    mario->habilitarSueno(true);
    mario->usarMiradaCuatroDirecciones(true);

    // Minions guardia: cada uno con su propia semilla (patrullan distinto).
    const ParametrosGuardia parametros = ParametrosGuardia::segunDificultad(dificultad);
    unsigned semilla = 1;
    for (const MapaLaberinto::Celda& celda : celdasGuardias) {
        MinionGuardia* g = new MinionGuardia(mapa_, celda, parametros, semilla++, MASA_MINION);
        agregarPersonaje(g);
        guardias_.push_back(g);
    }

    for (const MapaLaberinto::Celda& celda : celdasBloques) {
        BloqueMovil* b = crearBloque(celda);
        agregarPersonaje(b);
        bloques_.push_back(b);
    }
}

// Cuántas celdas de pasillo seguidas hay desde 'desde' en la dirección  (dc, df), sin contar la propia, hasta MAX_CELDAS_RIEL.
int NivelLaberinto::celdasLibresSeguidas(MapaLaberinto::Celda desde, int dc, int df) const
{
    int n = 0;
    while (n < MAX_CELDAS_RIEL &&
           !mapa_.esPared(desde.columna + dc * (n + 1), desde.fila + df * (n + 1)))
        ++n;
    return n;
}


BloqueMovil* NivelLaberinto::crearBloque(MapaLaberinto::Celda celda) const
{
    const int horizontal = min(celdasLibresSeguidas(celda, -1, 0), celdasLibresSeguidas(celda, 1, 0));
    const int vertical = min(celdasLibresSeguidas(celda, 0, -1), celdasLibresSeguidas(celda, 0, 1));
    const bool esHorizontal = horizontal >= vertical;
    const vector2 eje = esHorizontal ? vector2(1.0f, 0.0f) : vector2(0.0f, 1.0f);
    const float amplitud = static_cast<float>(esHorizontal ? horizontal : vertical) * TAM_CELDA;
    return new BloqueMovil(mapa_.centroDe(celda), eje, amplitud, MASA_BLOQUE);
}


// Cada fotograma

void NivelLaberinto::actualizar(float dt)
{
    if (!(dt > 0.0f)) return;

    avanzarTiempo(dt);
    actualizarLuces(dt);

    Protagonista& mario = *protagonista_;
    const vector2 posicionMario = mario.posicion();
    const bool marioDormido = mario.estaDormido();

    // 2. Agente inteligente
    for (MinionGuardia* g : guardias_)
        g->pensar(dt, posicionMario, marioDormido);

    // 3. Fuerzas
    const float velMaxMario = (mario.estaCorriendo() ? VEL_CORRER : VEL_CAMINAR) * mario.factorVelocidad();
    aplicarControlDeVelocidad(mario, mario.direccionEfectiva() * velMaxMario);
    for (MinionGuardia* g : guardias_)
        aplicarControlDeVelocidad(*g, g->velocidadDeseada());
    for (BloqueMovil* b : bloques_)
        aplicarResorte(*b);

    // 4. Integrar
    actualizarPersonajes(dt);

    // 5. Choques
    for (BloqueMovil* b : bloques_) {
        resolverChoqueConBloque(mario, RADIO_MARIO, *b);
        for (MinionGuardia* g : guardias_)
            resolverChoqueConBloque(*g, RADIO_MINION, *b);
    }
    resolverChoqueConParedes(mario, RADIO_MARIO);
    for (MinionGuardia* g : guardias_)
        resolverChoqueConParedes(*g, RADIO_MINION);

    // 6. Reglas
    revisarCapturas();
}

bool NivelLaberinto::condicionVictoria() const
{
    return !protagonista_->estaCapturado() &&
           mapa_.celdaDe(protagonista_->posicion()) == mapa_.celdaSalida();
}

bool NivelLaberinto::condicionDerrota() const
{
    return protagonista_->estaCapturado();
}


// FÍSICA 1: fuerza de control con rozamiento viscoso
// F = b · (v_deseada − v)
void NivelLaberinto::aplicarControlDeVelocidad(Personaje& p, vector2 velocidadDeseada) const
{
    p.aplicarFuerza((velocidadDeseada - p.velocidad()) * COEF_CONTROL);
}


// FÍSICA 2: resorte (oscilador armónico) de los bloques
// F = −k · x, sobre el eje del riel (x = desplazamiento desde el centro)
void NivelLaberinto::aplicarResorte(BloqueMovil& b) const
{
    b.aplicarFuerza(b.eje() * (-CONSTANTE_RESORTE * b.desplazamiento()));
}


// FÍSICA 3: choques

void NivelLaberinto::resolverChoqueConParedes(Personaje& p, float radio) const
{
    vector2 pos = p.posicion();
    vector2 vel = p.velocidad();
    const float tam = mapa_.tamCelda();
    const int alto = mapa_.alto();
    const MapaLaberinto::Celda centro = mapa_.celdaDe(pos);
    bool huboChoque = false;

    for (int df = -1; df <= 1; ++df) {
        for (int dc = -1; dc <= 1; ++dc) {
            const int c = centro.columna + dc;
            const int f = centro.fila + df;
            if (!mapa_.esPared(c, f)) continue;

            // Caja de la pared en metros (y hacia arriba).
            const float xMin = static_cast<float>(c) * tam;
            const float yMin = static_cast<float>(alto - 1 - f) * tam;
            const vector2 cercano(limitar(pos.x, xMin, xMin + tam),
                                  limitar(pos.y, yMin, yMin + tam));
            const vector2 d = pos - cercano;
            const float d2 = d.normaCuadrada();
            if (d2 >= radio * radio) continue;

            vector2 normal;
            float penetracion;
            if (d2 > 1e-12f) {
                const float distancia = sqrt(d2);
                normal = d / distancia;
                penetracion = radio - distancia;
            } else {
                // El centro quedó dentro de la pared (muy raro): se saca
                // por el lado más cercano.
                const vector2 e = pos - mapa_.centroDe(c, f);
                if (fabs(e.x) > fabs(e.y)) normal = vector2(e.x >= 0.0f ? 1.0f : -1.0f, 0.0f);
                else                       normal = vector2(0.0f, e.y >= 0.0f ? 1.0f : -1.0f);
                penetracion = radio + 0.5f * tam - fabs(e.punto(normal));
            }
            pos += normal * penetracion;
            const float vn = vel.punto(normal);
            if (vn < 0.0f) vel -= normal * vn;
            huboChoque = true;
        }
    }
    if (huboChoque) {
        p.setPosicion(pos);
        p.setVelocidad(vel);
    }
}


void NivelLaberinto::resolverChoqueConBloque(Personaje& p, float radio, BloqueMovil& b) const
{
    const vector2 d = p.posicion() - b.posicion();
    const float sumaRadios = radio + RADIO_BLOQUE;
    const float d2 = d.normaCuadrada();
    if (d2 >= sumaRadios * sumaRadios) return;

    float distancia = sqrt(d2);
    vector2 normal = distancia > 1e-6f ? d / distancia : vector2(0.0f, 1.0f);
    if (distancia <= 1e-6f) distancia = 0.0f;
    p.setPosicion(p.posicion() + normal * (sumaRadios - distancia));

    const float vRel = (p.velocidad() - b.velocidad()).punto(normal);
    if (vRel < 0.0f) {
        const float invP = 1.0f / p.masa();
        const float invB = 1.0f / b.masa();
        const float impulso = -(1.0f + RESTITUCION) * vRel / (invP + invB);
        p.setVelocidad(p.velocidad() + normal * (impulso * invP));
        b.setVelocidad(b.velocidad() - normal * (impulso * invB));
        b.ajustarAlRiel();
    }
}


// Evento por tiempo: apagón

void NivelLaberinto::actualizarLuces(float dt)
{
    tiempoParaCambioLuz_ -= dt;
    if (tiempoParaCambioLuz_ > 0.0f) return;

    lucesApagadas_ = !lucesApagadas_;
    tiempoParaCambioLuz_ += lucesApagadas_ ? apagon_.duracion : apagon_.intervalo;
    const float factor = lucesApagadas_ ? apagon_.factorMinion : 1.0f;
    for (MinionGuardia* g : guardias_)
        g->setFactorLuz(factor);   // recalcula el radio solo en este momento
}


// Capturas

void NivelLaberinto::revisarCapturas()
{
    Protagonista& mario = *protagonista_;
    if (mario.estaCapturado()) return;
    const vector2 pm = mario.posicion();
    for (const MinionGuardia* g : guardias_) {
        if ((g->posicion() - pm).normaCuadrada() <= RADIO_CAPTURA * RADIO_CAPTURA) {
            mario.capturar();
            return;
        }
    }
}
