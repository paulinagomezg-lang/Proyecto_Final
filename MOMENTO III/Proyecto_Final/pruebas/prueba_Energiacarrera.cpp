// Pruebas de energiacarrera.

#include "Energiacarrera.h"
#include "Excepciones_del_Juego.h"
#include <cstdio>
#include <cmath>
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
static bool casi(float a, float b) { return fabs(a - b) < 1e-3f; }

// Simula 'segundos' de juego a 60 fotogramas por segundo.
static void simular(energiacarrera& e, float segundos, bool corriendo, float mult = 1.0f)
{
    const float dt = 1.0f / 60.0f;
    const int pasos = static_cast<int>(segundos * 60.0f + 0.5f);
    for (int i = 0; i < pasos; ++i) e.actualizar(dt, corriendo, mult);
}

void ejecutar(int& correctas, int& totales)
{
    printf("Validacion de parametros\n");
    {
        bool lanzo = false;
        try { energiacarrera e(0.0f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "energia maxima 0 lanza excepcion");
        lanzo = false;
        try { energiacarrera e(100, 25, 15, 150); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "umbral mayor que la energia maxima lanza excepcion");
        lanzo = false;
        try { energiacarrera e(100, 25, 15, 30, 1.5f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "factor minimo fuera de [0, 1] lanza excepcion");
    }

    // Valores por defecto: max 100, gasta 25/s, recupera 15/s, umbral 30, factor minimo 0.4
    printf("Correr gasta energia\n");
    {
        energiacarrera e;
        verificar(casi(e.porcentaje(), 1.0f) && casi(e.factorVelocidad(), 1.0f), "empieza llena y a velocidad normal");
        simular(e, 2.0f, true);
        verificar(casi(e.energia(), 50.0f), "2 s corriendo a 25/s: queda 50");
        verificar(casi(e.factorVelocidad(), 1.0f), "por encima del umbral va a velocidad normal");
    }

    printf("Cansancio: se va poniendo lento\n");
    {
        energiacarrera e;
        simular(e, 3.4f, true);   // 100 - 85 = 15, la mitad del umbral
        verificar(casi(e.energia(), 15.0f), "3.4 s corriendo: queda 15");
        verificar(casi(e.factorVelocidad(), 0.7f), "a mitad del umbral el factor es 0.7 (entre 0.4 y 1)");
        // Corre fotograma a fotograma hasta agotarse.
        int pasos = 0;
        while (!e.agotada() && pasos < 1000) { e.actualizar(1.0f / 60.0f, true); ++pasos; }
        verificar(e.energia() == 0.0f && e.agotada(), "al llegar a 0 queda agotado");
        verificar(casi(e.factorVelocidad(), 0.4f), "agotado va al factor minimo (0.4)");
        verificar(!e.puedeCorrer(), "agotado no puede correr");
    }

    printf("Recuperacion\n");
    {
        energiacarrera e;
        while (!e.agotada()) e.actualizar(1.0f / 60.0f, true);   // agotado, energia 0
        simular(e, 1.0f, true);            // sigue presionando correr
        verificar(casi(e.energia(), 15.0f), "agotado, aunque presione correr, recupera en vez de gastar");
        verificar(e.agotada(), "con 15 todavia no supera el umbral: sigue sin poder correr");
        simular(e, 1.0f, false);           // 30
        verificar(!e.agotada() && e.puedeCorrer(), "al llegar al umbral puede volver a correr");
        simular(e, 10.0f, false);
        verificar(casi(e.energia(), 100.0f), "la energia no pasa del maximo");
    }

    printf("Interaccion con el poder pesado\n");
    {
        energiacarrera normal, pesado;
        simular(normal, 1.0f, true, 1.0f);
        simular(pesado, 1.0f, true, 3.0f);   // masa triple
        verificar(casi(100.0f - pesado.energia(), 3.0f * (100.0f - normal.energia())),
                  "con masa triple gasta el triple de energia");
    }

    printf("Casos especiales\n");
    {
        energiacarrera e;
        e.actualizar(0.0f, true);
        e.actualizar(-1.0f, true);
        e.actualizar(NAN, true);
        verificar(casi(e.energia(), 100.0f), "dt cero, negativo o NaN no cambia nada");
        simular(e, 5.0f, true);
        e.reiniciar();
        verificar(casi(e.energia(), 100.0f) && !e.agotada() && casi(e.factorVelocidad(), 1.0f),
                  "reiniciar deja la energia llena");
    }

    printf("Dificultad\n");
    {
        auto segundosHastaAgotarse = [](energiacarrera e) {
            int n = 0;
            while (!e.agotada() && n < 100000) { e.actualizar(1.0f / 60.0f, true); ++n; }
            return static_cast<float>(n) / 60.0f;
        };
        const float facil   = segundosHastaAgotarse(energiacarrera::segunDificultad(NivelDificultad::FACIL));
        const float normal  = segundosHastaAgotarse(energiacarrera::segunDificultad(NivelDificultad::NORMAL));
        const float dificil = segundosHastaAgotarse(energiacarrera::segunDificultad(NivelDificultad::DIFICIL));
        printf("     corre %.1f s en facil, %.1f s en normal y %.1f s en dificil\n", facil, normal, dificil);
        verificar(facil > normal && normal > dificil, "mas dificil = se agota antes");

        energiacarrera ef = energiacarrera::segunDificultad(NivelDificultad::FACIL);
        energiacarrera ed = energiacarrera::segunDificultad(NivelDificultad::DIFICIL);
        while (!ef.agotada()) ef.actualizar(1.0f / 60.0f, true);
        while (!ed.agotada()) ed.actualizar(1.0f / 60.0f, true);
        verificar(ef.factorVelocidad() > ed.factorVelocidad(), "agotado en dificil queda mas lento que en facil");
    }
    correctas = total - fallas;
    totales = total;
}

} // namespace

// La llama main_pruebas.cpp
void pruebasEnergiaCarrera(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}