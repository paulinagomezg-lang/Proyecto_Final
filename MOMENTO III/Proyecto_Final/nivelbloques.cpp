#include "nivelbloques.h"
#include "Protagonista.h"
#include "minionejercitocolombiano.h"
#include "Excepciones_del_Juego.h"
#include <algorithm>
#include <cmath>

using namespace std;

namespace {

// Valores de las físicas. Para ajustar el juego después de probarlo,
// solo se cambian estos números.
const float GRAVEDAD            = 9.8f;    // m/s²
const float COEF_ARRASTRE_AIRE  = 1.5f;    // c: velocidad terminal = m·g / c
const float RIGIDEZ_INICIAL     = 196.0f;  // k0: Mario normal (m = 1) se hunde m·g/k = 0.05
const float AMORTIGUAMIENTO     = 8.0f;    // b: menor que el crítico, así se ve oscilar
const float VEL_DEBILITAMIENTO  = 40.0f;   // α: pesado y quieto rompe el bloque en ~2.5 s
const float LIMITE_HUNDIMIENTO  = 0.3f;    // y_max
const float FACTOR_DEBILITAMIENTO = 0.8f;  // cada evento por tiempo: k = 0.8·k
const float VELOCIDAD_CAMINAR   = 3.0f;    // m/s
const float FACTOR_CORRER       = 1.8f;

const float RIGIDEZ_MINIMA      = 5.0f;    // k nunca llega a 0 (evita dividir por 0)
const float MASA_BLOQUE         = 1.0f;    // masa con la que vuelve a su lugar sin carga
const float ESCALON_MAXIMO      = 0.35f;   // sube solo bordes más bajos que esto
const float RADIO_ITEM          = 0.6f;
const float EPS                 = 1e-3f;
const float CAIDA_MAXIMA        = 50.0f;   // por seguridad: más abajo, capturado

// Evento por tiempo: cada cuántos segundos se debilitan todos los bloques.
float tiempoDebilitamientoSegun(NivelDificultad dificultad)
{
    switch (dificultad) {
    case NivelDificultad::FACIL:   return 16.0f;
    case NivelDificultad::NORMAL:  return 12.0f;
    case NivelDificultad::DIFICIL: return 8.0f;
    }
    return 12.0f;
}

} // namespace

// ------------------------------------------------------------------
// Mapa del juego
// ------------------------------------------------------------------
/*
 *   ___                      ___ ___                  ___     ___
 *  |   |     ___            |   |   |  ___           |   |___|   |  ___
 *  suelo A   (tierra)      suelo B (escalones)       (tierra)          suelo C  META
 *  0..6   foso 1: 6..14     14..20                   foso 2: 20..30   30..40   x=38
 */
MapaNivelBloques NivelBloques::mapaPorDefecto(NivelDificultad dificultad)
{
    MapaNivelBloques m;
    //                      xInicio ancho yArriba alto  firme
    m.bloques = {
        BloqueTierra( 0.0f, 6.0f, 0.0f, 1.0f, true),   // suelo A
        BloqueTierra( 6.5f, 1.5f, 0.8f, 0.5f, false),  // foso 1
        BloqueTierra( 9.0f, 1.5f, 1.6f, 0.5f, false),
        BloqueTierra(11.5f, 1.5f, 0.8f, 0.5f, false),
        BloqueTierra(14.0f, 6.0f, 0.0f, 1.0f, true),   // suelo B
        BloqueTierra(16.0f, 1.0f, 1.0f, 1.0f, false),  // escalones sobre el suelo
        BloqueTierra(17.0f, 1.0f, 2.0f, 2.0f, false),
        BloqueTierra(20.5f, 1.5f, 2.0f, 0.5f, false),  // foso 2
        BloqueTierra(23.0f, 1.5f, 1.2f, 0.5f, false),
        BloqueTierra(25.5f, 1.5f, 2.0f, 0.5f, false),
        BloqueTierra(28.0f, 1.5f, 0.8f, 0.5f, false),
        BloqueTierra(30.0f, 10.0f, 0.0f, 1.0f, true),  // suelo C
    };
    m.fosos = { {6.0f, 14.0f, 4.0f}, {20.0f, 30.0f, 4.0f} };

    // Ítems de poder pesado: como perjudican a Mario, hay más en difícil
    // (fácil 1, normal 2, difícil 3).
    const vector2 todos[] = { vector2(9.75f, 2.3f), vector2(24.25f, 1.9f), vector2(15.0f, 0.6f) };
    const int cantidad = dificultad == NivelDificultad::FACIL ? 1
                         : dificultad == NivelDificultad::NORMAL ? 2 : 3;
    for (int i = 0; i < cantidad; ++i) m.items.push_back(todos[i]);

    m.minions = { vector2(8.0f, -4.0f), vector2(12.0f, -4.0f),
                 vector2(22.0f, -4.0f), vector2(25.0f, -4.0f), vector2(28.0f, -4.0f) };
    m.inicioMario = vector2(1.0f, 0.0f);
    m.xMeta = 38.0f;
    return m;
}

// Construcción: aquí se valida todo y se reserva toda la memoria

NivelBloques::NivelBloques(const MapaNivelBloques& mapa, NivelDificultad dificultad)
    : gravedad_(GRAVEDAD),
    coefArrastreAire_(COEF_ARRASTRE_AIRE),
    rigidezInicial_(RIGIDEZ_INICIAL),
    amortiguamiento_(AMORTIGUAMIENTO),
    velDebilitamiento_(VEL_DEBILITAMIENTO),
    limiteHundimiento_(LIMITE_HUNDIMIENTO),
    tiempoDebilitamiento_(tiempoDebilitamientoSegun(dificultad)),
    factorDebilitamiento_(FACTOR_DEBILITAMIENTO),
    velocidadCaminar_(VELOCIDAD_CAMINAR),
    factorCorrer_(FACTOR_CORRER),
    xMeta_(mapa.xMeta),
    anchoMaximoBloque_(0.0f),
    indiceApoyo_(-1),
    indiceRecuperando_(-1),
    caidaLibre_(false),
    acumuladoDebilitamiento_(0.0f),
    eventosDebilitamiento_(0)
{
    // 1. Validar ANTES de crear nada con new: si algo falla, no queda nada a medias.
    if (mapa.bloques.empty())
        throw Excepciones_del_Juego("NivelBloques: el mapa no tiene bloques");
    for (const BloqueTierra& b : mapa.bloques) {
        Excepciones_de_Parametros_Fisicos::exigirPositivo("anchoBloque", b.ancho());
        Excepciones_de_Parametros_Fisicos::exigirPositivo("altoBloque", b.alto());
    }
    for (const Foso& f : mapa.fosos) {
        Excepciones_de_Parametros_Fisicos::exigirPositivo("anchoFoso", f.xFin - f.xInicio);
        Excepciones_de_Parametros_Fisicos::exigirPositivo("profundidadFoso", f.profundidad);
    }
    if (!(mapa.xMeta > mapa.inicioMario.x))
        throw Excepciones_del_Juego("NivelBloques: la meta debe estar delante de Mario");
    Excepciones_de_Parametros_Fisicos::exigirPositivo("gravedad", gravedad_);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("coefArrastreAire", coefArrastreAire_);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("rigidezInicial", rigidezInicial_);
    Excepciones_de_Parametros_Fisicos::exigirNoNegativo("amortiguamiento", amortiguamiento_);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("limiteHundimiento", limiteHundimiento_);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("tiempoDebilitamiento", tiempoDebilitamiento_);
    Excepciones_de_Parametros_Fisicos::exigirEnRango("factorDebilitamiento", factorDebilitamiento_, 0.0f, 1.0f);

    // 2. Copiar los datos (por valor) y ordenar los bloques por x.
    bloques_ = mapa.bloques;
    stable_sort(bloques_.begin(), bloques_.end(),
                [](const BloqueTierra& a, const BloqueTierra& b) { return a.xInicio() < b.xInicio(); });
    for (BloqueTierra& b : bloques_) {
        b.rigidez_ = rigidezInicial_;
        if (b.ancho() > anchoMaximoBloque_) anchoMaximoBloque_ = b.ancho();
    }
    fosos_ = mapa.fosos;
    items_.reserve(mapa.items.size());
    for (const vector2& p : mapa.items) items_.push_back(ItemPoder(p));
    minions_.reserve(mapa.minions.size());
    reservarPersonajes(mapa.minions.size() + 1);

    // 3. Crear los personajes. Cada new pasa enseguida a Nivel, que es
    //    el dueño: si algo falla después, ~Nivel() los libera.
    Protagonista* mario = new Protagonista(mapa.inicioMario, dificultad);
    agregarPersonaje(mario);
    protagonista_ = mario;
    for (const vector2& p : mapa.minions) {
        MinionEjercitoColombiano* minion = new MinionEjercitoColombiano(p);
        agregarPersonaje(minion);
        minions_.push_back(minion);   // ya reservado: no lanza
    }

    // 4. ¿Mario empieza parado sobre un bloque?
    for (size_t i = 0; i < bloques_.size(); ++i) {
        const BloqueTierra& b = bloques_[i];
        if (b.contieneX(mapa.inicioMario.x, MEDIO_ANCHO_MARIO) &&
            fabs(b.superficie() - mapa.inicioMario.y) < EPS) {
            indiceApoyo_ = static_cast<int>(i);
            break;
        }
    }
}


// Ciclo del juego: sin new ni excepciones

void NivelBloques::actualizar(float dt)
{
    if (!(dt > 0.0f)) return;

    avanzarTiempo(dt);
    debilitarBloques(dt);                          // evento por tiempo
    actualizarFlotacionItems(tiempoTranscurrido());

    Protagonista* mario = protagonista_;
    if (mario->estaCapturado()) return;            // el nivel ya terminó

    const vector2 anterior = mario->posicion();
    moverHorizontal(mario);

    // --- Física vertical ---
    if (indiceApoyo_ >= 0) {
        BloqueTierra& bloque = bloques_[static_cast<size_t>(indiceApoyo_)];
        if (mario->consumirSalto()) {
            aplicarSaltoParabolico(mario);         // física 1
        } else {
            // Pesado y quieto: el bloque se va debilitando (k = k - α·t).
            const bool quieto = mario->direccionEfectiva().normaCuadrada() == 0.0f;
            actualizarResorteBloque(bloque, mario->masa(), true,
                                    mario->esPesado() && quieto, dt);   // física 2
            if (bloque.roto()) {
                indiceApoyo_ = -1;
                caidaLibre_ = true;                // empieza la física 3
            } else {
                mario->setVelocidad(vector2(mario->velocidad().x, 0.0f));
            }
        }
    }

    // El último bloque que dejó vuelve a su lugar (sin carga).
    if (indiceRecuperando_ >= 0 && indiceRecuperando_ != indiceApoyo_) {
        BloqueTierra& b = bloques_[static_cast<size_t>(indiceRecuperando_)];
        actualizarResorteBloque(b, MASA_BLOQUE, false, false, dt);
        if (fabs(b.hundimiento_) < EPS && fabs(b.velHundimiento_) < EPS) {
            b.hundimiento_ = 0.0f;
            b.velHundimiento_ = 0.0f;
            indiceRecuperando_ = -1;
        }
    }

    if (indiceApoyo_ < 0) {
        mario->aplicarFuerza(vector2(0.0f, -mario->masa() * gravedad_));   // peso
        if (caidaLibre_ || anterior.y < 0.0f)
            aplicarCaidaConArrastre(mario);                                // física 3
    }

    // --- Cada personaje integra con las fuerzas que el nivel aplicó ---
    actualizarPersonajes(dt);

    resolverColisiones(mario, anterior);
    recogerItems(mario);
    revisarCaptura(mario);
}

bool NivelBloques::condicionVictoria() const
{
    return !protagonista_->estaCapturado() && protagonista_->posicion().x >= xMeta_;
}

bool NivelBloques::condicionDerrota() const
{
    return protagonista_->estaCapturado();
}


// Físicas del nivel 1


// Física 1: tiro parabólico con impulso fijo.
void NivelBloques::aplicarSaltoParabolico(Protagonista* mario)
{
    const float v0 = mario->impulsoSalto() / mario->masa();
    mario->setVelocidad(vector2(mario->velocidad().x, v0));
    indiceRecuperando_ = indiceApoyo_;
    indiceApoyo_ = -1;
    caidaLibre_ = false;
}

// Física 2: resorte amortiguado.
void NivelBloques::actualizarResorteBloque(BloqueTierra& bloque, float masa, bool cargado,
                                           bool debilitar, float dt)
{
    if (bloque.esFirme() || bloque.roto()) return;

    if (debilitar)
        bloque.rigidez_ = max(RIGIDEZ_MINIMA, bloque.rigidez_ - velDebilitamiento_ * dt);

    const float inversaMasa = 1.0f / masa;          // masa > 0: la validó Personaje
    const float peso = cargado ? gravedad_ : 0.0f;
    const float a = peso
                    - bloque.rigidez_ * inversaMasa * bloque.hundimiento_
                    - amortiguamiento_ * inversaMasa * bloque.velHundimiento_;

    bloque.velHundimiento_ += a * dt;               // primero la velocidad
    bloque.hundimiento_    += bloque.velHundimiento_ * dt;   // luego la posición

    if (bloque.hundimiento_ > limiteHundimiento_)
        bloque.roto_ = true;
}

// Física 3: caída con resistencia del aire
void NivelBloques::aplicarCaidaConArrastre(Protagonista* mario)
{
    mario->aplicarFuerza(vector2(0.0f, -coefArrastreAire_ * mario->velocidad().y));
}

// Evento por tiempo: cada 'tiempoDebilitamiento_' segundos, todos los
// bloques de tierra pierden rigidez.
void NivelBloques::debilitarBloques(float dt)
{
    if (eventosDebilitamiento_ >= EVENTOS_PARA_ROMPER) return;
    acumuladoDebilitamiento_ += dt;
    if (acumuladoDebilitamiento_ < tiempoDebilitamiento_) return;

    acumuladoDebilitamiento_ -= tiempoDebilitamiento_;
    ++eventosDebilitamiento_;
    const bool romper = eventosDebilitamiento_ >= EVENTOS_PARA_ROMPER;
    for (BloqueTierra& b : bloques_) {
        if (b.esFirme() || b.roto()) continue;
        if (romper) {
            b.roto_ = true;
        } else {
            b.agrietado_ = true;
            b.rigidez_ = max(RIGIDEZ_MINIMA, b.rigidez_ * factorDebilitamiento_);
        }
    }
}

// Decorativa: el ítem sube y baja (movimiento armónico simple).
void NivelBloques::actualizarFlotacionItems(float t)
{
    for (ItemPoder& item : items_) {
        if (item.recogido_) continue;
        item.posicion_ = item.posicionBase_
                         + vector2(0.0f, item.amplitud_ * sin(item.omega_ * t + item.fase_));
    }
}

// Reglas del nivel


// Movimiento horizontal: no es una de las físicas (es rectilíneo).
// La velocidad depende de correr, del cansancio y de "pesado".
void NivelBloques::moverHorizontal(Protagonista* mario)
{
    const float rapidez = velocidadCaminar_
                          * (mario->estaCorriendo() ? factorCorrer_ : 1.0f)
                          * mario->factorVelocidad();
    const float vx = mario->direccionEfectiva().x * rapidez;
    mario->setVelocidad(vector2(vx, mario->velocidad().y));
}

size_t NivelBloques::primerBloqueCercano(float x) const
{
    // Ningún bloque más a la izquierda de esto puede tocar la x dada.
    const float desde = x - anchoMaximoBloque_ - MEDIO_ANCHO_MARIO;
    auto it = lower_bound(bloques_.begin(), bloques_.end(), desde,
                          [](const BloqueTierra& b, float valor) { return b.xInicio() < valor; });
    return static_cast<size_t>(it - bloques_.begin());
}

void NivelBloques::resolverColisiones(Protagonista* mario, vector2 anterior)
{
    vector2 p = mario->posicion();
    vector2 v = mario->velocidad();
    const float hw = MEDIO_ANCHO_MARIO;

    // Borde izquierdo del mapa
    if (p.x < hw) { p.x = hw; v.x = 0.0f; }

    // Dentro de un foso (por debajo del suelo) sus paredes no lo dejan salir:
    // sin esto podría "pasar por debajo" del suelo de al lado.
    if (p.y < 0.0f) {
        for (const Foso& f : fosos_) {
            if (!f.contieneX(anterior.x)) continue;
            if (p.x < f.xInicio + hw) { p.x = f.xInicio + hw; v.x = 0.0f; }
            if (p.x > f.xFin - hw)    { p.x = f.xFin - hw;    v.x = 0.0f; }
            break;
        }
    }

    // Sigue sobre su bloque, o se salió por el borde
    if (indiceApoyo_ >= 0) {
        const BloqueTierra& b = bloques_[static_cast<size_t>(indiceApoyo_)];
        if (b.contieneX(p.x, hw)) {
            p.y = b.superficie();
            v.y = 0.0f;
        } else {
            indiceRecuperando_ = indiceApoyo_;
            indiceApoyo_ = -1;
        }
    }

    const float xMin = min(anterior.x, p.x) - hw;
    const float xMax = max(anterior.x, p.x) + hw;
    const size_t inicio = primerBloqueCercano(xMin);

    // Paso 1: aterrizar o subir un escalón pequeño. Gana el bloque más alto.
    int mejor = -1;
    float mejorAltura = 0.0f;
    for (size_t i = inicio; i < bloques_.size() && bloques_[i].xInicio() < xMax; ++i) {
        const BloqueTierra& b = bloques_[i];
        if (b.roto() || static_cast<int>(i) == indiceApoyo_ || !b.contieneX(p.x, hw)) continue;
        const float arriba = b.superficie();
        const bool aterriza = v.y <= 0.0f && anterior.y >= arriba - EPS && p.y <= arriba;
        const bool escalon  = indiceApoyo_ >= 0 && arriba > p.y && arriba - p.y <= ESCALON_MAXIMO;
        if ((aterriza || escalon) && (mejor < 0 || arriba > mejorAltura)) {
            mejor = static_cast<int>(i);
            mejorAltura = arriba;
        }
    }
    if (mejor >= 0) {
        if (indiceApoyo_ >= 0 && indiceApoyo_ != mejor) indiceRecuperando_ = indiceApoyo_;
        indiceApoyo_ = mejor;
        p.y = mejorAltura;
        v.y = 0.0f;
        caidaLibre_ = false;
    }

    // Paso 2: techos y paredes de los demás bloques.
    for (size_t i = inicio; i < bloques_.size() && bloques_[i].xInicio() < xMax; ++i) {
        const BloqueTierra& b = bloques_[i];
        if (b.roto() || static_cast<int>(i) == indiceApoyo_ || !b.contieneX(p.x, hw)) continue;
        const float arriba = b.superficie();
        const float abajo = b.yAbajo();
        if (!(p.y < arriba - EPS && p.y + ALTO_MARIO > abajo + EPS)) continue;   // no se tocan

        if (v.y > 0.0f && anterior.y + ALTO_MARIO <= abajo + EPS) {
            p.y = abajo - ALTO_MARIO;          // se golpeó la cabeza
            v.y = 0.0f;
        } else if (anterior.x <= b.xInicio()) {
            p.x = b.xInicio() - hw;            // pared por la izquierda
            v.x = 0.0f;
        } else if (anterior.x >= b.xFin()) {
            p.x = b.xFin() + hw;               // pared por la derecha
            v.x = 0.0f;
        }
    }

    mario->setPosicion(p);
    mario->setVelocidad(v);
}

void NivelBloques::recogerItems(Protagonista* mario)
{
    // Se compara con el centro de Mario y la distancia al cuadrado (sin raíz).
    const vector2 centro = mario->posicion() + vector2(0.0f, ALTO_MARIO * 0.5f);
    for (ItemPoder& item : items_) {
        if (item.recogido_) continue;
        if (centro.distanciaCuadrada(item.posicion_) <= RADIO_ITEM * RADIO_ITEM) {
            item.recogido_ = true;
            mario->activarPoderPesado();
        }
    }
}

void NivelBloques::revisarCaptura(Protagonista* mario)
{
    const vector2 p = mario->posicion();
    if (p.y >= 0.0f) return;          // por encima del suelo nadie lo alcanza

    for (const MinionEjercitoColombiano* minion : minions_) {
        if (minion->puedeCapturar(p)) { mario->capturar(); return; }
    }
    // Si llega al fondo del foso, el ejército lo captura aunque no haya
    // un Minion justo debajo.
    for (const Foso& f : fosos_) {
        if (f.contieneX(p.x) && p.y <= f.fondo()) { mario->capturar(); return; }
    }
    if (p.y < -CAIDA_MAXIMA) mario->capturar();
}