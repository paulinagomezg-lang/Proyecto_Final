// Ejecuta todas las pruebas de la lógica del juego y muestra el total.
// Para agregar una prueba nueva:
//   1. En su archivo, cambiar main() por una función pruebasX(int&, int&).
//   2. Declararla aquí abajo y agregarla a la lista 'suites'.
//   3. Agregar el archivo (y el .cpp de la clase) al SOURCES de pruebas.pro.

#include <cstdio>
using namespace std;

void pruebasVector2(int& correctas, int& totales);
void pruebasEnergiaCarrera(int& correctas, int& totales);
void pruebasGestorLogico(int& correctas, int& totales);
void pruebasNivel(int& correctas, int& totales);
void pruebasPersonajeEnemigo(int& correctas, int& totales);
void pruebasProtagonista(int& correctas, int& totales);
void pruebasMapaLaberinto(int& correctas, int& totales);
void pruebasMinionGuardia(int& correctas, int& totales);
void pruebasBloqueMovil(int& correctas, int& totales);
void pruebasNivelLaberinto(int& correctas, int& totales);

struct Suite
{
    const char* nombre;
    void (*ejecutar)(int&, int&);
};

// Resultado de cada suite, para el resumen del final.
struct Resultado
{
    int correctas;
    int totales;
};

int main()
{
    const Suite suites[] = {
                             {"vector2",             pruebasVector2},
                             {"energiacarrera",      pruebasEnergiaCarrera},
                             {"gestorlogico",        pruebasGestorLogico},
                             {"Personaje y Enemigo", pruebasPersonajeEnemigo},
                             {"Protagonista",        pruebasProtagonista},
                             {"Nivel",               pruebasNivel},
                             {"MapaLaberinto",       pruebasMapaLaberinto},
                             {"MinionGuardia",       pruebasMinionGuardia},
                             {"BloqueMovil",         pruebasBloqueMovil},
                             {"NivelLaberinto",      pruebasNivelLaberinto},
                             };
    // Cantidad de suites, calculada por el compilador (no hay que cambiarla
    // al agregar una prueba nueva).
    const int numSuites = static_cast<int>(sizeof(suites) / sizeof(suites[0]));
    Resultado resultados[sizeof(suites) / sizeof(suites[0])];   // arreglo fijo, sin memoria dinámica

    int correctasTotal = 0, totalGeneral = 0;
    printf("========== PRUEBAS DE LA LOGICA DEL JUEGO ==========\n");

    for (int i = 0; i < numSuites; ++i) {
        printf("\n---------- %s ----------\n", suites[i].nombre);
        int correctas = 0, totales = 0;
        suites[i].ejecutar(correctas, totales);
        printf("  => %s: %d de %d correctas\n", suites[i].nombre, correctas, totales);
        resultados[i] = {correctas, totales};
        correctasTotal += correctas;
        totalGeneral += totales;
    }

    // Resumen: cuánto aporta cada suite al total de pruebas.
    printf("\n==================== RESUMEN =======================\n");
    printf("%-22s %10s %10s %9s\n", "Suite", "Correctas", "Aporte", "Estado");
    printf("----------------------------------------------------\n");
    for (int i = 0; i < numSuites; ++i) {
        const Resultado& r = resultados[i];
        // Porcentaje del total de pruebas que aporta esta suite.
        const double aporte = totalGeneral > 0 ? 100.0 * r.totales / totalGeneral : 0.0;
        printf("%-22s %4d / %-4d %8.1f %% %9s\n",
               suites[i].nombre, r.correctas, r.totales, aporte,
               r.correctas == r.totales ? "OK" : "FALLA");
    }
    printf("----------------------------------------------------\n");
    printf("%-22s %4d / %-4d %8.1f %%\n", "TOTAL", correctasTotal, totalGeneral, 100.0);

    printf("\n====================================================\n");
    printf("TOTAL: %d de %d pruebas correctas\n", correctasTotal, totalGeneral);
    printf("====================================================\n");
    return correctasTotal == totalGeneral ? 0 : 1;
}