// Pruebas de NivelLaberinto (nivel 2): las 3 físicas, el agente dentro del
// nivel, el evento por tiempo, ganar/perder y la memoria.
// No tiene main(): main_pruebas.cpp llama a pruebasNivelLaberinto().

#include "NivelLaberinto.h"
#include "protagonista.h"
#include "MinionGuardia.h"
#include "BloqueMovil.h"
#include "gestorlogico.h"
#include "Excepciones_del_Mapa.h"
#include <cstdio>
#include <cmath>
using namespace std;

namespace {

int fallas = 0, total = 0;
void verificar(bool c, const char* d)
{
    ++total;
    printf("  %s  %s\n", c ? "OK   " : "FALLA", d);
    if (!c) ++fallas;
}
bool casi(float a, float b, float tol) { return fabs(a - b) < tol; }

const float DT = 1.0f / 60.0f;
typedef NivelLaberinto NL;

void simular(NivelLaberinto& n, float segundos)
{
    const int pasos = static_cast<int>(segundos * 60.0f + 0.5f);
    for (int i = 0; i < pasos; ++i) n.actualizar(DT);
}

// Pasillo largo sin Minions: 20 celdas libres.
const char* PASILLO =
    "######################\n"
    "#M..................S#\n"
    "######################\n";

// Sala de 7x3 con un bloque en el centro (se mueve en horizontal).
const char* SALA_BLOQUE =
    "#########\n"
    "#.......#\n"
    "#M..B..S#\n"
    "#.......#\n"
    "#########\n";

Nivel* fabricaPasillo(int, NivelDificultad d) { return new NivelLaberinto(PASILLO, "pasillo.txt", d); }

void ejecutar(int& correctas, int& totales)
{
    printf("Construccion\n");
    {
        NivelLaberinto n("#########\n#M..G..S#\n#..B....#\n#########\n", "c.txt", NivelDificultad::NORMAL);
        verificar(n.personajes().size() == 3, "crea a Mario, 1 Minion y 1 bloque (3 personajes)");
        verificar(n.guardias().size() == 1 && n.bloques().size() == 1, "guardias() y bloques() los listan por tipo");
        verificar(n.protagonista() != nullptr &&
                      n.mapa().celdaDe(n.protagonista()->posicion()) == n.mapa().celdaInicio(),
                  "Mario empieza en la M");
        verificar(n.protagonista()->mirada() == Mirada::ABAJO, "en el laberinto Mario aparece de frente (4 direcciones)");
        verificar(n.personajes().capacity() == 3, "la lista de personajes se reservo justo (sin memoria de sobra)");
        verificar(!n.lucesApagadas(), "empieza con las luces encendidas");
    }

    printf("Fisica 1: fuerza de control y rozamiento  F = b(v_d - v)\n");
    {
        NivelLaberinto n(PASILLO, "pasillo.txt", NivelDificultad::NORMAL);
        Protagonista& m = *n.protagonista();
        m.setEntrada(vector2(1, 0), false);
        const float tau = 1.0f / NL::COEF_CONTROL;          // m/b con m = 1 kg
        simular(n, tau);
        const float esperado = NL::VEL_CAMINAR * (1.0f - exp(-1.0f));   // 63 %
        verificar(casi(m.velocidad().x, esperado, 0.05f * NL::VEL_CAMINAR),
                  "en t = m/b la velocidad llega al 63 % (v = v_d(1 - e^-1))");
        simular(n, 1.0f);
        verificar(casi(m.velocidad().x, NL::VEL_CAMINAR, 0.01f), "despues de 1 s camina a 2.5 m/s");
        verificar(casi(m.velocidad().y, 0.0f, 1e-4f), "no se desvia en y");

        m.setEntrada(vector2(1, 0), true);
        simular(n, 0.6f);
        verificar(m.velocidad().x > 3.8f && m.estaCorriendo(), "corriendo llega a casi 4 m/s");

        m.setEntrada(vector2(), false);
        simular(n, 1.0f);
        verificar(fabs(m.velocidad().x) < 0.01f, "al soltar las teclas el rozamiento lo frena");
    }

    printf("Fisica 3a: choque con las paredes\n");
    {
        NivelLaberinto n(PASILLO, "pasillo.txt", NivelDificultad::NORMAL);
        Protagonista& m = *n.protagonista();
        m.setEntrada(vector2(-1, 0), false);              // contra la pared de la izquierda
        float xMin = 100.0f;
        for (int i = 0; i < 120; ++i) {
            n.actualizar(DT);
            if (m.posicion().x < xMin) xMin = m.posicion().x;
        }
        verificar(xMin >= 1.0f + NL::RADIO_MARIO - 1e-3f, "no atraviesa la pared (se queda a su radio de distancia)");
        verificar(fabs(m.velocidad().x) < 1e-3f, "la velocidad contra la pared se anula");

        m.setEntrada(vector2(1, 1), false);               // diagonal contra el techo del pasillo
        const float x0 = m.posicion().x;
        float yMax = 0.0f;
        for (int i = 0; i < 60; ++i) {
            n.actualizar(DT);
            if (m.posicion().y > yMax) yMax = m.posicion().y;
        }
        verificar(m.posicion().x > x0 + 1.0f, "en diagonal contra una pared se desliza a lo largo de ella");
        verificar(yMax <= 2.0f - NL::RADIO_MARIO + 1e-3f, "y no se mete en el techo");
    }

    printf("Fisica 2: bloque movil (resorte)  F = -k x,  T = 2*pi*sqrt(m/k)\n");
    {
        NivelLaberinto n(SALA_BLOQUE, "sala.txt", NivelDificultad::NORMAL);
        const BloqueMovil& b = *n.bloques()[0];
        verificar(b.eje() == vector2(1.0f, 0.0f), "en la sala el bloque se mueve en horizontal (donde hay mas espacio)");
        verificar(casi(b.amplitud(), 3.0f, 1e-4f), "su amplitud es el espacio libre a cada lado (3 m)");

        // Se aleja a Mario del bloque para medir el periodo sin choques.
        n.protagonista()->setPosicion(n.mapa().centroDe(1, 1));
        const float T = 6.28318530718f * sqrt(NL::MASA_BLOQUE / NL::CONSTANTE_RESORTE);
        float anterior = b.desplazamiento();
        float primerCruce = -1.0f, ultimoCruce = -1.0f;
        int cruces = 0;
        float maximo = 0.0f, tiempo = 0.0f;
        for (int i = 0; i < 60 * 12; ++i) {
            n.actualizar(DT);
            tiempo += DT;
            const float s = b.desplazamiento();
            if ((anterior > 0.0f) != (s > 0.0f)) {        // cruzo el centro
                if (primerCruce < 0.0f) primerCruce = tiempo;
                ultimoCruce = tiempo;
                ++cruces;
            }
            if (fabs(s) > maximo) maximo = fabs(s);
            anterior = s;
        }
        // Entre cruces del centro pasa medio periodo.
        const float periodoMedido = 2.0f * (ultimoCruce - primerCruce) / static_cast<float>(cruces - 1);
        verificar(casi(periodoMedido, T, 0.02f * T), "el periodo medido es 2*pi*sqrt(m/k) (unos 3 s)");
        verificar(maximo <= b.amplitud() * 1.02f && maximo >= b.amplitud() * 0.98f,
                  "la amplitud se conserva (Euler semi-implicito no gana ni pierde energia)");
        verificar(fabs(b.posicion().y - n.mapa().centroDe(4, 2).y) < 1e-4f, "el bloque no se sale de su riel");
    }

    printf("Fisica 3b: choque de Mario con el bloque (restitucion)\n");
    {
        NivelLaberinto n(SALA_BLOQUE, "sala.txt", NivelDificultad::NORMAL);
        Protagonista& m = *n.protagonista();
        const BloqueMovil& b = *n.bloques()[0];
        // Mario quieto en el camino del bloque, en el centro de la sala.
        m.setPosicion(n.mapa().centroDe(4, 2));
        // (el bloque empieza en +3 m y viene hacia el centro)
        float maxVel = 0.0f, peorSolape = 0.0f;
        for (int i = 0; i < 60 * 2; ++i) {
            n.actualizar(DT);
            const float solape = NL::RADIO_MARIO + NL::RADIO_BLOQUE - (m.posicion() - b.posicion()).norma();
            if (solape > peorSolape) peorSolape = solape;
            if (m.velocidad().norma() > maxVel) maxVel = m.velocidad().norma();
        }
        verificar(maxVel > 1.0f, "el bloque golpea a Mario y lo empuja");
        verificar(peorSolape < 0.05f, "Mario y el bloque no se atraviesan");
        verificar(!n.mapa().esParedEn(m.posicion()), "aunque lo empuje, Mario no termina dentro de una pared");
        verificar(fabs(b.posicion().y - n.mapa().centroDe(4, 2).y) < 1e-4f && fabs(b.desplazamiento()) <= b.amplitud() + 1e-4f,
                  "despues del choque el bloque sigue en su riel");
    }

    printf("Agente dentro del nivel: lo ve, lo persigue y lo atrapa\n");
    {
        // El Minion mira a la derecha y Mario esta 3 m al frente, quieto.
        NivelLaberinto n("############\n#G...M....S#\n############\n", "caza.txt", NivelDificultad::NORMAL);
        const MinionGuardia& g = *n.guardias()[0];
        bool sospecho = false, persiguio = false;
        int f = 0;
        while (!n.condicionDerrota() && f < 60 * 6) {
            n.actualizar(DT);
            if (g.estado() == EstadoAgente::SOSPECHAR) sospecho = true;
            if (g.estado() == EstadoAgente::PERSEGUIR) persiguio = true;
            ++f;
        }
        verificar(sospecho && persiguio, "pasa por SOSPECHAR y luego PERSEGUIR");
        verificar(n.condicionDerrota() && n.protagonista()->estaCapturado(), "lo atrapa: condicionDerrota() = true");
        verificar(!n.condicionVictoria(), "atrapado no puede ganar");
    }
    {
        // Mario huye corriendo y llega a la salida antes de que lo atrapen.
        NivelLaberinto n("##############\n#G..M.......S#\n##############\n", "huida.txt", NivelDificultad::FACIL);
        n.protagonista()->setEntrada(vector2(1, 0), true);
        int f = 0;
        while (!n.condicionVictoria() && !n.condicionDerrota() && f < 60 * 10) {
            n.actualizar(DT);
            ++f;
        }
        verificar(n.condicionVictoria(), "corriendo hacia la salida gana (el Minion es mas lento)");
    }
    {
        // Mario dormido: los Minions lo oyen roncar aunque esten de espaldas.
        //   El Minion esta a la derecha de Mario mirando a la derecha.
        NivelLaberinto n("###############\n#....MG.......#\n#S#############\n", "ronca.txt", NivelDificultad::NORMAL);
        int f = 0;
        while (!n.condicionDerrota() && f < 60 * 15) {
            n.actualizar(DT);
            ++f;
        }
        verificar(n.protagonista()->estaCapturado(), "quedarse quieto (se duerme y ronca) hace que lo atrapen");
    }

    printf("Victoria\n");
    {
        NivelLaberinto n(PASILLO, "pasillo.txt", NivelDificultad::NORMAL);
        verificar(!n.condicionVictoria() && !n.condicionDerrota(), "al empezar no ha ganado ni perdido");
        n.protagonista()->setEntrada(vector2(1, 0), false);
        int f = 0;
        while (!n.condicionVictoria() && f < 60 * 15) {
            n.actualizar(DT);
            ++f;
        }
        verificar(n.condicionVictoria(), "caminando hasta la S gana");
        verificar(fabs(static_cast<float>(f) * DT - 18.5f / NL::VEL_CAMINAR) < 0.5f,
                  "tarda lo que da la fisica (18.5 m a 2.5 m/s = 7.4 s)");
    }

    printf("Dormilon en el nivel\n");
    {
        NivelLaberinto n(PASILLO, "pasillo.txt", NivelDificultad::NORMAL);
        simular(n, n.protagonista()->tiempoParaDormir() + 0.1f);
        verificar(n.protagonista()->estaDormido(), "quieto 3.5 s (normal) se duerme: el nivel activo el dormilon");
        n.protagonista()->setEntrada(vector2(1, 0), false);
        simular(n, 0.5f);
        verificar(fabs(n.protagonista()->velocidad().x) < 1e-3f, "dormido no se mueve aunque presione las teclas");
    }

    printf("Evento por tiempo: apagon (cambia con la dificultad)\n");
    {
        NivelLaberinto n("#########\n#M..G..S#\n#########\n", "luz.txt", NivelDificultad::NORMAL);
        const MinionGuardia& g = *n.guardias()[0];
        const ParametrosApagon& a = n.apagon();
        const float radioNormal = g.radioVision();
        verificar(!n.lucesApagadas() && !n.avisoApagon(), "empieza con luz y sin aviso");
        simular(n, a.intervalo - a.aviso - 0.5f);
        verificar(!n.avisoApagon(), "antes del aviso no parpadea");
        simular(n, 1.0f);
        verificar(n.avisoApagon() && !n.lucesApagadas(), "los ultimos 2 s antes del apagon hay aviso (parpadeo)");
        simular(n, a.aviso);
        verificar(n.lucesApagadas() && casi(g.radioVision(), radioNormal * a.factorMinion, 1e-4f),
                  "a los 20 s se apaga y los Minions ven el 60 %");
        verificar(!n.avisoApagon(), "a oscuras ya no hay aviso");
        verificar(casi(n.radioVisibleMario(), 3.0f, 1e-4f), "a oscuras el jugador ve 3 m alrededor de Mario");
        simular(n, a.duracion);
        verificar(!n.lucesApagadas() && casi(g.radioVision(), radioNormal, 1e-4f),
                  "6 s despues vuelve la luz y los Minions ven normal");
        verificar(n.tiempoParaCambioDeLuz() > 0.0f, "tiempoParaCambioDeLuz() sirve para una cuenta regresiva");

        // Varios ciclos seguidos: el reloj no se desfasa.
        NivelLaberinto c("#########\n#M..G..S#\n#########\n", "luz.txt", NivelDificultad::NORMAL);
        int apagones = 0;
        bool antes = false;
        for (int i = 0; i < 60 * 260; ++i) {          // 260 s = 10 ciclos de 26 s
            c.actualizar(DT);
            if (c.lucesApagadas() && !antes) ++apagones;
            antes = c.lucesApagadas();
        }
        verificar(apagones == 10, "en 260 s hay exactamente 10 apagones (ciclos de 20 + 6 s)");

        const ParametrosApagon f = ParametrosApagon::segunDificultad(NivelDificultad::FACIL);
        const ParametrosApagon d = ParametrosApagon::segunDificultad(NivelDificultad::DIFICIL);
        verificar(d.intervalo < a.intervalo && a.intervalo < f.intervalo, "en dificil se apaga mas seguido");
        verificar(d.duracion > a.duracion && a.duracion > f.duracion, "en dificil el apagon dura mas");
        verificar(d.radioVisibleMario < a.radioVisibleMario && a.radioVisibleMario < f.radioVisibleMario,
                  "en dificil el jugador ve menos a oscuras");
        verificar(d.factorMinion > a.factorMinion && a.factorMinion > f.factorMinion,
                  "en dificil los Minions pierden menos vista a oscuras");
        verificar(d.aviso < a.aviso && a.aviso < f.aviso, "en dificil el aviso es mas corto");

        NivelLaberinto nd("#########\n#M..G..S#\n#########\n", "luz.txt", NivelDificultad::DIFICIL);
        simular(nd, d.intervalo + 0.1f);
        verificar(nd.lucesApagadas() && casi(nd.radioVisibleMario(), 2.0f, 1e-4f),
                  "en dificil se apaga a los 12 s y el jugador solo ve 2 m");
    }

    printf("Mapa del juego y memoria\n");
    {
        bool lanzo = false;
        try {
            NivelLaberinto n("#####\n#M.X#\n#####\n", "malo.txt", NivelDificultad::NORMAL);
        } catch (const Excepciones_del_Mapa& e) {
            lanzo = e.linea() == 2;
        }
        verificar(lanzo, "un mapa invalido lanza Excepciones_del_Mapa (y no deja fugas)");

        Nivel* base = new NivelLaberinto("#########\n#M.G.B.S#\n#.......#\n#########\n", "b.txt", NivelDificultad::DIFICIL);
        simular(*static_cast<NivelLaberinto*>(base), 2.0f);
        delete base;
        verificar(true, "delete por Nivel* libera Mario, Minions y bloques (lo revisa el sanitizer)");

        gestorlogico juego(fabricaPasillo, 1);
        juego.iniciar();
        verificar(juego.estado() == EstadoJuego::JUGANDO && juego.nivelactual() != nullptr,
                  "gestorlogico crea el NivelLaberinto con la fabrica");
        juego.nivelactual()->protagonista()->setEntrada(vector2(1, 0), true);
        for (int i = 0; i < 60 * 15 && juego.estado() == EstadoJuego::JUGANDO; ++i)
            juego.actualizar(DT);
        verificar(juego.estado() == EstadoJuego::VICTORIA, "jugando con gestorlogico se gana al llegar a la salida");
    }

    correctas = total - fallas;
    totales = total;
}

} // namespace

void pruebasNivelLaberinto(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}