// Pruebas del MinionGuardia y su agente inteligente.
// No tiene main(): main_pruebas.cpp llama a pruebasMinionGuardia().
// Aquí se prueba el agente solo; el movimiento con las físicas reales se
// prueba en prueba_NivelLaberinto.cpp.

#include "MinionGuardia.h"
#include "Excepciones_del_Juego.h"
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
bool casi(float a, float b, float tol = 1e-3f) { return fabs(a - b) < tol; }

const float DT = 1.0f / 60.0f;
typedef MapaLaberinto::Celda C;

// Movimiento ideal (sin el modelo físico del nivel): el Minion toma
// directamente la velocidad que quiere. Sirve para probar solo el agente.
void pasoIdeal(MinionGuardia& g, vector2 mario, bool dormido)
{
    g.pensar(DT, mario, dormido);
    g.setVelocidad(g.velocidadDeseada());
    g.actualizar(DT);
}

void ejecutar(int& correctas, int& totales)
{
    const ParametrosGuardia normal = ParametrosGuardia::segunDificultad(NivelDificultad::NORMAL);

    printf("Parametros segun la dificultad\n");
    {
        const ParametrosGuardia f = ParametrosGuardia::segunDificultad(NivelDificultad::FACIL);
        const ParametrosGuardia d = ParametrosGuardia::segunDificultad(NivelDificultad::DIFICIL);
        verificar(f.radioVision < normal.radioVision && normal.radioVision < d.radioVision,
                  "en dificil ve mas lejos");
        verificar(f.aperturaCono < normal.aperturaCono && normal.aperturaCono < d.aperturaCono,
                  "en dificil el cono es mas ancho");
        verificar(f.velPersecucion < normal.velPersecucion && normal.velPersecucion < d.velPersecucion,
                  "en dificil persigue mas rapido");
        verificar(f.tiempoAlerta > normal.tiempoAlerta && normal.tiempoAlerta > d.tiempoAlerta,
                  "en dificil se alerta mas rapido");
        verificar(f.radioOido < normal.radioOido && normal.radioOido < d.radioOido,
                  "en dificil oye roncar desde mas lejos");
        verificar(d.velPersecucion < 4.0f, "Mario corriendo (4 m/s) siempre es mas rapido que un Minion");
    }

    printf("Construccion\n");
    {
        MapaLaberinto m("#########\n#G.....S#\n#M#######\n", "a.txt");
        MinionGuardia g(m, C{1, 1}, normal, 1);
        verificar(g.estado() == EstadoAgente::PATRULLAR && casi(g.alerta(), 0.0f), "empieza patrullando, sin alerta");
        verificar(casi(g.posicion().x, m.centroDe(1, 1).x) && casi(g.posicion().y, m.centroDe(1, 1).y),
                  "empieza en el centro de su puesto (G)");
        verificar(casi(g.radioVision(), normal.radioVision), "toma el radio de vision de la dificultad");

        bool lanzo = false;
        try { MinionGuardia p(m, C{0, 0}, normal, 1); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "un puesto dentro de una pared lanza excepcion");

        ParametrosGuardia malo = normal;
        malo.velPatrulla = 0.0f;
        lanzo = false;
        try { MinionGuardia p(m, C{1, 1}, malo, 1); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "velocidad de patrulla 0 lanza excepcion");
    }

    printf("Percepcion: ve a Mario (cono + paredes)\n");
    {
        // Pasillo largo. El Minion empieza mirando a la derecha (angulo 0).
        MapaLaberinto m("###########\n#G.......S#\n#M#########\n", "pasillo.txt");
        {
            MinionGuardia g(m, C{1, 1}, normal, 1);
            g.pensar(DT, m.centroDe(5, 1), false);          // 4 m al frente
            verificar(g.veAMario() && g.estado() == EstadoAgente::SOSPECHAR,
                      "Mario 4 m al frente: lo ve y pasa a SOSPECHAR");
            verificar(g.velocidadDeseada().normaCuadrada() == 0.0f, "al sospechar se queda quieto mirando");
        }
        {
            MinionGuardia g(m, C{1, 1}, normal, 1);
            g.pensar(DT, m.centroDe(8, 1), false);          // 7 m: fuera del radio (5 m)
            verificar(!g.veAMario() && g.estado() == EstadoAgente::PATRULLAR, "Mario a 7 m: no lo ve (radio 5 m)");
        }
        {
            // Mario detras de una pared, a 2 m.
            MapaLaberinto p("#######\n#G#.S.#\n#M#####\n", "pared.txt");
            MinionGuardia g(p, C{1, 1}, normal, 1);
            g.pensar(DT, p.centroDe(3, 1), false);
            verificar(!g.veAMario(), "detras de una pared no lo ve aunque este cerca");
        }
        {
            // Mario a la espalda (el Minion mira a la derecha).
            MapaLaberinto e("###########\n#....G...S#\n#M#########\n", "espalda.txt");
            MinionGuardia g(e, C{5, 1}, normal, 1);
            g.pensar(DT, e.centroDe(3, 1), false);           // 2 m atras, despierto
            verificar(!g.veAMario(), "a la espalda no lo ve (fuera del cono)");
            MinionGuardia g2(e, C{5, 1}, normal, 1);
            g2.pensar(DT, e.centroDe(3, 1), true);          // 2 m atras, DORMIDO
            verificar(g2.veAMario(), "a la espalda pero dormido: lo oye roncar (radioOido)");
            verificar(casi(g2.alerta(), 1.0f), "dormido: la alerta se llena de una vez (es vulnerable)");
            g2.pensar(DT, e.centroDe(3, 1), true);
            verificar(g2.estado() == EstadoAgente::PERSEGUIR, "y en el siguiente fotograma lo persigue");
        }
    }

    printf("Decision: SOSPECHAR -> PERSEGUIR -> BUSCAR -> PATRULLAR\n");
    {
        MapaLaberinto m("###########\n#G.......S#\n#M#########\n", "pasillo.txt");
        MinionGuardia g(m, C{1, 1}, normal, 1);
        const vector2 mario = m.centroDe(5, 1);

        g.pensar(DT, mario, false);
        int fotogramas = 1;
        while (g.estado() == EstadoAgente::SOSPECHAR && fotogramas < 600) {
            g.pensar(DT, mario, false);
            ++fotogramas;
        }
        verificar(g.estado() == EstadoAgente::PERSEGUIR, "viendolo un rato, pasa a PERSEGUIR");
        verificar(fabs(static_cast<float>(fotogramas) * DT - normal.tiempoAlerta) < 3.0f * DT,
                  "tarda tiempoAlerta (0.8 s en normal) en decidir perseguirlo");
        verificar(g.velocidadDeseada().x > 0.0f && casi(g.velocidadDeseada().norma(), normal.velPersecucion),
                  "persigue hacia Mario a su velocidad de persecucion");

        // Mario se esconde: deja de estar a la vista (aqui, fuera del mapa).
        const vector2 escondido(-50.0f, -50.0f);
        g.pensar(DT, escondido, false);
        verificar(g.estado() == EstadoAgente::BUSCAR, "si lo pierde de vista pasa a BUSCAR");

        // Va a donde lo vio por ultima vez y busca alrededor.
        int pasos = 0;
        bool llego = false;
        while (g.estado() == EstadoAgente::BUSCAR && pasos < 60 * 20) {
            pasoIdeal(g, escondido, false);
            if (m.celdaDe(g.posicion()) == C{5, 1}) llego = true;
            ++pasos;
        }
        verificar(llego, "fue hasta donde lo vio por ultima vez");
        verificar(g.estado() == EstadoAgente::PATRULLAR, "despues de buscar un rato vuelve a PATRULLAR");
        verificar(casi(g.alerta(), 0.0f), "y la alerta vuelve a 0");
    }
    {
        // Si solo lo ve un momento, la alerta baja y vuelve a patrullar.
        MapaLaberinto m("###########\n#G.......S#\n#M#########\n", "pasillo.txt");
        MinionGuardia g(m, C{1, 1}, normal, 1);
        for (int i = 0; i < 12; ++i) g.pensar(DT, m.centroDe(5, 1), false);   // 0.2 s
        const float alertaVista = g.alerta();
        verificar(g.estado() == EstadoAgente::SOSPECHAR && alertaVista > 0.0f && alertaVista < 1.0f,
                  "lo vio 0.2 s: sospecha, la alerta subio un poco");
        int f = 0;
        while (g.estado() == EstadoAgente::SOSPECHAR && f < 600) {
            g.pensar(DT, vector2(-50.0f, -50.0f), false);
            ++f;
        }
        verificar(g.estado() == EstadoAgente::PATRULLAR && casi(g.alerta(), 0.0f),
                  "si deja de verlo, la alerta baja y vuelve a PATRULLAR");
    }

    printf("Accion: camino mas corto y patrulla\n");
    {
        // Sala con columnas: para patrullar tiene que rodearlas.
        MapaLaberinto m("###############\n#.............#\n#.##.##G##.##.#\n#.............#\n#M###########S#\n", "sala.txt");
        MinionGuardia g(m, C{7, 2}, normal, 7);
        bool dentroDeZona = true, nuncaEnPared = true;
        float recorrido = 0.0f;
        vector2 antes = g.posicion();
        for (int i = 0; i < 60 * 30; ++i) {          // 30 s patrullando, Mario lejos
            pasoIdeal(g, vector2(-50.0f, -50.0f), false);
            const C c = m.celdaDe(g.posicion());
            if (abs(c.columna - 7) + abs(c.fila - 2) > static_cast<int>(normal.radioPatrulla) + 1) dentroDeZona = false;
            if (m.esParedEn(g.posicion())) nuncaEnPared = false;
            recorrido += (g.posicion() - antes).norma();
            antes = g.posicion();
        }
        verificar(recorrido > 10.0f, "patrullando se mueve (recorre mas de 10 m en 30 s)");
        verificar(dentroDeZona, "no se aleja de su puesto mas de radioPatrulla celdas");
        verificar(nuncaEnPared, "patrullando nunca se mete en una pared");
        verificar(g.estado() == EstadoAgente::PATRULLAR, "sin ver a Mario sigue patrullando");

        MinionGuardia a(m, C{7, 2}, normal, 3), b(m, C{7, 2}, normal, 3);
        bool iguales = true;
        for (int i = 0; i < 600; ++i) {
            pasoIdeal(a, vector2(-50.0f, -50.0f), false);
            pasoIdeal(b, vector2(-50.0f, -50.0f), false);
            if (!(a.posicion() == b.posicion())) iguales = false;
        }
        verificar(iguales, "con la misma semilla patrulla igual (las pruebas son repetibles)");
    }

    printf("Apagon (evento por tiempo)\n");
    {
        MapaLaberinto m("###########\n#G.......S#\n#M#########\n", "pasillo.txt");
        MinionGuardia g(m, C{1, 1}, normal, 1);
        g.setFactorLuz(0.5f);
        verificar(casi(g.radioVision(), normal.radioVision * 0.5f), "con las luces apagadas ve a la mitad de distancia");
        g.pensar(DT, m.centroDe(5, 1), false);          // 4 m: antes lo veia
        verificar(!g.veAMario(), "a oscuras ya no ve a Mario a 4 m");
        g.setFactorLuz(1.0f);
        verificar(casi(g.radioVision(), normal.radioVision), "al volver la luz recupera su vision");
        g.setFactorLuz(0.0f);
        verificar(g.radioVision() > 0.0f, "nunca queda con radio 0 (no lanza excepcion)");
    }

    printf("Aprendizaje (4): aprende de cada encuentro\n");
    {
        MapaLaberinto m("###########\n#G.......S#\n#M#########\n", "pasillo.txt");
        const vector2 mario = m.centroDe(5, 1);
        const vector2 escondido(-50.0f, -50.0f);

        // Un encuentro completo: lo ve hasta perseguirlo, Mario se esconde y
        // el Minion lo busca hasta rendirse. Devuelve los fotogramas que tardó
        // en decidir perseguirlo (su tiempo de reacción).
        auto encuentro = [&](MinionGuardia& g) {
            int f = 0;
            while (g.estado() != EstadoAgente::PERSEGUIR && f < 600) { g.pensar(DT, mario, false); ++f; }
            int pasos = 0;
            while (g.estado() != EstadoAgente::PATRULLAR && pasos < 60 * 30) { pasoIdeal(g, escondido, false); ++pasos; }
            return f;
        };

        MinionGuardia g(m, C{1, 1}, normal, 1);
        verificar(g.escapes() == 0 && casi(g.factorReaccion(), 1.0f) && casi(g.pesoVigilar(), 0.0f)
                      && g.zonasFrecuentes().empty(), "empieza sin nada aprendido");

        const int reaccion1 = encuentro(g);
        verificar(g.escapes() == 1, "registro de exito/fracaso: cuenta que Mario se escapo");
        verificar(casi(g.factorReaccion(), 0.85f), "despues de un escape reacciona 15 % mas rapido (normal)");
        verificar(casi(g.pesoVigilar(), 0.2f), "y sube el peso de ir a vigilar zonas frecuentes");
        bool guardoZona = false;
        for (const ZonaFrecuente& z : g.zonasFrecuentes())
            if (z.celda == C{5, 1} && z.peso >= 2.0f) guardoZona = true;
        verificar(guardoZona, "guarda la zona donde vio a Mario (y le sube el peso)");

        g.setPosicion(m.centroDe(1, 1));
        g.setAnguloVision(0.0f);
        const int reaccion2 = encuentro(g);
        verificar(reaccion2 < reaccion1, "en el siguiente encuentro decide perseguirlo antes");
        verificar(fabs(static_cast<float>(reaccion2) * DT - normal.tiempoAlerta * 0.85f) < 3.0f * DT,
                  "tarda tiempoAlerta x factorReaccion (0.8 x 0.85 = 0.68 s)");

        for (int i = 0; i < 12; ++i) {
            g.setPosicion(m.centroDe(1, 1));
            g.setAnguloVision(0.0f);
            encuentro(g);
        }
        verificar(g.factorReaccion() >= MinionGuardia::FACTOR_REACCION_MINIMO - 1e-4f,
                  "por mas que aprenda, el factor no baja del minimo (sigue siendo justo)");
        verificar(g.pesoVigilar() <= MinionGuardia::PESO_VIGILAR_MAXIMO + 1e-4f,
                  "y el peso de vigilar no pasa del maximo");
        verificar(static_cast<int>(g.zonasFrecuentes().size()) <= MinionGuardia::MAX_ZONAS,
                  "guarda como mucho MAX_ZONAS zonas (memoria acotada)");

        const ParametrosGuardia pf = ParametrosGuardia::segunDificultad(NivelDificultad::FACIL);
        const ParametrosGuardia pd = ParametrosGuardia::segunDificultad(NivelDificultad::DIFICIL);
        MinionGuardia gf(m, C{1, 1}, pf, 1), gd(m, C{1, 1}, pd, 1);
        encuentro(gf);
        encuentro(gd);
        verificar(gd.factorReaccion() < gf.factorReaccion(), "en dificil aprende mas rapido que en facil");
    }
    {
        // Lo aprendido cambia la patrulla: va mas a la zona donde vio a Mario.
        MapaLaberinto m("###############\n#.............#\n#.##.##G##.##.#\n#.............#\n#M###########S#\n", "sala.txt");
        const C zona{2, 3};
        MinionGuardia novato(m, C{7, 2}, normal, 5), experto(m, C{7, 2}, normal, 5);
        // El experto vivio un encuentro en esa zona y Mario se le escapo.
        experto.setPosicion(m.centroDe(3, 3));
        experto.setAnguloVision(3.14159265f);
        int f = 0;
        while (experto.estado() != EstadoAgente::PERSEGUIR && f < 600) { experto.pensar(DT, m.centroDe(zona), false); ++f; }
        int pasos = 0;
        while (experto.estado() != EstadoAgente::PATRULLAR && pasos < 60 * 30) { pasoIdeal(experto, vector2(-50.0f, -50.0f), false); ++pasos; }
        verificar(experto.escapes() == 1, "el experto ya vivio un encuentro");

        int visitasNovato = 0, visitasExperto = 0;
        bool novatoEn = false, expertoEn = false;
        for (int i = 0; i < 60 * 120; ++i) {      // 2 minutos patrullando
            pasoIdeal(novato, vector2(-50.0f, -50.0f), false);
            pasoIdeal(experto, vector2(-50.0f, -50.0f), false);
            const bool n = m.celdaDe(novato.posicion()) == zona, e = m.celdaDe(experto.posicion()) == zona;
            if (n && !novatoEn) ++visitasNovato;
            if (e && !expertoEn) ++visitasExperto;
            novatoEn = n; expertoEn = e;
        }
        printf("     (visitas a la zona en 2 min: novato %d, experto %d)\n", visitasNovato, visitasExperto);
        verificar(visitasExperto > visitasNovato, "el que aprendio vigila mas la zona donde vio a Mario");
    }

    correctas = total - fallas;
    totales = total;
}

} // namespace

void pruebasMinionGuardia(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}