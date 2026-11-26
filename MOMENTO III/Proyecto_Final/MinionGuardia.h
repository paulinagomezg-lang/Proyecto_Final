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
    float tasaAprendizaje = 1.0f;  // qué tanto cambia con cada encuentro (más en difícil)

    static ParametrosGuardia segunDificultad(NivelDificultad dificultad);
};

// 4. APRENDIZAJE: una zona donde el agente ha visto a Mario y qué tanto
// pesa (sube cada vez que lo vuelve a ver cerca de ahí).
struct ZonaFrecuente
{
    MapaLaberinto::Celda celda;
    float peso;
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

    // Aprendizaje (consultas para la interfaz y las pruebas)
    int escapes() const { return escapes_; }                 // encuentros que Mario ganó
    float factorReaccion() const { return factorReaccion_; } // 1 = normal; menos = reacciona antes
    float pesoVigilar() const { return pesoVigilar_; }       // prob. de ir a una zona frecuente
    const vector<ZonaFrecuente>& zonasFrecuentes() const { return zonas_; }

    // Mínimo al que puede bajar el factor de reacción (no se vuelve imposible).
    static constexpr float FACTOR_REACCION_MINIMO = 0.5f;
    static constexpr float PESO_VIGILAR_MAXIMO = 0.7f;
    static const int MAX_ZONAS = 6;

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

    // 4. Aprendizaje
    void registrarZona(MapaLaberinto::Celda celda);   // guarda dónde vio a Mario
    void aprenderDeEscape();                           // al terminar un encuentro sin captura
    bool elegirZonaFrecuente(MapaLaberinto::Celda& destino);

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

    // Lo aprendido (se conserva durante todo el intento del nivel).
    vector<ZonaFrecuente> zonas_;   // capacidad reservada una vez (MAX_ZONAS)
    int escapes_;
    float factorReaccion_;          // multiplica tiempoAlerta
    float pesoVigilar_;             // 0 = solo ruta normal; sube con cada escape
};

#endif // MINIONGUARDIA_H