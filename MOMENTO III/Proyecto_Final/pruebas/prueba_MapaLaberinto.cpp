// Pruebas de MapaLaberinto.
// No tiene main(): main_pruebas.cpp llama a pruebasMapaLaberinto().

#include "MapaLaberinto.h"
#include "Excepciones_del_Mapa.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
using namespace std;

namespace {

int fallas = 0, total = 0;
void verificar(bool c, const char* d)
{
    ++total;
    printf("  %s  %s\n", c ? "OK   " : "FALLA", d);
    if (!c) ++fallas;
}
bool casi(float a, float b, float tol = 1e-4f) { return fabs(a - b) < tol; }
bool casi(vector2 a, vector2 b) { return casi(a.x, b.x) && casi(a.y, b.y); }

// Devuelve true si cargar ese texto lanza Excepciones_del_Mapa con esa línea
// (linea = 0: no importa la línea).
bool lanzaErrorDeMapa(const char* texto, int lineaEsperada = 0)
{
    try {
        MapaLaberinto m(texto, "prueba.txt");
    } catch (const Excepciones_del_Mapa& e) {
        return lineaEsperada == 0 || e.linea() == lineaEsperada;
    }
    return false;
}

// Mapa pequeño para las pruebas (5 columnas x 4 filas):
//   fila 0: #####
//   fila 1: #M.G#
//   fila 2: #.#B#
//   fila 3: ##S##
const char* MAPA_PEQUENO =
    "#####\n"
    "#M.G#\n"
    "#.#B#\n"
    "##S##\n";

void ejecutar(int& correctas, int& totales)
{
    printf("Carga\n");
    {
        MapaLaberinto m(MAPA_PEQUENO, "pequeno.txt");
        verificar(m.ancho() == 5 && m.alto() == 4, "lee el tamano: 5 columnas x 4 filas");
        verificar(m.celdaInicio() == MapaLaberinto::Celda{1, 1}, "encuentra el inicio de Mario (M)");
        verificar(m.celdaSalida() == MapaLaberinto::Celda{2, 3}, "encuentra la salida (S)");
        verificar(m.guardias().size() == 1 && m.guardias()[0] == MapaLaberinto::Celda{3, 1},
                  "encuentra el Minion guardia (G)");
        verificar(m.bloquesMoviles().size() == 1 && m.bloquesMoviles()[0] == MapaLaberinto::Celda{3, 2},
                  "encuentra el bloque movil (B)");
        verificar(m.esPared(0, 0) && m.esPared(2, 2), "las # son pared");
        verificar(!m.esPared(1, 1) && !m.esPared(2, 1) && !m.esPared(3, 1),
                  "M, . y G son pasillo (se puede caminar)");
        verificar(m.esPared(-1, 0) && m.esPared(5, 1) && m.esPared(1, -1) && m.esPared(1, 4),
                  "fuera del mapa cuenta como pared");

        MapaLaberinto windows("###\r\n#MS\r\n###\r\n\r\n\r\n", "windows.txt");
        verificar(windows.ancho() == 3 && windows.alto() == 3,
                  "acepta saltos de linea de Windows (\\r\\n) y lineas vacias al final");
        MapaLaberinto sinSalto("M.S", "una.txt");
        verificar(sinSalto.alto() == 1 && sinSalto.ancho() == 3, "acepta un archivo sin salto de linea final");
        MapaLaberinto conEspacios("M S", "espacios.txt");
        verificar(!conEspacios.esPared(1, 0), "un espacio tambien es pasillo");
    }

    printf("Errores al cargar (Excepciones_del_Mapa con la linea)\n");
    {
        verificar(lanzaErrorDeMapa(""), "mapa vacio lanza excepcion");
        verificar(lanzaErrorDeMapa("\n\n"), "mapa con solo lineas vacias lanza excepcion");
        verificar(lanzaErrorDeMapa("#####\n#M.S#\n####\n", 3), "fila de distinto ancho: error en la linea 3");
        verificar(lanzaErrorDeMapa("#####\n#M.S#\n#.X.#\n", 3), "caracter invalido: error en la linea 3");
        verificar(lanzaErrorDeMapa("###\n#.S\n###\n"), "sin M lanza excepcion");
        verificar(lanzaErrorDeMapa("###\n#M.\n###\n"), "sin S lanza excepcion");
        verificar(lanzaErrorDeMapa("#M#\n#M#\n#S#\n", 2), "dos M: error en la linea 2");
        verificar(lanzaErrorDeMapa("#S#\n#M#\n#S#\n", 3), "dos S: error en la linea 3");
        verificar(lanzaErrorDeMapa("#M#\n\n#S#\n", 2), "fila vacia en medio: error en la linea 2");

        bool lanzo = false;
        try {
            MapaLaberinto m("#M#\n#S#\n", "x.txt", 0.0f);
        } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "tamCelda = 0 lanza Excepciones_de_Parametros_Fisicos");

        string fila(MapaLaberinto::MAXIMO_LADO + 1, '.');
        fila[0] = 'M';
        fila[1] = 'S';
        verificar(lanzaErrorDeMapa(fila.c_str()), "un mapa mas grande que el maximo lanza excepcion");

        string mensaje;
        try {
            MapaLaberinto m("#M#\n#?#\n#S#\n", "nivel2.txt");
        } catch (const Excepciones_del_Juego& e) { mensaje = e.what(); }
        verificar(mensaje.find("nivel2.txt") != string::npos && mensaje.find("linea 2") != string::npos,
                  "el mensaje dice el archivo y la linea (se atrapa como Excepciones_del_Juego)");

        lanzo = false;
        try {
            MapaLaberinto m = MapaLaberinto::desdeArchivo("no_existe_este_mapa.txt");
        } catch (const Excepciones_de_Recursos&) { lanzo = true; }
        verificar(lanzo, "archivo que no existe lanza Excepciones_de_Recursos");
    }

    printf("Coordenadas (y hacia arriba, celdas de tamCelda metros)\n");
    {
        MapaLaberinto m(MAPA_PEQUENO, "pequeno.txt", 2.0f);   // celdas de 2 m
        // Fila 0 (arriba en el archivo) es la de mayor y en el mundo.
        verificar(casi(m.centroDe(0, 3), vector2(1.0f, 1.0f)), "la fila de abajo del archivo esta en y pequeno");
        verificar(casi(m.centroDe(0, 0), vector2(1.0f, 7.0f)), "la fila de arriba del archivo esta en y grande");
        verificar(casi(m.inicioMario(), vector2(3.0f, 5.0f)), "inicioMario() da el centro de la celda M en metros");
        verificar(m.celdaDe(vector2(3.0f, 5.0f)) == MapaLaberinto::Celda{1, 1}, "celdaDe() es la inversa de centroDe()");
        verificar(m.celdaDe(vector2(3.9f, 4.1f)) == MapaLaberinto::Celda{1, 1}, "un punto cerca del borde sigue en su celda");
        verificar(m.celdaDe(vector2(-0.1f, 1.0f)).columna == -1, "x negativo queda fuera del mapa (columna -1)");
        verificar(m.esParedEn(vector2(-5.0f, 1.0f)) && m.esParedEn(vector2(1e9f, 1.0f)),
                  "posiciones fuera o enormes cuentan como pared (sin desbordarse)");
        verificar(m.esParedEn(vector2(NAN, 0.0f)), "una posicion NaN cuenta como pared");
        verificar(!m.esParedEn(m.salida()), "la salida no es pared");

        bool idaYVuelta = true;
        for (int f = 0; f < m.alto(); ++f)
            for (int c = 0; c < m.ancho(); ++c)
                if (m.celdaDe(m.centroDe(c, f)) != MapaLaberinto::Celda{c, f})
                    idaYVuelta = false;
        verificar(idaYVuelta, "ida y vuelta celda -> mundo -> celda en todas las celdas");
    }

    printf("Linea de vision (las paredes tapan)\n");
    {
        // 7 x 5, con una pared en el centro:
        //   #######
        //   #M...S#
        //   #..#..#
        //   #.....#
        //   #######
        MapaLaberinto m("#######\n#M...S#\n#..#..#\n#.....#\n#######\n", "vision.txt");
        verificar(m.hayLineaDeVision(m.centroDe(1, 1), m.centroDe(5, 1)), "en un pasillo recto se ve");
        verificar(!m.hayLineaDeVision(m.centroDe(1, 2), m.centroDe(5, 2)), "la pared del medio tapa la vision");
        verificar(m.hayLineaDeVision(m.centroDe(1, 3), m.centroDe(5, 3)), "por debajo de la pared si se ve");
        verificar(m.hayLineaDeVision(m.centroDe(1, 1), m.centroDe(1, 3)), "en vertical por un pasillo se ve");
        verificar(!m.hayLineaDeVision(m.centroDe(2, 1), m.centroDe(4, 3)), "en diagonal a traves de la pared no se ve");
        verificar(m.hayLineaDeVision(m.centroDe(2, 2), m.centroDe(2, 2)), "un punto se ve a si mismo");
        verificar(!m.hayLineaDeVision(m.centroDe(1, 1), m.centroDe(0, 1)), "no se ve hacia dentro de una pared");
        verificar(m.hayLineaDeVision(m.centroDe(5, 3), m.centroDe(1, 1)) ==
                      m.hayLineaDeVision(m.centroDe(1, 1), m.centroDe(5, 3)),
                  "ver de A a B es lo mismo que de B a A");
        verificar(!m.hayLineaDeVision(m.centroDe(1, 2), m.centroDe(5, 2)) &&
                      !m.hayLineaDeVision(m.centroDe(5, 2), m.centroDe(1, 2)),
                  "la pared tapa en los dos sentidos");
    }

    printf("Camino mas corto (BFS)\n");
    {
        //   #######
        //   #M#...#     fila 1
        //   #.#.#.#     fila 2
        //   #...#S#     fila 3
        //   #######
        MapaLaberinto m("#######\n#M#...#\n#.#.#.#\n#...#S#\n#######\n", "camino.txt");
        typedef MapaLaberinto::Celda C;
        C paso{0, 0};
        verificar(m.siguientePaso(C{1, 1}, C{5, 3}, paso) && paso == C{1, 2},
                  "desde M el primer paso es bajar (la pared no deja ir a la derecha)");

        // Seguir el camino paso a paso debe llegar a S en 10 pasos (el mas corto).
        C actual{1, 1};
        int pasos = 0;
        bool siempreVecina = true, nuncaPared = true;
        while (actual != C{5, 3} && pasos < 50) {
            C sig{0, 0};
            if (!m.siguientePaso(actual, C{5, 3}, sig)) break;
            if (abs(sig.columna - actual.columna) + abs(sig.fila - actual.fila) != 1) siempreVecina = false;
            if (m.esPared(sig)) nuncaPared = false;
            actual = sig;
            ++pasos;
        }
        verificar(actual == C{5, 3}, "siguiendo los pasos se llega a la salida");
        verificar(pasos == 10, "por el camino mas corto (10 pasos)");
        verificar(siempreVecina && nuncaPared, "cada paso es a una celda vecina y nunca a una pared");

        verificar(m.siguientePaso(C{3, 1}, C{3, 1}, paso) && paso == C{3, 1}, "si ya esta en el destino, el paso es la misma celda");
        verificar(!m.siguientePaso(C{1, 1}, C{0, 0}, paso), "hacia una pared no hay camino");
        verificar(!m.siguientePaso(C{-3, 1}, C{1, 1}, paso), "desde fuera del mapa no hay camino");

        MapaLaberinto cerrado("#####\n#M#S#\n#####\n", "cerrado.txt");
        verificar(!cerrado.siguientePaso(cerrado.celdaInicio(), cerrado.celdaSalida(), paso),
                  "si una pared separa los dos lados, no hay camino (devuelve false)");

        // Muchas busquedas seguidas (como en el juego) siguen dando lo mismo.
        bool igual = true;
        for (int i = 0; i < 1000; ++i)
            if (!m.siguientePaso(C{1, 1}, C{5, 3}, paso) || paso != C{1, 2}) igual = false;
        verificar(igual, "1000 busquedas seguidas dan el mismo resultado (la marca de visitados funciona)");

        verificar(m.celdasLibres().size() == 11, "celdasLibres() tiene los 11 pasillos del mapa");
    }

    printf("Mapa del juego (mapas/laberinto1.txt)\n");
    {
        // Qt Creator ejecuta las pruebas desde la carpeta de compilación,
        // así que se busca el archivo en varias carpetas de arriba.
        const char* rutas[] = {"mapas/laberinto1.txt", "../mapas/laberinto1.txt",
                               "../../mapas/laberinto1.txt", "../../../mapas/laberinto1.txt"};
        bool cargo = false;
        for (const char* ruta : rutas) {
            if (cargo) break;
            try {
                MapaLaberinto m = MapaLaberinto::desdeArchivo(ruta);
                cargo = true;
                verificar(m.ancho() == 21 && m.alto() == 13, "el mapa del nivel mide 21 x 13");
                verificar(m.guardias().size() == 3 && m.bloquesMoviles().size() == 2,
                          "tiene 3 Minions guardia y 2 bloques moviles");
                verificar(!m.esParedEn(m.inicioMario()) && !m.esParedEn(m.salida()),
                          "Mario y la salida estan en pasillos");
            } catch (const Excepciones_de_Recursos&) {
                // no está en esta carpeta: se prueba la siguiente
            }
        }
        if (!cargo)
            printf("  (no se encontro mapas/laberinto1.txt: se omiten estas 3 pruebas)\n");
    }

    correctas = total - fallas;
    totales = total;
}

} // namespace

void pruebasMapaLaberinto(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}