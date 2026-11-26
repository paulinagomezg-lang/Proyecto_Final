#include "MinionGuardia.h"
#include "Excepciones_del_Juego.h"
#include <cstdlib>

using namespace std;

namespace {

const float GRADOS = 3.14159265f / 180.0f;
const float VELOCIDAD_GIRO = 3.14159265f;   // rad/s al mirar alrededor (media vuelta por segundo)
const float FRACCION_LLEGADA = 0.1f;        // "llegó" si está a menos de 0.1 celdas del punto

// Aprendizaje (por cada escape, multiplicado por tasaAprendizaje)
const float REDUCCION_REACCION = 0.15f;     // reacciona 15 % más rápido
const float AUMENTO_VIGILAR = 0.2f;         // +20 % de ir a vigilar zonas frecuentes
const float DESGASTE_ZONA = 0.6f;           // al vigilar una zona, su peso baja (no se obsesiona)

int distanciaManhattan(MapaLaberinto::Celda a, MapaLaberinto::Celda b)
{
    return abs(a.columna - b.columna) + abs(a.fila - b.fila);
}

}

// Parámetros según la dificultad

ParametrosGuardia ParametrosGuardia::segunDificultad(NivelDificultad dificultad)
{
    switch (dificultad) {
    case NivelDificultad::FACIL:
        return {4.0f, 70.0f * GRADOS, 1.2f, 2.6f, 1.2f, 3.0f, 2.5f, 4.0f, 0.5f};
    case NivelDificultad::NORMAL:
        return {5.0f, 90.0f * GRADOS, 1.5f, 3.0f, 0.8f, 5.0f, 3.5f, 6.0f, 1.0f};
    case NivelDificultad::DIFICIL:
        return {6.5f, 110.0f * GRADOS, 1.8f, 3.4f, 0.4f, 8.0f, 5.0f, 8.0f, 1.5f};
    }
    return {5.0f, 90.0f * GRADOS, 1.5f, 3.0f, 0.8f, 5.0f, 3.5f, 6.0f, 1.0f};
}

// Construcción

MinionGuardia::MinionGuardia(const MapaLaberinto& mapa, MapaLaberinto::Celda puesto, const ParametrosGuardia& parametros, unsigned semilla, float masa)
    : Enemigo(mapa.centroDe(puesto), masa, parametros.radioVision, 0.0f, parametros.aperturaCono),
    mapa_(&mapa),
    parametros_(parametros),
    puesto_(puesto),
    estado_(EstadoAgente::PATRULLAR),
    alerta_(0.0f),
    veAMario_(false),
    ultimaVista_(posicion_),
    destinoPatrulla_(posicion_),
    tiempoEnEstado_(0.0f),
    factorLuz_(1.0f),
    llegoAlPunto_(false),
    pasoActual_(puesto),
    destinoRuta_(puesto),
    hayPaso_(false),
    velocidadDeseada_(0.0f, 0.0f),
    semilla_(semilla * 2654435761u + 12345u),  // mezcla la semilla (nunca queda en 0)
    escapes_(0),
    factorReaccion_(1.0f),
    pesoVigilar_(0.0f)
{
    zonas_.reserve(MAX_ZONAS);   // única reserva: en el juego no vuelve a pedir memoria
    // Enemigo ya validó radioVision y aperturaCono.
    Excepciones_de_Parametros_Fisicos::exigirPositivo("velPatrulla", parametros.velPatrulla);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("velPersecucion", parametros.velPersecucion);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("tiempoAlerta", parametros.tiempoAlerta);
    Excepciones_de_Parametros_Fisicos::exigirNoNegativo("tiempoBusqueda", parametros.tiempoBusqueda);
    Excepciones_de_Parametros_Fisicos::exigirNoNegativo("radioOido", parametros.radioOido);
    Excepciones_de_Parametros_Fisicos::exigirNoNegativo("radioPatrulla", parametros.radioPatrulla);
    Excepciones_de_Parametros_Fisicos::exigirNoNegativo("tasaAprendizaje", parametros.tasaAprendizaje);
    if (mapa.esPared(puesto))
        throw Excepciones_del_Juego("el puesto de un Minion guardia esta dentro de una pared");

    elegirPuntoDePatrulla();
}

void MinionGuardia::setFactorLuz(float factor)
{
    if (factor < 0.1f) factor = 0.1f;   // nunca queda ciego del todo (y el radio sigue > 0)
    if (factor > 1.0f) factor = 1.0f;
    if (factor == factorLuz_) return;   // solo recalcula cuando cambia
    factorLuz_ = factor;
    setRadioVision(parametros_.radioVision * factor);
}

void MinionGuardia::cambiarEstado(EstadoAgente nuevo)
{
    estado_ = nuevo;
    tiempoEnEstado_ = 0.0f;
    llegoAlPunto_ = false;
    hayPaso_ = false;
}

// 1. PERCEPCIÓN

bool MinionGuardia::percibe(vector2 posicionMario, bool marioDormido) const
{
    if (marioDormido) {
        const float r = parametros_.radioOido;
        if ((posicionMario - posicion_).normaCuadrada() <= r * r &&
            mapa_->hayLineaDeVision(posicion_, posicionMario))
            return true;
    }
    return detecta(posicionMario) && mapa_->hayLineaDeVision(posicion_, posicionMario);
}

// 2. DECISIÓN (máquina de estados) y 3. ACCIÓN

void MinionGuardia::pensar(float dt, vector2 posicionMario, bool marioDormido)
{
    velocidadDeseada_ = vector2(0.0f, 0.0f);
    if (!(dt > 0.0f)) return;

    // Percepción y memoria
    veAMario_ = percibe(posicionMario, marioDormido);
    if (veAMario_) ultimaVista_ = posicionMario;
    tiempoEnEstado_ += dt;

    switch (estado_) {
    case EstadoAgente::PATRULLAR:
        if (veAMario_) {
            cambiarEstado(EstadoAgente::SOSPECHAR);
            // Dormido no hay duda: es Mario. Lo persigue de una vez.
            if (marioDormido) alerta_ = 1.0f;
            mirarHacia(ultimaVista_);
            break;
        }
        if (irHacia(destinoPatrulla_, parametros_.velPatrulla))
            elegirPuntoDePatrulla();
        break;

    case EstadoAgente::SOSPECHAR:
        // Se queda quieto mirando hacia donde vio algo.
        mirarHacia(ultimaVista_);
        if (veAMario_) {
            // La reacción aprendida acorta el tiempo para decidir perseguirlo.
            alerta_ += marioDormido ? 1.0f : dt / (parametros_.tiempoAlerta * factorReaccion_);
            if (alerta_ >= 1.0f) {
                alerta_ = 1.0f;
                registrarZona(mapa_->celdaDe(ultimaVista_));   // empieza un encuentro: ¿dónde?
                cambiarEstado(EstadoAgente::PERSEGUIR);
                irHacia(ultimaVista_, parametros_.velPersecucion);   // arranca en este mismo fotograma
            }
        } else {
            // Si deja de verlo, la alerta baja a la mitad de la velocidad.
            alerta_ -= dt / (2.0f * parametros_.tiempoAlerta * factorReaccion_);
            if (alerta_ <= 0.0f) {
                alerta_ = 0.0f;
                cambiarEstado(EstadoAgente::PATRULLAR);
            }
        }
        break;

    case EstadoAgente::PERSEGUIR:
        if (veAMario_) {
            irHacia(ultimaVista_, parametros_.velPersecucion);
        } else {
            // Lo perdió: va a donde lo vio por última vez.
            cambiarEstado(EstadoAgente::BUSCAR);
            irHacia(ultimaVista_, parametros_.velPersecucion);
        }
        break;

    case EstadoAgente::BUSCAR:
        if (veAMario_) {
            alerta_ = 1.0f;
            cambiarEstado(EstadoAgente::PERSEGUIR);
            irHacia(ultimaVista_, parametros_.velPersecucion);
            break;
        }
        if (!llegoAlPunto_) {
            if (irHacia(ultimaVista_, parametros_.velPersecucion)) {
                llegoAlPunto_ = true;
                tiempoEnEstado_ = 0.0f;   // ahora cuenta el tiempo mirando alrededor
            }
        } else {
            // Mira alrededor girando el cono de visión.
            setAnguloVision(anguloVision_ + VELOCIDAD_GIRO * dt);
            if (tiempoEnEstado_ >= parametros_.tiempoBusqueda) {
                // Fin del encuentro: Mario se escapó. 4. APRENDE de esto.
                aprenderDeEscape();
                alerta_ = 0.0f;
                cambiarEstado(EstadoAgente::PATRULLAR);
                elegirPuntoDePatrulla();
            }
        }
        break;
    }
}


bool MinionGuardia::irHacia(vector2 destino, float velocidad)
{
    const MapaLaberinto::Celda aqui = mapa_->celdaDe(posicion_);
    const MapaLaberinto::Celda meta = mapa_->celdaDe(destino);
    const float tam = mapa_->tamCelda();
    const float llegada2 = (FRACCION_LLEGADA * tam) * (FRACCION_LLEGADA * tam);

    vector2 objetivo;
    if (aqui == meta) {
        const vector2 d = destino - posicion_;
        if (d.normaCuadrada() <= llegada2) {
            velocidadDeseada_ = vector2(0.0f, 0.0f);
            return true;
        }
        objetivo = destino;
    } else {
        const bool cambioMeta = !hayPaso_ || meta != destinoRuta_;
        const bool lejosDeRuta = hayPaso_ && distanciaManhattan(aqui, pasoActual_) > 1;
        const bool llegoAlPaso = hayPaso_ &&
                                 (mapa_->centroDe(pasoActual_) - posicion_).normaCuadrada() <= llegada2;
        if (cambioMeta || lejosDeRuta || llegoAlPaso) {
            MapaLaberinto::Celda paso;
            if (!mapa_->siguientePaso(aqui, meta, paso)) {
                hayPaso_ = false;
                velocidadDeseada_ = vector2(0.0f, 0.0f);
                return true;   // inalcanzable: se rinde (elige otro destino)
            }
            pasoActual_ = paso;
            destinoRuta_ = meta;
            hayPaso_ = true;
        }
        objetivo = mapa_->centroDe(pasoActual_);
    }

    velocidadDeseada_ = (objetivo - posicion_).normalizado() * velocidad;
    // Mira hacia donde camina.
    if (velocidadDeseada_.normaCuadrada() > 0.0f)
        setAnguloVision(velocidadDeseada_.angulo());
    return false;
}

void MinionGuardia::mirarHacia(vector2 punto)
{
    const vector2 d = punto - posicion_;
    if (d.normaCuadrada() > 0.0f)
        setAnguloVision(d.angulo());
}

// Patrullar o vigilar "según los pesos aprendidos": con probabilidad
// pesoVigilar va a una zona donde ya vio a Mario; si no, ruta normal.
void MinionGuardia::elegirPuntoDePatrulla()
{
    const vector<MapaLaberinto::Celda>& libres = mapa_->celdasLibres();
    const MapaLaberinto::Celda aqui = mapa_->celdaDe(posicion_);
    const int radio = static_cast<int>(parametros_.radioPatrulla);
    hayPaso_ = false;

    MapaLaberinto::Celda zona;
    if (pesoVigilar_ > 0.0f &&
        static_cast<float>(aleatorio() % 1000u) < pesoVigilar_ * 1000.0f &&
        elegirZonaFrecuente(zona) && zona != aqui) {
        destinoPatrulla_ = mapa_->centroDe(zona);
        return;
    }

    for (int intento = 0; intento < 12 && !libres.empty(); ++intento) {
        const MapaLaberinto::Celda c = libres[aleatorio() % libres.size()];
        if (c != aqui && distanciaManhattan(c, puesto_) <= radio) {
            destinoPatrulla_ = mapa_->centroDe(c);
            return;
        }
    }
    destinoPatrulla_ = mapa_->centroDe(puesto_);
}

// ------------------------------------------------------------------
// 4. APRENDIZAJE
// ------------------------------------------------------------------

// Guarda la celda donde vio a Mario. Si ya hay una zona a 1 celda o menos,
// le sube el peso; si no, la agrega (si está lleno, reemplaza la más débil).
void MinionGuardia::registrarZona(MapaLaberinto::Celda celda)
{
    for (ZonaFrecuente& z : zonas_)
        if (distanciaManhattan(z.celda, celda) <= 1) {
            z.peso += 1.0f;
            return;
        }
    if (static_cast<int>(zonas_.size()) < MAX_ZONAS) {
        zonas_.push_back(ZonaFrecuente{celda, 1.0f});
        return;
    }
    size_t masDebil = 0;
    for (size_t i = 1; i < zonas_.size(); ++i)
        if (zonas_[i].peso < zonas_[masDebil].peso) masDebil = i;
    zonas_[masDebil] = ZonaFrecuente{celda, 1.0f};
}

// Mario se escapó: cuenta el fracaso, guarda dónde lo perdió, reacciona más
// rápido la próxima vez y vigilará más las zonas frecuentes (con límites).
void MinionGuardia::aprenderDeEscape()
{
    ++escapes_;
    registrarZona(mapa_->celdaDe(ultimaVista_));
    const float tasa = parametros_.tasaAprendizaje;
    factorReaccion_ *= (1.0f - REDUCCION_REACCION * tasa);
    if (factorReaccion_ < FACTOR_REACCION_MINIMO) factorReaccion_ = FACTOR_REACCION_MINIMO;
    pesoVigilar_ += AUMENTO_VIGILAR * tasa;
    if (pesoVigilar_ > PESO_VIGILAR_MAXIMO) pesoVigilar_ = PESO_VIGILAR_MAXIMO;
}

// Escoge la zona con más peso que no esté demasiado lejos de su puesto
// (así los Minions no abandonan su sector). Al elegirla, su peso se desgasta.
bool MinionGuardia::elegirZonaFrecuente(MapaLaberinto::Celda& destino)
{
    const int alcance = 2 * static_cast<int>(parametros_.radioPatrulla);
    ZonaFrecuente* mejor = nullptr;
    for (ZonaFrecuente& z : zonas_)
        if (distanciaManhattan(z.celda, puesto_) <= alcance && (mejor == nullptr || z.peso > mejor->peso))
            mejor = &z;
    if (mejor == nullptr) return false;
    destino = mejor->celda;
    mejor->peso *= DESGASTE_ZONA;
    return true;
}

// Generador congruencial lineal (los mismos números que usa glibc para
// rand()), con su propio estado para que cada Minion sea independiente
unsigned MinionGuardia::aleatorio()
{
    semilla_ = semilla_ * 1103515245u + 12345u;
    return semilla_ >> 8;   // los bits bajos son poco aleatorios
}