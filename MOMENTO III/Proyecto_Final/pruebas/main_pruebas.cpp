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

struct Suite
{
    const char* nombre;
    void (*ejecutar)(int&, int&);
};

int main()
{
    const Suite suites[] = {
                             {"vector2",        pruebasVector2},
                             {"energiacarrera", pruebasEnergiaCarrera},
                             {"gestorlogico",   pruebasGestorLogico},
                             {"Nivel",          pruebasNivel},
                             };

    int correctasTotal = 0, totalGeneral = 0;
    printf("========== PRUEBAS DE LA LOGICA DEL JUEGO ==========\n");

    for (const Suite& s : suites) {
        printf("\n---------- %s ----------\n", s.nombre);
        int correctas = 0, totales = 0;
        s.ejecutar(correctas, totales);
        printf("  => %s: %d de %d correctas\n", s.nombre, correctas, totales);
        correctasTotal += correctas;
        totalGeneral += totales;
    }

    printf("\n====================================================\n");
    printf("TOTAL: %d de %d pruebas correctas\n", correctasTotal, totalGeneral);
    printf("====================================================\n");
    return correctasTotal == totalGeneral ? 0 : 1;
}