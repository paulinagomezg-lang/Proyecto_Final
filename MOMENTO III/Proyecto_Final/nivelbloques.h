#ifndef NIVELBLOQUES_H
#define NIVELBLOQUES_H

#include "Nivel.h"
#include "NivelDificultad.h"
#include "bloquetierra.h"
#include "itempoder.h"
#include "foso.h"
#include "vector2.h"
#include <vector>

using namespace std;

class Protagonista;
class MinionEjercitoColombiano;

// Descripción del mapa: qué hay y dónde. La usa el constructor.
struct MapaNivelBloques
{
    vector<BloqueTierra> bloques;
    vector<Foso>         fosos;
    vector<vector2>      items;          // posiciones base de los ítems de poder
    vector<vector2>      minions;        // posiciones de los Minions (en los fosos)
    vector2              inicioMario;
    float                xMeta = 0.0f;
};

class NivelBloques : public Nivel
{
public:
    // Lanza Excepciones_del_Juego si el mapa no es válido.
    NivelBloques(const MapaNivelBloques& mapa, NivelDificultad dificultad);

    // El mapa del juego, según la dificultad.
    static MapaNivelBloques mapaPorDefecto(NivelDificultad dificultad);

    // --- Nivel ---
    void actualizar(float dt) override;
    bool condicionVictoria() const override;
    bool condicionDerrota() const override;

    // --- Consultas para la interfaz y las pruebas ---
    const vector<BloqueTierra>& bloques() const { return bloques_; }
    const vector<ItemPoder>&    items() const   { return items_; }
    const vector<Foso>&         fosos() const   { return fosos_; }
    float xMeta() const                         { return xMeta_; }
    int   indiceBloqueApoyo() const             { return indiceApoyo_; }   // -1 si está en el aire
    bool  enCaidaLibre() const                  { return caidaLibre_; }
    float tiempoDebilitamiento() const          { return tiempoDebilitamiento_; }
    float tiempoHastaDebilitar() const          { return tiempoDebilitamiento_ - acumuladoDebilitamiento_; }
    float limiteHundimiento() const             { return limiteHundimiento_; }
    float rigidezInicial() const                { return rigidezInicial_; }
    float gravedad() const                      { return gravedad_; }
    float coefArrastreAire() const              { return coefArrastreAire_; }

    // Tamaño de Mario para las colisiones (los pies están en su posición).
    static constexpr float MEDIO_ANCHO_MARIO = 0.3f;
    static constexpr float ALTO_MARIO = 0.9f;

private:
    // --- Físicas del nivel 1 ---
    void aplicarSaltoParabolico(Protagonista* mario);
    void actualizarResorteBloque(BloqueTierra& bloque, float masa, bool cargado, bool debilitar, float dt);
    void aplicarCaidaConArrastre(Protagonista* mario);
    void debilitarBloques(float dt);
    void actualizarFlotacionItems(float t);

    // --- Reglas ---
    void moverHorizontal(Protagonista* mario);
    void resolverColisiones(Protagonista* mario, vector2 anterior);
    void recogerItems(Protagonista* mario);
    void revisarCaptura(Protagonista* mario);

    // Primer bloque que podría tocar la x dada (búsqueda binaria).
    size_t primerBloqueCercano(float x) const;

    // Parámetros de las físicas (atributos del nivel)
    float gravedad_;              // g
    float coefArrastreAire_;      // c
    float rigidezInicial_;        // k0
    float amortiguamiento_;       // b
    float velDebilitamiento_;     // α: cuánto baja k por segundo con pesado y quieto
    float limiteHundimiento_;     // y_max: si se hunde más, el bloque se rompe
    float tiempoDebilitamiento_;  // cada cuántos segundos se debilitan todos
    float factorDebilitamiento_;  // k se multiplica por esto en cada evento
    float velocidadCaminar_;      // m/s
    float factorCorrer_;          // correr = caminar · factor

    // Datos del mapa
    vector<BloqueTierra> bloques_;     // ordenados por xInicio
    vector<ItemPoder>    items_;
    vector<Foso>         fosos_;
    vector<MinionEjercitoColombiano*> minions_;   // no es dueño (Nivel los libera)
    float xMeta_;
    float anchoMaximoBloque_;          // para la búsqueda de cercanos

    // Estado
    int   indiceApoyo_;          // bloque que Mario pisa, o -1
    int   indiceRecuperando_;    // último bloque que dejó (vuelve a su lugar)
    bool  caidaLibre_;           // cayendo porque se rompió el bloque o hacia un foso
    float acumuladoDebilitamiento_;
};

#endif // NIVELBLOQUES_H
