// Pruebas de vector2. Se compilan y ejecutan aparte del juego.
// Cada prueba imprime OK o FALLA, y el programa termina con código 1
// si alguna falla.

#include "vector2.h"
#include <cstdio>

static int fallas = 0;
static int total = 0;

static void verificar(bool condicion, const char* descripcion) {
    ++total;
    if (condicion) {
        std::printf("  OK     %s\n", descripcion);
    } else {
        std::printf("  FALLA  %s\n", descripcion);
        ++fallas;
    }
}

static bool casi(float a, float b, float tol = 1e-4f) { return std::fabs(a - b) <= tol; }

int main() {
    const float PI = 3.14159265f;

    std::printf("Construccion\n");
    verificar(vector2() == vector2(0, 0), "el constructor por defecto crea (0, 0)");
    verificar(vector2(3, 4).x == 3 && vector2(3, 4).y == 4, "el constructor guarda x, y");
    verificar(vector2::desdeAngulo(0, 5) == vector2(5, 0), "desdeAngulo(0, 5) = (5, 0)");
    verificar(vector2::desdeAngulo(PI / 2, 2) == vector2(0, 2), "desdeAngulo(pi/2, 2) = (0, 2)");

    std::printf("Operaciones\n");
    verificar(vector2(1, 2) + vector2(3, 4) == vector2(4, 6), "suma");
    verificar(vector2(5, 5) - vector2(2, 3) == vector2(3, 2), "resta");
    verificar(-vector2(1, -2) == vector2(-1, 2), "negacion");
    verificar(vector2(1, 2) * 3.0f == vector2(3, 6), "vector por escalar");
    verificar(3.0f * vector2(1, 2) == vector2(3, 6), "escalar por vector");
    verificar(vector2(6, 4) / 2.0f == vector2(3, 2), "division por escalar");
    verificar(vector2(6, 4) / 0.0f == vector2(0, 0), "dividir por cero devuelve (0, 0) sin infinitos");

    vector2 v(1, 1);
    v += vector2(2, 3);   verificar(v == vector2(3, 4), "+=");
    v -= vector2(1, 1);   verificar(v == vector2(2, 3), "-=");
    v *= 2.0f;            verificar(v == vector2(4, 6), "*=");
    v /= 2.0f;            verificar(v == vector2(2, 3), "/=");

    std::printf("Productos\n");
    verificar(casi(vector2(1, 2).punto(vector2(3, 4)), 11), "producto punto (1,2)·(3,4) = 11");
    verificar(casi(vector2(1, 0).punto(vector2(0, 1)), 0), "vectores perpendiculares: punto = 0");
    verificar(vector2(1, 0).cruz(vector2(0, 1)) > 0, "cruz: (0,1) esta a la izquierda de (1,0)");
    verificar(vector2(1, 0).cruz(vector2(0, -1)) < 0, "cruz: (0,-1) esta a la derecha de (1,0)");

    std::printf("Longitudes y distancias\n");
    verificar(casi(vector2(3, 4).normaCuadrada(), 25), "normaCuadrada (3,4) = 25");
    verificar(casi(vector2(3, 4).norma(), 5), "norma (3,4) = 5");
    verificar(casi(vector2(1, 1).distanciaCuadrada(vector2(4, 5)), 25), "distanciaCuadrada = 25");
    verificar(casi(vector2(1, 1).distancia(vector2(4, 5)), 5), "distancia = 5");
    verificar(casi(vector2(3, 4).normalizado().norma(), 1), "normalizado tiene longitud 1");
    verificar(vector2(3, 4).normalizado() == vector2(0.6f, 0.8f), "normalizado conserva la direccion");
    verificar(vector2().normalizado() == vector2(0, 0), "normalizar el vector nulo devuelve (0, 0)");
    verificar(vector2(3, 4).limitado(10) == vector2(3, 4), "limitado no cambia un vector corto");
    verificar(casi(vector2(30, 40).limitado(5).norma(), 5), "limitado recorta un vector largo");

    std::printf("Angulos\n");
    verificar(casi(vector2(1, 0).angulo(), 0), "angulo de (1,0) = 0");
    verificar(casi(vector2(0, 1).angulo(), PI / 2), "angulo de (0,1) = pi/2");
    verificar(vector2(1, 0).rotado(PI / 2) == vector2(0, 1), "rotar (1,0) 90 grados da (0,1)");
    verificar(vector2(2, 0).rotado(PI) == vector2(-2, 0), "rotar (2,0) 180 grados da (-2,0)");
    verificar(vector2(1, 0).rotado(std::cos(PI / 2), std::sin(PI / 2)) == vector2(0, 1),
              "rotar con cos y sin precalculados da el mismo resultado");
    verificar(vector2(30, 40).limitado(5) == vector2(3, 4), "limitado conserva la direccion");

    std::printf("Comparacion\n");
    verificar(vector2(0.1f + 0.2f, 0) == vector2(0.3f, 0), "== tolera errores de redondeo");
    verificar(vector2(1, 2) != vector2(1, 2.1f), "!= detecta valores distintos");
    verificar(vector2(1, 2).CasiIgual(vector2(1.000001f, 2)), "CasiIgual acepta diferencias minimas");
    verificar(!vector2(1, 2).CasiIgual(vector2(1, 3)), "CasiIgual compara tambien la y");

    std::printf("Uso con vectores constantes\n");
    {
        // Las demas clases recibiran vectores const; todos los metodos
        // de consulta deben poder usarse sobre ellos.
        const vector2 a(1, 2), b(3, 4);
        verificar(casi(a.punto(b), 11), "punto funciona con vectores const");
        verificar(casi(a.cruz(b), -2), "cruz funciona con vectores const");
        verificar(casi(b.norma(), 5), "norma funciona con vectores const");
    }
    {
        vector2 v2(3, 4);
        v2 *= 2.0f;
        verificar(casi(v2.y, 8), "*= multiplica tambien la y");
    }

    std::printf("Uso real: un paso de fisica (Euler semi-implicito)\n");
    {
        // Salto: velocidad inicial hacia arriba y gravedad hacia abajo.
        vector2 pos(0, 0), vel(2, 10);
        const vector2 g(0, -9.8f);
        const float dt = 0.016f;
        for (int i = 0; i < 100; ++i) {
            vel += g * dt;       // primero la velocidad
            pos += vel * dt;     // luego la posicion
        }
        verificar(casi(pos.x, 3.2f, 1e-3f), "x avanza con velocidad constante (2 * 1.6 s = 3.2)");
        verificar(vel.y < 0, "despues de 1.6 s la velocidad vertical ya es hacia abajo");
    }

    std::printf("\n%d de %d pruebas correctas\n", total - fallas, total);
    return fallas == 0 ? 0 : 1;
}