// Pruebas de NivelBloques (nivel 1) y sus clases: BloqueTierra, ItemPoder,
// Foso, MinionEjercitoColombiano y la fábrica de niveles.
// No tiene main(): main_pruebas.cpp llama a pruebasNivelBloques().

#include "nivelbloques.h"
#include "Protagonista.h"
#include "minionejercitocolombiano.h"
#include "fabricaniveles.h"
#include "gestorlogico.h"
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
bool casi(float a, float b, float tol) { return fabs(a - b) <= tol; }

const float DT = 1.0f / 60.0f;

void simular(NivelBloques& n, float segundos)
{
    const int pasos = static_cast<int>(segundos * 60.0f + 0.5f);
    for (int i = 0; i < pasos; ++i) n.actualizar(DT);
}

// Mapa mínimo: un bloque de tierra ancho y Mario parado encima.
MapaNivelBloques mapaSobreTierra()
{
    MapaNivelBloques m;
    m.bloques = { BloqueTierra(0.0f, 40.0f, 1.0f, 0.5f, false) };
    m.inicioMario = vector2(2.0f, 1.0f);
    m.xMeta = 39.0f;
    return m;
}

// Suelo firme ancho, para saltar y caminar.
MapaNivelBloques mapaSueloFirme()
{
    MapaNivelBloques m;
    m.bloques = { BloqueTierra(0.0f, 40.0f, 0.0f, 1.0f, true) };
    m.inicioMario = vector2(2.0f, 0.0f);
    m.xMeta = 39.0f;
    return m;
}

// Suelo, un foso con un Minion y otro suelo.
MapaNivelBloques mapaConFoso()
{
    MapaNivelBloques m;
    m.bloques = { BloqueTierra(0.0f, 5.0f, 0.0f, 1.0f, true),
                 BloqueTierra(15.0f, 10.0f, 0.0f, 1.0f, true) };
    m.fosos = { {5.0f, 15.0f, 4.0f} };
    m.minions = { vector2(6.0f, -4.0f) };
    m.inicioMario = vector2(2.0f, 0.0f);
    m.xMeta = 24.0f;
    return m;
}

// Mario empieza en el aire sobre un foso muy hondo (para medir la caída).
MapaNivelBloques mapaCaida()
{
    MapaNivelBloques m;
    m.bloques = { BloqueTierra(50.0f, 5.0f, 0.0f, 1.0f, true) };
    m.fosos = { {0.0f, 50.0f, 1000.0f} };
    m.inicioMario = vector2(2.0f, -0.01f);
    m.xMeta = 54.0f;
    return m;
}

Nivel* fabricaConFoso(int, NivelDificultad d) { return new NivelBloques(mapaConFoso(), d); }

void ejecutar(int& correctas, int& totales)
{
    printf("Construccion del mapa del juego\n");
    {
        NivelBloques n(NivelBloques::mapaPorDefecto(NivelDificultad::NORMAL), NivelDificultad::NORMAL);
        verificar(n.personajes().size() == 6, "Mario y 5 Minions en la lista de personajes");
        verificar(n.protagonista() != nullptr && n.indiceBloqueApoyo() == 0, "Mario empieza parado en el suelo A");
        bool ordenados = true;
        for (size_t i = 1; i < n.bloques().size(); ++i)
            if (n.bloques()[i].xInicio() < n.bloques()[i - 1].xInicio()) ordenados = false;
        verificar(ordenados, "los bloques quedan ordenados por x");
        verificar(n.personajes().capacity() == 6, "reservarPersonajes reservo justo lo necesario");

        NivelBloques f(NivelBloques::mapaPorDefecto(NivelDificultad::FACIL), NivelDificultad::FACIL);
        NivelBloques d(NivelBloques::mapaPorDefecto(NivelDificultad::DIFICIL), NivelDificultad::DIFICIL);
        verificar(f.items().size() == 1 && n.items().size() == 2 && d.items().size() == 3,
                  "items de poder: 1 en facil, 2 en normal, 3 en dificil");
        verificar(f.tiempoDebilitamiento() > n.tiempoDebilitamiento() &&
                      n.tiempoDebilitamiento() > d.tiempoDebilitamiento(),
                  "en dificil los bloques se debilitan mas seguido");
    }

    printf("Validacion del mapa (excepciones)\n");
    {
        bool lanzo = false;
        MapaNivelBloques vacio; vacio.xMeta = 10.0f;
        try { NivelBloques n(vacio, NivelDificultad::NORMAL); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "un mapa sin bloques lanza excepcion");

        lanzo = false;
        MapaNivelBloques m = mapaSueloFirme();
        m.bloques.push_back(BloqueTierra(5.0f, 0.0f, 1.0f, 1.0f, false));
        try { NivelBloques n(m, NivelDificultad::NORMAL); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "un bloque de ancho 0 lanza Excepciones_de_Parametros_Fisicos");

        lanzo = false;
        m = mapaConFoso();
        m.fosos.push_back({10.0f, 8.0f, 3.0f});
        try { NivelBloques n(m, NivelDificultad::NORMAL); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "un foso al reves (xFin < xInicio) lanza excepcion");

        lanzo = false;
        m = mapaSueloFirme(); m.xMeta = 1.0f;
        try { NivelBloques n(m, NivelDificultad::NORMAL); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "una meta detras de Mario lanza excepcion");
    }

    printf("Fisica 2: resorte amortiguado del bloque\n");
    {
        NivelBloques n(mapaSobreTierra(), NivelDificultad::NORMAL);
        const float esperado = n.gravedad() / n.rigidezInicial();          // m·g/k con m = 1
        simular(n, 0.1f);
        const float alInicio = n.bloques()[0].hundimiento();
        simular(n, 3.9f);
        verificar(alInicio > 0.0f && alInicio != n.bloques()[0].hundimiento(), "el bloque se mueve al recibir a Mario (oscila)");
        verificar(casi(n.bloques()[0].hundimiento(), esperado, 0.005f), "Mario normal se hunde m*g/k = 0.05");
        verificar(casi(n.protagonista()->posicion().y, 1.0f - esperado, 0.005f), "Mario baja junto con la superficie del bloque");
        simular(n, 6.0f);
        verificar(!n.bloques()[0].roto(), "Mario normal quieto 10 s no rompe el bloque");

        NivelBloques p(mapaSobreTierra(), NivelDificultad::NORMAL);
        p.protagonista()->activarPoderPesado();
        p.protagonista()->setEntrada(vector2(1, 0), false);              // pesado pero caminando
        simular(p, 4.0f);
        verificar(casi(p.bloques()[0].hundimiento(), 3.0f * esperado, 0.01f), "pesado se hunde el triple (0.15)");
        verificar(!p.bloques()[0].roto() && casi(p.bloques()[0].rigidez(), p.rigidezInicial(), 0.01f),
                  "pesado caminando no debilita el bloque");

        NivelBloques q(mapaSobreTierra(), NivelDificultad::NORMAL);
        q.protagonista()->activarPoderPesado();                            // pesado y quieto
        simular(q, 1.0f);
        verificar(!q.bloques()[0].roto() && q.bloques()[0].rigidez() < q.rigidezInicial(),
                  "pesado y quieto: el bloque se debilita (k baja)");
        simular(q, 3.0f);
        verificar(q.bloques()[0].roto(), "pesado y quieto unos segundos: el bloque se rompe");
        verificar(q.indiceBloqueApoyo() == -1 && q.enCaidaLibre(), "al romperse Mario queda en caida libre");
        verificar(q.protagonista()->posicion().y < 1.0f - q.limiteHundimiento(), "y empieza a caer");
    }

    printf("Fisica 1: tiro parabolico (salto)\n");
    {
        NivelBloques n(mapaSueloFirme(), NivelDificultad::NORMAL);
        Protagonista* mario = n.protagonista();
        const float v0 = mario->impulsoSalto() / mario->masa();
        const float hTeorica = v0 * v0 / (2.0f * n.gravedad());
        mario->solicitarSalto();
        float hmax = 0.0f;
        for (int i = 0; i < 120; ++i) { n.actualizar(DT); hmax = max(hmax, mario->posicion().y); }
        verificar(fabs(hmax - hTeorica) / hTeorica < 0.03f, "altura del salto dentro del 3% de v0^2/(2g)");
        verificar(n.indiceBloqueApoyo() == 0 && casi(mario->posicion().y, 0.0f, 1e-4f), "despues del salto vuelve a caer al suelo");

        NivelBloques p(mapaSueloFirme(), NivelDificultad::NORMAL);
        p.protagonista()->activarPoderPesado();
        p.protagonista()->solicitarSalto();
        float hPesado = 0.0f;
        for (int i = 0; i < 120; ++i) { p.actualizar(DT); hPesado = max(hPesado, p.protagonista()->posicion().y); }
        verificar(fabs(hPesado - hTeorica / 9.0f) / (hTeorica / 9.0f) < 0.1f, "pesado salta la novena parte de alto");

        NivelBloques a(mapaSueloFirme(), NivelDificultad::NORMAL);
        a.protagonista()->setEntrada(vector2(1, 0), false);
        a.protagonista()->solicitarSalto();
        simular(a, 0.3f);
        verificar(a.protagonista()->posicion().x > 2.5f && a.protagonista()->posicion().y > 0.5f,
                  "saltando mientras camina describe una parabola (avanza y sube)");
    }

    printf("Fisica 3: caida con resistencia del aire\n");
    {
        NivelBloques n(mapaCaida(), NivelDificultad::NORMAL);
        simular(n, 4.0f);
        const float vtNormal = 1.0f * n.gravedad() / n.coefArrastreAire();       // m·g/c
        verificar(casi(-n.protagonista()->velocidad().y, vtNormal, 0.05f), "Mario normal llega a la velocidad terminal m*g/c");

        NivelBloques p(mapaCaida(), NivelDificultad::NORMAL);
        p.protagonista()->activarPoderPesado();
        simular(p, 4.0f);
        const float vtPesado = 3.0f * p.gravedad() / p.coefArrastreAire();
        const float esperado = vtPesado * (1.0f - exp(-4.0f * p.coefArrastreAire() / 3.0f));
        verificar(casi(-p.protagonista()->velocidad().y, esperado, 0.3f), "pesado sigue v(t) = (m*g/c)(1 - e^(-c*t/m))");
        verificar(-p.protagonista()->velocidad().y > -n.protagonista()->velocidad().y * 2.0f, "pesado cae mucho mas rapido");
    }

    printf("Caminar, paredes y escalones\n");
    {
        MapaNivelBloques m = mapaSueloFirme();
        m.bloques.push_back(BloqueTierra(5.0f, 1.0f, 1.0f, 1.0f, false));    // un escalon de 1 m
        NivelBloques n(m, NivelDificultad::NORMAL);
        Protagonista* mario = n.protagonista();
        mario->setEntrada(vector2(1, 0), false);
        simular(n, 3.0f);
        verificar(casi(mario->posicion().x, 5.0f - NivelBloques::MEDIO_ANCHO_MARIO, 0.01f), "la pared del escalon lo detiene");
        mario->solicitarSalto();
        simular(n, 1.5f);
        verificar(mario->posicion().x > 5.0f, "saltando pasa el escalon");

        NivelBloques c(mapaSueloFirme(), NivelDificultad::NORMAL);
        c.protagonista()->setEntrada(vector2(1, 0), true);
        simular(c, 1.0f);
        NivelBloques w(mapaSueloFirme(), NivelDificultad::NORMAL);
        w.protagonista()->setEntrada(vector2(1, 0), false);
        simular(w, 1.0f);
        verificar(c.protagonista()->posicion().x > w.protagonista()->posicion().x, "corriendo avanza mas que caminando");

        NivelBloques i(mapaSueloFirme(), NivelDificultad::NORMAL);
        i.protagonista()->setEntrada(vector2(-1, 0), false);
        simular(i, 3.0f);
        verificar(casi(i.protagonista()->posicion().x, NivelBloques::MEDIO_ANCHO_MARIO, 1e-4f), "no se sale por el borde izquierdo");
    }

    printf("Item de poder\n");
    {
        MapaNivelBloques m = mapaSueloFirme();
        m.items = { vector2(4.0f, 0.5f) };
        NivelBloques n(m, NivelDificultad::NORMAL);
        const float y0 = n.items()[0].posicion().y;
        simular(n, 0.3f);
        verificar(!casi(n.items()[0].posicion().y, y0, 1e-3f), "el item flota (sube y baja)");
        verificar(!n.protagonista()->esPesado(), "antes de tocarlo Mario no es pesado");
        n.protagonista()->setEntrada(vector2(1, 0), false);
        simular(n, 1.5f);
        verificar(n.items()[0].recogido() && n.protagonista()->esPesado(), "al tocarlo se lo come y queda pesado");
        verificar(casi(n.protagonista()->masa(), 3.0f, 1e-4f), "su masa se triplica");
    }

    printf("Fosos, captura y victoria\n");
    {
        NivelBloques n(mapaConFoso(), NivelDificultad::NORMAL);
        n.protagonista()->setEntrada(vector2(1, 0), false);
        simular(n, 5.0f);
        verificar(n.protagonista()->estaCapturado() && n.condicionDerrota(), "al caer al foso el ejercito lo captura");
        verificar(!n.condicionVictoria(), "capturado no gana");

        MapaNivelBloques sinMinion = mapaConFoso();
        sinMinion.minions.clear();
        NivelBloques s(sinMinion, NivelDificultad::NORMAL);
        s.protagonista()->setEntrada(vector2(1, 0), false);
        simular(s, 5.0f);
        verificar(s.condicionDerrota(), "aunque no haya un Minion justo debajo, el fondo del foso lo captura");

        MapaNivelBloques pared = mapaConFoso();
        pared.minions.clear();
        pared.inicioMario = vector2(14.0f, -0.5f);                       // dentro del foso, cayendo
        NivelBloques w(pared, NivelDificultad::NORMAL);
        w.protagonista()->setEntrada(vector2(1, 0), true);              // intenta salirse por la derecha
        simular(w, 3.0f);
        verificar(w.protagonista()->posicion().x <= 15.0f - NivelBloques::MEDIO_ANCHO_MARIO + 1e-4f && w.condicionDerrota(),
                  "las paredes del foso no lo dejan pasar por debajo del suelo");

        MinionEjercitoColombiano minion(vector2(0.0f, -4.0f));
        verificar(minion.puedeCapturar(vector2(0.5f, -3.0f)) && !minion.puedeCapturar(vector2(0.0f, -1.0f)),
                  "el Minion captura solo lo que esta a su alcance (1.5 m)");

        MapaNivelBloques corto = mapaSueloFirme(); corto.xMeta = 5.0f;
        NivelBloques v(corto, NivelDificultad::NORMAL);
        v.protagonista()->setEntrada(vector2(1, 0), false);
        simular(v, 0.5f);
        verificar(!v.condicionVictoria(), "antes de la meta no ha ganado");
        simular(v, 1.0f);
        verificar(v.condicionVictoria() && !v.condicionDerrota(), "al llegar a la meta gana");
    }

    printf("Evento por tiempo: los bloques se debilitan\n");
    {
        NivelBloques n(NivelBloques::mapaPorDefecto(NivelDificultad::NORMAL), NivelDificultad::NORMAL);
        const float k0 = n.bloques()[1].rigidez();
        simular(n, n.tiempoDebilitamiento() - 0.2f);
        verificar(casi(n.bloques()[1].rigidez(), k0, 1e-3f), "antes del tiempo la rigidez no cambia");
        simular(n, 0.4f);
        verificar(casi(n.bloques()[1].rigidez(), 0.8f * k0, 0.01f), "al cumplirse el tiempo k baja al 80%");
        verificar(casi(n.bloques()[0].rigidez(), k0, 1e-3f), "el suelo firme no se debilita");
    }

    printf("Fabrica de niveles y gestorlogico\n");
    {
        Nivel* nivel = crearNivel(0, NivelDificultad::NORMAL);
        verificar(dynamic_cast<NivelBloques*>(nivel) != nullptr, "crearNivel(0) crea el NivelBloques");
        delete nivel;                                   // por Nivel*: destructor virtual
        bool lanzo = false;
        try { crearNivel(5, NivelDificultad::NORMAL); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "un nivel que no existe lanza excepcion");

        gestorlogico g(crearNivel, totalNiveles());
        g.iniciar();
        verificar(g.estado() == EstadoJuego::JUGANDO && g.nivelactual() != nullptr, "gestorlogico arranca el nivel 1");
        for (int i = 0; i < 600; ++i) g.actualizar(DT);
        verificar(g.estado() == EstadoJuego::JUGANDO, "Mario quieto en el suelo firme sigue jugando");

        gestorlogico h(fabricaConFoso, 1);
        h.iniciar();
        static_cast<NivelBloques*>(h.nivelactual())->protagonista()->setEntrada(vector2(1, 0), false);
        for (int i = 0; i < 300 && h.estado() == EstadoJuego::JUGANDO; ++i) h.actualizar(DT);
        verificar(h.estado() == EstadoJuego::VIDA_PERDIDA && h.vidas() == 2, "caer al foso le quita una vida");
    }

    printf("Casos especiales\n");
    {
        NivelBloques n(mapaSobreTierra(), NivelDificultad::NORMAL);
        const vector2 antes = n.protagonista()->posicion();
        n.actualizar(0.0f);
        n.actualizar(-1.0f);
        n.actualizar(NAN);
        verificar(n.protagonista()->posicion() == antes && n.tiempoTranscurrido() == 0.0f,
                  "dt cero, negativo o NaN no cambia nada");
    }

    correctas = total - fallas;
    totales = total;
}

} // namespace

// La llama main_pruebas.cpp
void pruebasNivelBloques(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}
