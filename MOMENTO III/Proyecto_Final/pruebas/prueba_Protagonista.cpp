// Pruebas de Protagonista (Mario).
// No tiene main(): main_pruebas.cpp llama a pruebasProtagonista().

#include "protagonista.h"
#include "Excepciones_del_Juego.h"
#include <cstdio>
#include <cmath>
using namespace std;

// Todo lo de esta prueba queda privado a este archivo, para que los
// nombres (verificar, fallas, total...) no choquen con las otras pruebas.
namespace {

int fallas = 0, total = 0;
void verificar(bool c, const char* d)
{
    ++total;
    printf("  %s  %s\n", c ? "OK   " : "FALLA", d);
    if (!c) ++fallas;
}
bool casi(float a, float b, float tol = 1e-3f) { return fabs(a - b) < tol; }

const float DT = 1.0f / 60.0f;

// Simula 'segundos' de juego llamando a actualizar() a 60 fps.
void simular(Protagonista& p, float segundos)
{
    const int pasos = static_cast<int>(segundos * 60.0f + 0.5f);
    for (int i = 0; i < pasos; ++i) p.actualizar(DT);
}

// Para contar destrucciones a traves de Personaje*.
int vivos = 0;
class ProtagonistaContado : public Protagonista
{
public:
    ProtagonistaContado() : Protagonista(vector2(), NivelDificultad::NORMAL) { ++vivos; }
    ~ProtagonistaContado() override { --vivos; }
};

void ejecutar(int& correctas, int& totales)
{
    printf("Construccion\n");
    {
        Protagonista m(vector2(2, 3), NivelDificultad::NORMAL);
        verificar(m.posicion() == vector2(2, 3), "guarda la posicion inicial");
        verificar(casi(m.masa(), 1.0f), "empieza con la masa normal");
        verificar(m.estado() == EstadoProtagonista::NORMAL && m.puedeMoverse(), "empieza NORMAL y puede moverse");
        verificar(!m.esPesado() && casi(m.proporcionMasa(), 1.0f), "empieza sin el poder pesado");
        verificar(casi(m.energia().porcentaje(), 1.0f), "empieza con la energia llena");

        bool lanzo = false;
        try { Protagonista p(vector2(), NivelDificultad::NORMAL, 0.0f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "masa 0 lanza excepcion");
        lanzo = false;
        try { Protagonista p(vector2(), NivelDificultad::NORMAL, 1.0f, 0.5f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "factorPesado menor que 1 lanza excepcion");
        lanzo = false;
        try { Protagonista p(vector2(), NivelDificultad::NORMAL, 1.0f, 3.0f, -2.0f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "impulso de salto negativo lanza excepcion");
        lanzo = false;
        try { Protagonista p(vector2(), NivelDificultad::NORMAL, 1.0f, 3.0f, 6.0f, 1.5f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "lentitud del poder mayor que 1 lanza excepcion");
    }

    printf("Poder pesado\n");
    {
        Protagonista m(vector2(), NivelDificultad::NORMAL, 1.0f, 3.0f, 6.0f);
        const float v0Normal = m.impulsoSalto() / m.masa();
        const float velNormal = m.factorVelocidad();
        m.activarPoderPesado();                       // Mario se come el item
        verificar(m.esPesado() && casi(m.masa(), 3.0f), "activar el poder triplica la masa");
        verificar(casi(m.factorVelocidad(), velNormal * 0.6f), "pesado se mueve mas lento (60 % de su velocidad)");
        verificar(casi(m.proporcionMasa(), 3.0f), "proporcionMasa = 3");
        const float v0Pesado = m.impulsoSalto() / m.masa();
        verificar(casi(v0Pesado, v0Normal / 3.0f), "con el mismo impulso salta con un tercio de velocidad (v0 = J/m)");
        verificar(casi((v0Pesado * v0Pesado) / (v0Normal * v0Normal), 1.0f / 9.0f),
                  "la altura del salto (v0^2/2g) baja a la novena parte");
        m.activarPoderPesado();
        verificar(casi(m.masa(), 3.0f), "activarlo dos veces no la multiplica otra vez");
        m.desactivarPoderPesado();
        verificar(!m.esPesado() && casi(m.masa(), 1.0f), "desactivarlo vuelve a la masa normal");
        verificar(casi(m.factorVelocidad(), velNormal), "sin el poder vuelve a su velocidad normal");
        verificar(casi(m.tiempoPesadoRestante(), 0.0f), "sin el poder no le queda tiempo pesado");
    }

    printf("Duracion del poder pesado\n");
    {
        Protagonista facil(vector2(), NivelDificultad::FACIL);
        Protagonista normal(vector2(), NivelDificultad::NORMAL);
        Protagonista dificil(vector2(), NivelDificultad::DIFICIL);
        verificar(casi(facil.duracionPesado(), 10.0f) && casi(normal.duracionPesado(), 15.0f) &&
                      casi(dificil.duracionPesado(), 20.0f),
                  "dura 10 s en facil, 15 s en normal y 20 s en dificil");

        normal.activarPoderPesado();
        verificar(casi(normal.tiempoPesadoRestante(), 15.0f), "al comerse el item le quedan 15 s");
        simular(normal, 14.9f);
        verificar(normal.esPesado() && casi(normal.masa(), 3.0f), "a los 14.9 s sigue pesado");
        simular(normal, 0.2f);
        verificar(!normal.esPesado() && casi(normal.masa(), 1.0f) && casi(normal.tiempoPesadoRestante(), 0.0f),
                  "a los 15 s se le quita solo y vuelve a su masa normal");

        dificil.activarPoderPesado();
        simular(dificil, 15.5f);
        verificar(dificil.esPesado(), "en dificil a los 15.5 s todavia sigue pesado");
        simular(dificil, 4.6f);
        verificar(!dificil.esPesado(), "en dificil se le quita a los 20 s");

        Protagonista otro(vector2(), NivelDificultad::NORMAL);
        otro.activarPoderPesado();
        simular(otro, 10.0f);
        otro.activarPoderPesado();                    // se come otro item
        verificar(casi(otro.tiempoPesadoRestante(), 15.0f) && casi(otro.masa(), 3.0f),
                  "otro item reinicia el tiempo sin multiplicar la masa otra vez");
        simular(otro, 10.0f);
        verificar(otro.esPesado(), "con el tiempo reiniciado sigue pesado 10 s despues");
    }

    printf("Entrada y salto\n");
    {
        Protagonista m(vector2(), NivelDificultad::NORMAL);
        m.setEntrada(vector2(1, 1), false);
        verificar(casi(m.direccionEfectiva().norma(), 1.0f), "en diagonal la direccion se normaliza (no va mas rapido)");
        m.setEntrada(vector2(0, 0), false);
        verificar(m.direccionEfectiva() == vector2(0, 0), "sin teclas la direccion es (0,0)");

        m.solicitarSalto();
        verificar(m.consumirSalto(), "un pedido de salto se entrega al nivel");
        verificar(!m.consumirSalto(), "y se entrega una sola vez");

        m.solicitarSalto();
        m.actualizar(DT);                 // el nivel no lo uso en este fotograma
        verificar(!m.consumirSalto(), "un pedido de salto dura un solo fotograma");

    }

    printf("Mirada\n");
    {
        // Nivel 1: solo derecha e izquierda.
        Protagonista m(vector2(), NivelDificultad::NORMAL);
        verificar(m.mirada() == Mirada::DERECHA, "nivel 1: empieza mirando a la derecha");
        m.setEntrada(vector2(-1, 0), false);
        verificar(m.mirada() == Mirada::IZQUIERDA, "nivel 1: al ir a la izquierda mira a la izquierda");
        m.setEntrada(vector2(0, 1), false);
        verificar(m.mirada() == Mirada::IZQUIERDA, "nivel 1: moverse en vertical no lo voltea (no mira arriba)");
        m.setEntrada(vector2(), false);
        verificar(m.mirada() == Mirada::IZQUIERDA, "nivel 1: al quedarse quieto conserva la mirada");
        m.setEntrada(vector2(1, 0), false);
        verificar(m.mirada() == Mirada::DERECHA, "nivel 1: al ir a la derecha mira a la derecha");

        // Laberinto: cuatro direcciones.
        Protagonista l(vector2(), NivelDificultad::NORMAL);
        l.usarMiradaCuatroDirecciones(true);
        verificar(l.mirada() == Mirada::ABAJO, "laberinto: aparece de frente");
        l.setEntrada(vector2(0, 1), false);
        verificar(l.mirada() == Mirada::ARRIBA, "laberinto: al subir se ve de espaldas");
        l.setEntrada(vector2(0, -1), false);
        verificar(l.mirada() == Mirada::ABAJO, "laberinto: al bajar se ve de frente");
        l.setEntrada(vector2(-1, 0), false);
        verificar(l.mirada() == Mirada::IZQUIERDA, "laberinto: tambien mira a la izquierda");
        l.setEntrada(vector2(1, 0.3f), false);
        verificar(l.mirada() == Mirada::DERECHA, "laberinto: en diagonal manda el eje en que mas se mueve");
        l.setEntrada(vector2(0.3f, 1), false);
        verificar(l.mirada() == Mirada::ARRIBA, "laberinto: diagonal mas vertical mira arriba");
        l.setEntrada(vector2(-1, -1), false);
        verificar(l.mirada() == Mirada::IZQUIERDA, "laberinto: diagonal exacta gana el horizontal");
        l.setEntrada(vector2(), false);
        verificar(l.mirada() == Mirada::IZQUIERDA, "laberinto: al quedarse quieto conserva la mirada");

        l.setEntrada(vector2(0, 1), false);
        l.usarMiradaCuatroDirecciones(false);
        verificar(l.mirada() == Mirada::DERECHA, "al volver a dos direcciones, si miraba arriba pasa a la derecha");
    }

    printf("Correr y energia\n");
    {
        Protagonista m(vector2(), NivelDificultad::NORMAL);
        m.setEntrada(vector2(1, 0), true);
        simular(m, 1.0f);
        verificar(m.estaCorriendo(), "con la tecla de correr y moviendose, corre");
        verificar(casi(m.energia().energia(), 75.0f, 0.1f), "1 s corriendo en normal gasta 25 de energia");

        Protagonista quieto(vector2(), NivelDificultad::NORMAL);
        quieto.setEntrada(vector2(0, 0), true);
        simular(quieto, 1.0f);
        verificar(!quieto.estaCorriendo() && casi(quieto.energia().porcentaje(), 1.0f),
                  "si no se mueve no cuenta como correr (no gasta)");

        Protagonista pesado(vector2(), NivelDificultad::NORMAL);
        pesado.activarPoderPesado();
        pesado.setEntrada(vector2(1, 0), true);
        simular(pesado, 1.0f);
        verificar(casi(100.0f - pesado.energia().energia(), 3.0f * 25.0f, 0.2f),
                  "pesado gasta el triple de energia al correr");

        Protagonista agotado(vector2(), NivelDificultad::NORMAL);
        agotado.setEntrada(vector2(1, 0), true);
        simular(agotado, 5.0f);
        verificar(!agotado.estaCorriendo() && agotado.factorVelocidad() < 1.0f,
                  "agotado deja de correr y va mas lento");
        const float soloCansado = agotado.factorVelocidad();
        agotado.activarPoderPesado();
        verificar(casi(agotado.factorVelocidad(), soloCansado * 0.6f),
                  "pesado y cansado a la vez: las dos lentitudes se combinan");
    }

    printf("Dormilon\n");
    {
        Protagonista nivel1(vector2(), NivelDificultad::NORMAL);
        simular(nivel1, 10.0f);
        verificar(nivel1.estado() == EstadoProtagonista::NORMAL, "sin habilitarlo (nivel 1) nunca se duerme");
        verificar(casi(nivel1.tiempoQuieto(), 10.0f, 0.05f), "pero el tiempo quieto se cuenta (lo usa el nivel 1)");
        nivel1.setEntrada(vector2(1, 0), false);
        nivel1.actualizar(DT);
        verificar(nivel1.tiempoQuieto() == 0.0f, "moverse reinicia el tiempo quieto");

        Protagonista salto(vector2(), NivelDificultad::NORMAL);
        salto.habilitarSueno(true);
        salto.solicitarSalto();           // en el laberinto nadie consume el salto
        simular(salto, 4.0f);
        verificar(salto.estaDormido(), "presionar saltar una vez no impide que se duerma despues");

        Protagonista m(vector2(), NivelDificultad::NORMAL);   // se duerme a los 3.5 s
        m.habilitarSueno(true);
        simular(m, 3.0f);
        verificar(m.estado() == EstadoProtagonista::NORMAL, "3 s quieto en normal: todavia despierto");
        simular(m, 0.6f);
        verificar(m.estado() == EstadoProtagonista::DORMIDO && m.estaDormido(), "pasados 3.5 s quieto se duerme");

        m.setEntrada(vector2(1, 0), true);
        verificar(m.direccionEfectiva() == vector2(0, 0) && !m.puedeMoverse(), "dormido no se mueve aunque presione teclas");
        m.solicitarSalto();
        verificar(!m.consumirSalto(), "dormido no puede saltar");

        simular(m, 2.0f);
        verificar(m.estado() == EstadoProtagonista::DESPERTANDO && m.estaDormido(),
                  "despues de dormir 2 s pasa a DESPERTANDO (sigue vulnerable)");
        simular(m, 1.0f);
        verificar(m.estado() == EstadoProtagonista::NORMAL && m.puedeMoverse(),
                  "despues de 1 s mas vuelve a NORMAL");

        Protagonista activo(vector2(), NivelDificultad::NORMAL);
        activo.habilitarSueno(true);
        for (int i = 0; i < 10; ++i) {          // 10 veces: 3 s quieto y luego se mueve
            activo.setEntrada(vector2(0, 0), false);
            simular(activo, 3.0f);
            activo.setEntrada(vector2(1, 0), false);
            simular(activo, 0.1f);
        }
        verificar(activo.estado() == EstadoProtagonista::NORMAL, "moverse a tiempo reinicia el contador de sueno");

        Protagonista ciclos(vector2(), NivelDificultad::NORMAL);
        ciclos.habilitarSueno(true);
        int veces = 0;
        bool dormidoAntes = false;
        for (int i = 0; i < 60 * 6500; ++i) {     // 6500 s quieto a 60 fps
            ciclos.actualizar(DT);
            if (ciclos.estado() == EstadoProtagonista::DORMIDO && !dormidoAntes) ++veces;
            dormidoAntes = (ciclos.estado() == EstadoProtagonista::DORMIDO);
        }
        printf("     en 6500 s quieto se durmio %d veces (una cada 6.5 s)\n", veces);
        verificar(veces == 1000, "se vuelve a dormir cada vez que se queda quieto (1000 veces seguidas)");

        Protagonista facil(vector2(), NivelDificultad::FACIL);
        Protagonista dificil(vector2(), NivelDificultad::DIFICIL);
        verificar(facil.tiempoParaDormir() > dificil.tiempoParaDormir(),
                  "en dificil se duerme mas rapido que en facil");
    }

    printf("Captura\n");
    {
        Protagonista m(vector2(), NivelDificultad::NORMAL);
        m.habilitarSueno(true);
        m.setEntrada(vector2(1, 0), true);
        m.capturar();
        verificar(m.estaCapturado() && !m.puedeMoverse(), "capturado no puede moverse");
        verificar(m.direccionEfectiva() == vector2(0, 0), "capturado ignora las teclas");
        simular(m, 10.0f);
        verificar(m.estaCapturado(), "capturado no se duerme ni cambia de estado");
    }

    printf("Movimiento (fuerzas del nivel)\n");
    {
        Protagonista m(vector2(0, 0), NivelDificultad::NORMAL);   // masa 1
        m.aplicarFuerza(vector2(0, -9.8f));
        m.actualizar(1.0f);
        verificar(casi(m.velocidad().y, -9.8f) && casi(m.posicion().y, -9.8f),
                  "integra con la fuerza que aplica el nivel (Euler semi-implicito)");

        // Salto como lo hara NivelBloques: v0 = J/m y gravedad del nivel.
        Protagonista s(vector2(0, 0), NivelDificultad::NORMAL, 1.0f, 3.0f, 6.0f);
        s.setVelocidad(vector2(0, s.impulsoSalto() / s.masa()));
        float hmax = 0.0f;
        for (int i = 0; i < 120; ++i) {
            s.aplicarFuerza(vector2(0, -9.8f * s.masa()));
            s.actualizar(DT);
            if (s.posicion().y > hmax) hmax = s.posicion().y;
        }
        verificar(fabs(hmax - 36.0f / 19.6f) / (36.0f / 19.6f) < 0.03f,
                  "altura del salto dentro del 3% de v0^2/(2g)");

        m.actualizar(0.0f);
        m.actualizar(-1.0f);
        m.actualizar(NAN);
        verificar(casi(m.posicion().y, -9.8f), "dt cero, negativo o NaN no lo mueve");
    }

    printf("Memoria\n");
    {
        Personaje* p = new ProtagonistaContado();
        delete p;
        verificar(vivos == 0, "delete por Personaje* libera al Protagonista (destructor virtual)");
    }

    correctas = total - fallas;
    totales = total;
}

} // namespace

// La llama main_pruebas.cpp
void pruebasProtagonista(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}