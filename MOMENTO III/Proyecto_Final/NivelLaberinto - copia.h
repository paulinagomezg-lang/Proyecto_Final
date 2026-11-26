#ifndef NIVELLABERINTO_H
#define NIVELLABERINTO_H

#include "Nivel.h"
#include "MapaLaberinto.h"
#include "NivelDificultad.h"
#include <string>
#include <vector>

using namespace std;

// Parámetros del apagón según la dificultad (más difícil = se apaga más seguido y más tiempo, el jugador ve menos y los Minions pierden menos vista)
struct ParametrosApagon
{
    float intervalo;          // s con la luz encendida antes de cada apagón
    float duracion;           // s que dura el apagón
    float factorMinion;       // fracción de su radio de visión que conservan los Minions
    float radioVisibleMario;  // m que ve el jugador alrededor de Mario a oscuras
    float aviso;              // s de aviso (parpadeo) antes de apagarse

    static ParametrosApagon segunDificultad(NivelDificultad dificultad);
};

class Protagonista;
class MinionGuardia;
class BloqueMovil;
class Personaje;


class NivelLaberinto : public Nivel
{
public:
    // contenidoMapa: el texto del mapa (la interfaz lo lee con QFile).
    // nombreMapa: solo para los mensajes de error.
    // Lanza Excepciones_del_Mapa si el mapa es inválido.
    NivelLaberinto(const string& contenidoMapa, const string& nombreMapa,
                   NivelDificultad dificultad);

    void actualizar(float dt) override;
    bool condicionVictoria() const override;
    bool condicionDerrota() const override;

    // --- Consultas para la interfaz ---
    const MapaLaberinto& mapa() const { return mapa_; }
    const vector<MinionGuardia*>& guardias() const { return guardias_; }
    const vector<BloqueMovil*>& bloques() const { return bloques_; }
    bool lucesApagadas() const { return lucesApagadas_; }
    float tiempoParaCambioDeLuz() const { return tiempoParaCambioLuz_; }
    // true los últimos 'aviso' segundos antes de un apagón (para parpadear).
    bool avisoApagon() const
    {
        return !lucesApagadas_ && tiempoParaCambioLuz_ <= apagon_.aviso;
    }
    // Radio (m) que ve el jugador alrededor de Mario mientras está oscuro.
    float radioVisibleMario() const { return apagon_.radioVisibleMario; }
    const ParametrosApagon& apagon() const { return apagon_; }
    NivelDificultad dificultad() const { return dificultad_; }

    // --- Constantes de los modelos físicos (unidades SI, celdas de 1 m) ---
    static constexpr float TAM_CELDA = 1.0f;             // m
    static constexpr float RADIO_MARIO = 0.30f;          // m
    static constexpr float RADIO_MINION = 0.30f;         // m
    static constexpr float RADIO_BLOQUE = 0.45f;         // m
    static constexpr float RADIO_CAPTURA = 0.55f;        // m (distancia para atrapar)
    static constexpr float VEL_CAMINAR = 2.5f;           // m/s
    static constexpr float VEL_CORRER = 4.0f;            // m/s
    static constexpr float MASA_MINION = 1.0f;           // kg
    static constexpr float MASA_BLOQUE = 4.0f;           // kg
    static constexpr float COEF_CONTROL = 8.0f;          // b, N·s/m (física 1)
    static constexpr float CONSTANTE_RESORTE = 17.5f;    // k, N/m (física 2): T ≈ 3 s
    static constexpr float RESTITUCION = 0.6f;           // e (física 3)
    static constexpr int MAX_CELDAS_RIEL = 3;            // amplitud máxima de un bloque (celdas)

private:
    // Física 1
    void aplicarControlDeVelocidad(Personaje& p, vector2 velocidadDeseada) const;
    // Física 2
    void aplicarResorte(BloqueMovil& b) const;
    // Física 3
    void resolverChoqueConParedes(Personaje& p, float radio) const;
    void resolverChoqueConBloque(Personaje& p, float radio, BloqueMovil& b) const;
    // Evento por tiempo
    void actualizarLuces(float dt);
    // Reglas
    void revisarCapturas();
    // Construcción
    BloqueMovil* crearBloque(MapaLaberinto::Celda celda) const;
    int celdasLibresSeguidas(MapaLaberinto::Celda desde, int dc, int df) const;

    MapaLaberinto mapa_;
    NivelDificultad dificultad_;
    vector<MinionGuardia*> guardias_;   // sin dueño (el dueño es Nivel)
    vector<BloqueMovil*> bloques_;      // sin dueño (el dueño es Nivel)
    ParametrosApagon apagon_;
    bool lucesApagadas_;
    float tiempoParaCambioLuz_;
};

#endif // NIVELLABERINTO_H
