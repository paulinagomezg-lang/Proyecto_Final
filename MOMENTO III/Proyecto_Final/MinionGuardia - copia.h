#ifndef MINIONGUARDIA_H
#define MINIONGUARDIA_H
#include "enemigo.h"
#include "MapaLaberinto.h"
#include "NivelDificultad.h"

enum class EstadoAgente
{
    PATRULLAR,   // camina entre puntos cercanos a su puesto
    SOSPECHAR,   // vio algo: se detiene, mira hacia allá y se llena la alerta
    PERSEGUIR,   // corre hacia Mario por el camino más corto
    BUSCAR       // va a donde vio a Mario por última vez y mira alrededor
};

// Parámetros del agente que cambian con la dificultad.
struct ParametrosGuardia
{
    float radioVision;       // m
    float aperturaCono;      // rad (ángulo total)
    float velPatrulla;       // m/s
    float velPersecucion;    // m/s
    float tiempoAlerta;      // s viendo a Mario para pasar de SOSPECHAR a PERSEGUIR
    float tiempoBusqueda;    // s mirando alrededor en BUSCAR
    float radioOido;         // m: si Mario está dormido, lo oye roncar (sin cono)
    float radioPatrulla;     // celdas (distancia Manhattan) alrededor de su puesto

    static ParametrosGuardia segunDificultad(NivelDificultad dificultad);
};

class MinionGuardia : public Enemigo
{
public:
    // semilla: para que cada Minion patrulle distinto pero de forma
    // repetible (las pruebas dan siempre el mismo resultado).
    MinionGuardia(const MapaLaberinto& mapa, MapaLaberinto::Celda puesto, const ParametrosGuardia& parametros, unsigned semilla, float masa = 1.0f);

    // Percepción + decisión + acción. NivelLaberinto lo llama en cada
    // fotograma ANTES de aplicar las fuerzas.
    void pensar(float dt, vector2 posicionMario, bool marioDormido);

    // Solo integra el movimiento con las fuerzas que aplicó el nivel.
    void actualizar(float dt) override { integrar(dt); }

    //  Lo que usa el nivel
    vector2 velocidadDeseada() const { return velocidadDeseada_; }

    // Evento de apagón: con las luces apagadas ve menos (factor entre 0 y 1).
    // Solo recalcula cuando el factor cambia.
    void setFactorLuz(float factor);

    //Consultas (interfaz y pruebas)
    EstadoAgente estado() const { return estado_; }
    float alerta() const { return alerta_; }
    bool veAMario() const { return veAMario_; }
    vector2 ultimaPosicionVista() const { return ultimaVista_; }
    const ParametrosGuardia& parametros() const { return parametros_; }
    MapaLaberinto::Celda puesto() const { return puesto_; }

private:
    // Percepción
    bool percibe(vector2 posicionMario, bool marioDormido) const;
    // Acción: moverse hacia un punto por el camino más corto.
    // Devuelve true si ya llegó.
    bool irHacia(vector2 destino, float velocidad);
    void mirarHacia(vector2 punto);
    void elegirPuntoDePatrulla();
    void cambiarEstado(EstadoAgente nuevo);
    unsigned aleatorio();   // generador congruencial lineal (sin <random>)

    const MapaLaberinto* mapa_;   // no es dueño
    ParametrosGuardia parametros_;
    MapaLaberinto::Celda puesto_;

    EstadoAgente estado_;
    float alerta_;
    bool veAMario_;
    vector2 ultimaVista_;
    vector2 destinoPatrulla_;
    float tiempoEnEstado_;
    float factorLuz_;
    bool llegoAlPunto_;           // BUSCAR: ya llegó y está mirando alrededor

    // Ruta: siguiente celda del camino y el destino con que se calculó,
    // para recalcular solo cuando hace falta.
    MapaLaberinto::Celda pasoActual_;
    MapaLaberinto::Celda destinoRuta_;
    bool hayPaso_;

    vector2 velocidadDeseada_;
    unsigned semilla_;
};

#endif // MINIONGUARDIA_H
