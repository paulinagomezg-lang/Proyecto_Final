// Pruebas de GestorLogico usando niveles falsos.
// Los niveles falsos cuentan cuántos existen en memoria, para comprobar
// que nunca hay más de uno y que todos se liberan.

#include "gestorlogico.h"
#include "Nivel.h"
#include "Excepciones_del_Juego.h"
#include <cstdio>
using namespace std;

// Todo lo de esta prueba queda privado a este archivo, para que los
// nombres (verificar, fallas, total...) no choquen con las otras pruebas.
namespace {

static int fallas = 0, total = 0;
static void verificar(bool c, const char* d)
{
    ++total;
    printf("  %s  %s\n", c ? "OK   " : "FALLA", d);
    if (!c) ++fallas;
}


// Nivel falso: se gana o se pierde cuando la prueba lo indica.

static int nivelesVivos = 0;       // cuántos niveles existen ahora
static int maxNivelesVivos = 0;    // el máximo que llegó a haber
static int nivelesCreados = 0;
static bool fabricaDebeFallar = false;
static NivelDificultad ultimaDificultad = NivelDificultad::NORMAL;

class NivelFalso : public Nivel
{
public:
    float tiempo = 0.0f;
    float ultimoDt = 0.0f;
    bool gano = false;
    bool perdio = false;

    NivelFalso()
    {
        ++nivelesVivos;
        ++nivelesCreados;
        if (nivelesVivos > maxNivelesVivos) maxNivelesVivos = nivelesVivos;
    }
    ~NivelFalso() override { --nivelesVivos; }

    void actualizar(float dt) override { tiempo += dt; ultimoDt = dt; }
    bool condicionVictoria() const override { return gano; }
    bool condicionDerrota() const override { return perdio; }
};

static Nivel* fabricaFalsa(int, NivelDificultad dificultad)
{
    ultimaDificultad = dificultad;
    if (fabricaDebeFallar)
        throw Excepciones_del_Juego("archivo del mapa no encontrado");
    return new NivelFalso();
}

static NivelFalso* actual(const gestorlogico& g)
{
    return static_cast<NivelFalso*>(g.nivelactual());
}

void ejecutar(int& correctas, int& totales)
{
    printf("Construccion\n");
    {
        bool lanzo = false;
        try { gestorlogico g(nullptr, 2); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "fabrica nula lanza excepcion");
        lanzo = false;
        try { gestorlogico g(fabricaFalsa, 0); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "cero niveles lanza excepcion");

        gestorlogico g(fabricaFalsa, 2);
        verificar(g.estado() == EstadoJuego::MENU, "empieza en el menu");
        verificar(g.nivelactual() == nullptr, "en el menu no hay nivel en memoria");
    }

    printf("Partida completa: nivel 1 -> nivel 2 -> victoria\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        g.iniciar();
        verificar(g.estado() == EstadoJuego::JUGANDO && g.indicenivelactual() == 0, "iniciar carga el nivel 1");

        g.actualizar(0.016f);
        verificar(actual(g)->tiempo > 0.0f, "actualizar avanza el nivel");

        actual(g)->gano = true;
        g.actualizar(0.016f);
        verificar(g.estado() == EstadoJuego::NIVEL_SUPERADO, "ganar el nivel 1 da NIVEL_SUPERADO");

        g.avanzarNivel();
        verificar(g.indicenivelactual() == 1 && g.estado() == EstadoJuego::JUGANDO, "avanzar carga el nivel 2");
        verificar(nivelesVivos == 1, "el nivel 1 se libero al pasar al 2");

        actual(g)->gano = true;
        g.actualizar(0.016f);
        verificar(g.estado() == EstadoJuego::VICTORIA, "ganar el ultimo nivel da VICTORIA");

        g.avanzarNivel();
        verificar(g.indicenivelactual() == 1, "no se puede avanzar mas alla del ultimo nivel");
    }
    verificar(nivelesVivos == 0, "al destruir el gestor no queda ningun nivel en memoria");

    printf("Vidas y reinicio\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        g.iniciar();
        verificar(g.vidas() == 3, "la partida empieza con 3 vidas");

        actual(g)->perdio = true;
        actual(g)->gano = true;
        g.actualizar(0.016f);
        verificar(g.estado() == EstadoJuego::VIDA_PERDIDA, "si gana y pierde a la vez, cuenta la captura");
        verificar(g.vidas() == 2, "ser capturado quita una vida");

        g.actualizar(0.016f);
        verificar(g.vidas() == 2, "en VIDA_PERDIDA no se siguen quitando vidas");

        g.reiniciarNivel();
        verificar(g.estado() == EstadoJuego::JUGANDO && !actual(g)->perdio, "reiniciar crea el nivel de nuevo");
        verificar(g.vidas() == 2, "reiniciar no cuesta vidas");
        verificar(nivelesVivos == 1, "reiniciar no deja el nivel viejo en memoria");

        actual(g)->perdio = true; g.actualizar(0.016f); g.reiniciarNivel();
        actual(g)->perdio = true; g.actualizar(0.016f);
        verificar(g.vidas() == 0 && g.estado() == EstadoJuego::DERROTA, "con la tercera captura se acaba el juego");

        g.reiniciarNivel();
        verificar(g.estado() == EstadoJuego::DERROTA, "tras perder todo no se puede reiniciar el nivel");

        g.iniciar();
        verificar(g.vidas() == 3 && g.indicenivelactual() == 0, "una partida nueva vuelve a 3 vidas y al nivel 1");
    }
    printf("Las vidas pasan de un nivel al siguiente\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        g.iniciar();
        actual(g)->perdio = true; g.actualizar(0.016f); g.reiniciarNivel();
        actual(g)->gano = true;   g.actualizar(0.016f); g.avanzarNivel();
        verificar(g.indicenivelactual() == 1 && g.vidas() == 2, "Mario llega al nivel 2 con las vidas que le quedaban");
        g.volverALMenu();
        verificar(g.vidas() == 3, "volver al menu reinicia las vidas");
    }

    printf("Pausa\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        g.iniciar();
        g.pausar();
        g.actualizar(0.016f);
        verificar(g.estado() == EstadoJuego::PAUSA && actual(g)->tiempo == 0.0f, "en pausa el nivel no avanza");
        g.reanudar();
        g.actualizar(0.016f);
        verificar(actual(g)->tiempo > 0.0f, "al reanudar el nivel vuelve a avanzar");
    }

    printf("Paso de tiempo\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        g.iniciar();
        g.actualizar(2.0f);
        verificar(actual(g)->ultimoDt == gestorlogico::DT_MAXIMO, "un dt enorme se recorta a DT_MAXIMO");
        const float antes = actual(g)->tiempo;
        g.actualizar(-1.0f);
        g.actualizar(0.0f);
        verificar(actual(g)->tiempo == antes, "dt cero o negativo se ignora");
    }

    printf("Dificultad\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        verificar(g.setdificultad(NivelDificultad::DIFICIL), "en el menu se puede cambiar la dificultad");
        g.iniciar();
        verificar(ultimaDificultad == NivelDificultad::DIFICIL, "el nivel se crea con la dificultad elegida");
        verificar(!g.setdificultad(NivelDificultad::FACIL), "durante el juego no se puede cambiar");
        verificar(g.dificultad() == NivelDificultad::DIFICIL, "la dificultad no cambio");
    }

    printf("Volver al menu\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        g.iniciar();
        g.volverALMenu();
        verificar(g.estado() == EstadoJuego::MENU && g.nivelactual() == nullptr, "volver al menu libera el nivel");
        verificar(nivelesVivos == 0, "no queda ningun nivel en memoria");
    }

    printf("Error al crear un nivel (por ejemplo, falta el mapa)\n");
    {
        gestorlogico g(fabricaFalsa, 2);
        g.iniciar();
        actual(g)->gano = true;
        g.actualizar(0.016f);
        fabricaDebeFallar = true;
        bool lanzo = false;
        try { g.avanzarNivel(); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        fabricaDebeFallar = false;
        verificar(lanzo, "la excepcion llega hasta quien llamo (la interfaz)");
        verificar(g.estado() == EstadoJuego::MENU && g.nivelactual() == nullptr, "el juego queda en el menu, sin punteros invalidos");
        verificar(nivelesVivos == 0, "sin fugas: el nivel anterior se libero");
        g.actualizar(0.016f);
        verificar(true, "actualizar despues del error no rompe nada");
    }

    printf("Memoria\n");
    verificar(maxNivelesVivos == 1, "nunca hubo mas de un nivel en memoria a la vez");
    printf("     niveles creados en todas las pruebas: %d\n", nivelesCreados);
    correctas = total - fallas;
    totales = total;
}

} // namespace

// La llama main_pruebas.cpp
void pruebasGestorLogico(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}