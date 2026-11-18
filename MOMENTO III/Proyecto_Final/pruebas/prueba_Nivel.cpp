#include "Nivel.h"
#include "Personaje.h"
#include "gestorlogico.h"
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

// Cuenta cuántos personajes existen, para detectar fugas o doble delete.
static int personajesVivos = 0;
static int llamadasActualizar = 0;

class PersonajePrueba : public Personaje
{
public:
    explicit PersonajePrueba(vector2 p) : Personaje(p, 1.0f) { ++personajesVivos; }
    ~PersonajePrueba() override { --personajesVivos; }
    void actualizar(float dt) override { ++llamadasActualizar; integrar(dt); }
};

} // namespace

// Protagonista de prueba (el real se hará después).
// Va fuera del namespace porque Nivel.h declara 'class Protagonista;' global.
class Protagonista : public PersonajePrueba
{
public:
    explicit Protagonista(vector2 p) : PersonajePrueba(p) {}
};

namespace {

// Nivel de prueba: Mario cae con gravedad; se pierde si baja de y = -10
// y se gana si llega a x >= 5.
class NivelPrueba : public Nivel
{
public:
    explicit NivelPrueba(int minions)
    {
        reservarPersonajes(static_cast<size_t>(minions) + 1);
        Protagonista* mario = new Protagonista(vector2(0, 0));
        agregarPersonaje(mario);
        protagonista_ = mario;
        for (int i = 0; i < minions; ++i)
            agregarPersonaje(new PersonajePrueba(vector2(static_cast<float>(i), 0)));
    }

    void actualizar(float dt) override
    {
        avanzarTiempo(dt);
        protagonista_->aplicarFuerza(vector2(0.0f, -9.8f));   // física del nivel
        actualizarPersonajes(dt);
    }
    bool condicionVictoria() const override { return protagonista_->posicion().x >= 5.0f; }
    bool condicionDerrota() const override { return protagonista_->posicion().y < -10.0f; }

    // Para las pruebas: expone los métodos protegidos.
    void agregar(Personaje* p) { agregarPersonaje(p); }
    size_t capacidad() const { return personajes().capacity(); }
};

// Nivel que falla a mitad de su construcción (por ejemplo, falta el mapa).
class NivelQueFalla : public Nivel
{
public:
    NivelQueFalla()
    {
        agregarPersonaje(new PersonajePrueba(vector2(0, 0)));
        agregarPersonaje(new PersonajePrueba(vector2(1, 0)));
        throw Excepciones_del_Juego("archivo del mapa no encontrado");
    }
    void actualizar(float) override {}
    bool condicionVictoria() const override { return false; }
    bool condicionDerrota() const override { return false; }
};

static Nivel* fabricaPrueba(int, NivelDificultad) { return new NivelPrueba(3); }

void ejecutar(int& correctas, int& totales)
{
    printf("Construccion y memoria\n");
    {
        {
            NivelPrueba n(3);
            verificar(n.personajes().size() == 4, "el nivel tiene a Mario y 3 Minions");
            verificar(n.protagonista() != nullptr && n.personajes()[0] == static_cast<Personaje*>(n.protagonista()),
                      "protagonista() apunta a Mario, que esta en la lista");
            verificar(personajesVivos == 4, "hay 4 personajes en memoria");
            verificar(n.capacidad() == 4, "reservarPersonajes reservo justo lo necesario");
        }
        verificar(personajesVivos == 0, "al destruir el nivel se liberan todos (Mario una sola vez)");

        Nivel* base = new NivelPrueba(2);
        delete base;
        verificar(personajesVivos == 0, "delete por Nivel* libera todo (destructor virtual)");
    }

    printf("Validaciones al agregar\n");
    {
        NivelPrueba n(1);
        bool lanzo = false;
        try { n.agregar(nullptr); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "agregar un personaje nulo lanza excepcion");

        lanzo = false;
        try { n.agregar(n.personajes()[1]); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "agregar dos veces el mismo personaje lanza excepcion (evita doble delete)");
        verificar(n.personajes().size() == 2, "despues de los errores la lista no cambio");
    }
    verificar(personajesVivos == 0, "sin fugas despues de los errores");

    printf("Error a mitad de la construccion\n");
    {
        bool lanzo = false;
        try { NivelQueFalla n; } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "la excepcion sale del constructor");
        verificar(personajesVivos == 0, "los personajes ya agregados se liberaron (sin fugas)");
    }

    printf("Actualizacion\n");
    {
        NivelPrueba n(3);
        llamadasActualizar = 0;
        for (int i = 0; i < 60; ++i) n.actualizar(1.0f / 60.0f);
        verificar(llamadasActualizar == 4 * 60, "en cada fotograma se actualiza a los 4 personajes");
        verificar(fabs(n.tiempoTranscurrido() - 1.0f) < 1e-4f, "el tiempo del nivel suma 1 s en 60 fotogramas");
        verificar(n.protagonista()->posicion().y < 0.0f, "la fuerza que aplica el nivel mueve a Mario");
    }

    printf("Integracion con gestorlogico\n");
    {
        gestorlogico g(fabricaPrueba, 1);
        g.iniciar();
        verificar(g.estado() == EstadoJuego::JUGANDO && g.nivelactual() != nullptr, "gestorlogico crea el nivel");
        for (int i = 0; i < 200 && g.estado() == EstadoJuego::JUGANDO; ++i) g.actualizar(1.0f / 60.0f);
        verificar(g.estado() == EstadoJuego::VIDA_PERDIDA && g.vidas() == 2, "Mario cae, se pierde una vida");
        g.reiniciarNivel();
        verificar(personajesVivos == 4, "al reiniciar solo existen los personajes del nivel nuevo");
    }
    verificar(personajesVivos == 0, "al destruir gestorlogico no queda ningun personaje en memoria");
    correctas = total - fallas;
    totales = total;
}

} // namespace

// La llama main_pruebas.cpp
void pruebasNivel(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}