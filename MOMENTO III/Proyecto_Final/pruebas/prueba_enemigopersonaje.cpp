/*
 * prueba_enemigo_personaje.cpp
 * ----------------------------
 * Pruebas de las clases Personaje y Enemigo (y de las excepciones que
 * lanzan). No tiene main(): main_pruebas.cpp llama a
 * pruebasPersonajeEnemigo(correctas, totales), igual que a las demás.
 *
 * Todo lo auxiliar (clases de prueba, verificar, contadores) está dentro
 * de un namespace sin nombre, así que solo existe en este archivo y no
 * choca con funciones que se llamen igual en prueba_vector2.cpp.
 *
 * Como Personaje y Enemigo son abstractas (actualizar(dt) = 0), no se
 * pueden crear directamente. Por eso se definen dos clases mínimas solo
 * para probar: JugadorPrueba y EnemigoPrueba. Su actualizar(dt) solo
 * llama a integrar(dt).
 *
 * Cada prueba imprime [OK] o [FALLA] y al final el total.
 */

#include "enemigo.h"
#include "Excepciones_del_Juego.h"
#include <iostream>
#include <cmath>
#include <string>

using namespace std;

namespace {

// ------------------------------------------------------------------
// Clases concretas solo para las pruebas
// ------------------------------------------------------------------
class JugadorPrueba : public Personaje
{
public:
    JugadorPrueba(vector2 posicion, float masa) : Personaje(posicion, masa) {}
    void actualizar(float dt) override { integrar(dt); }
};

class EnemigoPrueba : public Enemigo
{
public:
    EnemigoPrueba(vector2 posicion, float masa,
                  float radio, float angulo, float apertura)
        : Enemigo(posicion, masa, radio, angulo, apertura) {}
    void actualizar(float dt) override { integrar(dt); }
};

// ------------------------------------------------------------------
// Utilidades
// ------------------------------------------------------------------
const float PI = 3.14159265f;
const float TOLERANCIA = 1e-4f;

int pruebasOk = 0;
int pruebasFallidas = 0;

void verificar(bool condicion, const string& nombre)
{
    if (condicion) {
        ++pruebasOk;
        cout << "  [OK]    " << nombre << "\n";
    } else {
        ++pruebasFallidas;
        cout << "  [FALLA] " << nombre << "\n";
    }
}

bool casiIgual(float a, float b)
{
    return fabs(a - b) < TOLERANCIA;
}

// ------------------------------------------------------------------
// 1. Personaje (jugador)
// ------------------------------------------------------------------
void pruebasPersonaje()
{
    cout << "\n== Personaje ==\n";

    JugadorPrueba j(vector2(1.0f, 2.0f), 2.0f);
    verificar(casiIgual(j.posicion().x, 1.0f) && casiIgual(j.posicion().y, 2.0f),
              "El constructor guarda la posicion");
    verificar(casiIgual(j.velocidad().x, 0.0f) && casiIgual(j.velocidad().y, 0.0f),
              "La velocidad inicial es cero");
    verificar(casiIgual(j.masa(), 2.0f), "El constructor guarda la masa");

    // Sin fuerzas no se mueve
    j.actualizar(1.0f);
    verificar(casiIgual(j.posicion().x, 1.0f) && casiIgual(j.posicion().y, 2.0f),
              "Sin fuerzas el personaje no se mueve");

    // F = 4, m = 2  ->  a = 2.  Con dt = 1 (Euler semi-implicito):
    //   v = 0 + 2*1 = 2,   x = 1 + 2*1 = 3
    j.aplicarFuerza(vector2(4.0f, 0.0f));
    j.actualizar(1.0f);
    verificar(casiIgual(j.velocidad().x, 2.0f), "a = F/m: la velocidad sube a 2");
    verificar(casiIgual(j.posicion().x, 3.0f),
              "Euler semi-implicito: la posicion usa la velocidad nueva (x = 3)");

    // La fuerza se reinicia: en el siguiente paso sigue con v = 2 constante
    j.actualizar(1.0f);
    verificar(casiIgual(j.velocidad().x, 2.0f) && casiIgual(j.posicion().x, 5.0f),
              "La fuerza dura un solo fotograma (v se mantiene en 2)");

    // Varias fuerzas en el mismo fotograma se suman
    JugadorPrueba k(vector2(0.0f, 0.0f), 1.0f);
    k.aplicarFuerza(vector2(3.0f, 0.0f));
    k.aplicarFuerza(vector2(-1.0f, 5.0f));
    k.actualizar(1.0f);
    verificar(casiIgual(k.velocidad().x, 2.0f) && casiIgual(k.velocidad().y, 5.0f),
              "Las fuerzas del mismo fotograma se acumulan");

    // Cambiar la masa (poder "pesado") cambia la aceleracion
    JugadorPrueba p(vector2(0.0f, 0.0f), 1.0f);
    p.setMasa(4.0f);
    p.aplicarFuerza(vector2(8.0f, 0.0f));
    p.actualizar(1.0f);
    verificar(casiIgual(p.velocidad().x, 2.0f),
              "setMasa actualiza 1/masa (8 / 4 = 2)");

    // Setters de posicion y velocidad
    p.setPosicion(vector2(10.0f, 20.0f));
    p.setVelocidad(vector2(0.0f, 0.0f));
    verificar(casiIgual(p.posicion().x, 10.0f) && casiIgual(p.posicion().y, 20.0f),
              "setPosicion cambia la posicion");
}

// ------------------------------------------------------------------
// 2. Enemigo: cono de vision
// ------------------------------------------------------------------
void pruebasDeteccion()
{
    cout << "\n== Enemigo: detecta() ==\n";

    // En (0,0), mira hacia +x, radio 10, cono de 90 grados (45 a cada lado)
    EnemigoPrueba e(vector2(0.0f, 0.0f), 1.0f, 10.0f, 0.0f, PI / 2);

    verificar(e.detecta(vector2(5.0f, 0.0f)),   "Ve un objetivo justo al frente");
    verificar(e.detecta(vector2(5.0f, 4.9f)),   "Ve un objetivo a 44 grados (dentro)");
    verificar(!e.detecta(vector2(5.0f, 5.2f)),  "No ve un objetivo a 46 grados (fuera)");
    verificar(!e.detecta(vector2(11.0f, 0.0f)), "No ve un objetivo fuera del radio");
    verificar(e.detecta(vector2(10.0f, 0.0f)),  "Ve un objetivo justo en el borde del radio");
    verificar(!e.detecta(vector2(-5.0f, 0.0f)), "No ve un objetivo detras");
    verificar(e.detecta(vector2(0.0f, 0.0f)),   "Ve un objetivo encima de el");

    // Gira la vision hacia +y
    e.setAnguloVision(PI / 2);
    verificar(e.detecta(vector2(0.0f, 5.0f)),   "Al girar 90 grados ve hacia +y");
    verificar(!e.detecta(vector2(5.0f, 0.0f)),  "Al girar ya no ve hacia +x");

    // La deteccion es relativa a la posicion del enemigo
    EnemigoPrueba m(vector2(100.0f, 100.0f), 1.0f, 10.0f, 0.0f, PI / 2);
    verificar(m.detecta(vector2(105.0f, 100.0f)), "Funciona si el enemigo no esta en el origen");
    verificar(!m.detecta(vector2(5.0f, 0.0f)),    "No ve objetivos lejanos aunque esten 'al frente'");

    // Cono mayor de 180 grados (270): solo queda una zona ciega atras
    EnemigoPrueba a(vector2(0.0f, 0.0f), 1.0f, 10.0f, 0.0f, 1.5f * PI);
    verificar(a.detecta(vector2(0.0f, 5.0f)),   "Cono de 270: ve a los lados");
    verificar(a.detecta(vector2(-5.0f, 5.2f)),  "Cono de 270: ve atras fuera de la zona ciega");
    verificar(!a.detecta(vector2(-5.0f, 0.0f)), "Cono de 270: no ve en la zona ciega");

    // Cono de 360: ve todo dentro del radio
    EnemigoPrueba t(vector2(0.0f, 0.0f), 1.0f, 10.0f, 0.0f, 2 * PI);
    verificar(t.detecta(vector2(-5.0f, 0.0f)),  "Cono de 360: ve detras");

    // Cambiar el radio
    e.setRadioVision(3.0f);
    verificar(!e.detecta(vector2(0.0f, 5.0f)),  "setRadioVision reduce el alcance");
}

// ------------------------------------------------------------------
// 3. Enemigo hereda de Personaje
// ------------------------------------------------------------------
void pruebasHerencia()
{
    cout << "\n== Enemigo: herencia y polimorfismo ==\n";

    EnemigoPrueba e(vector2(0.0f, 0.0f), 2.0f, 10.0f, 0.0f, PI / 2);
    e.aplicarFuerza(vector2(0.0f, 4.0f));
    e.actualizar(1.0f);
    verificar(casiIgual(e.posicion().y, 2.0f), "El enemigo se mueve con la fisica de Personaje");

    // Igual que en Nivel: se guardan como Personaje* y se borran por la base
    Personaje* lista[2];
    lista[0] = new JugadorPrueba(vector2(0.0f, 0.0f), 1.0f);
    lista[1] = new EnemigoPrueba(vector2(0.0f, 0.0f), 1.0f, 5.0f, 0.0f, PI);
    for (int i = 0; i < 2; ++i) {
        lista[i]->aplicarFuerza(vector2(1.0f, 0.0f));
        lista[i]->actualizar(1.0f);   // llama a la version de cada clase hija
    }
    verificar(casiIgual(lista[0]->posicion().x, 1.0f) && casiIgual(lista[1]->posicion().x, 1.0f),
              "actualizar() funciona a traves de Personaje*");
    for (int i = 0; i < 2; ++i)
        delete lista[i];              // destructor virtual: borra bien la clase hija
    verificar(true, "delete a traves de Personaje* (destructor virtual)");
}

// ------------------------------------------------------------------
// 4. Excepciones
// ------------------------------------------------------------------
// Devuelve true si crear el objeto lanza Excepciones_de_Parametros_Fisicos
template <typename Funcion>
bool lanzaParametroFisico(Funcion crear, const string& nombre)
{
    try {
        crear();
    }
    catch (const Excepciones_de_Parametros_Fisicos& e) {
        cout << "          mensaje: " << e.what() << "\n";
        verificar(true, nombre);
        return true;
    }
    catch (...) {
        verificar(false, nombre + " (lanzo otro tipo de excepcion)");
        return false;
    }
    verificar(false, nombre + " (no lanzo nada)");
    return false;
}

void pruebasExcepciones()
{
    cout << "\n== Excepciones ==\n";

    lanzaParametroFisico([] { JugadorPrueba j(vector2(), 0.0f); },
                         "Masa 0 lanza Excepciones_de_Parametros_Fisicos");
    lanzaParametroFisico([] { JugadorPrueba j(vector2(), -5.0f); },
                         "Masa negativa lanza la excepcion");
    lanzaParametroFisico([] { JugadorPrueba j(vector2(), 1.0f); j.setMasa(0.0f); },
                         "setMasa(0) lanza la excepcion");
    lanzaParametroFisico([] { EnemigoPrueba e(vector2(), 1.0f, -3.0f, 0.0f, 1.0f); },
                         "Radio de vision negativo lanza la excepcion");
    lanzaParametroFisico([] { EnemigoPrueba e(vector2(), 1.0f, 3.0f, 0.0f, 7.0f); },
                         "Apertura mayor a 360 grados lanza la excepcion");
    lanzaParametroFisico([] { EnemigoPrueba e(vector2(), 1.0f, 3.0f, 0.0f, -1.0f); },
                         "Apertura negativa lanza la excepcion");

    // La excepcion guarda el dato que fallo
    try {
        JugadorPrueba j(vector2(), -2.0f);
    }
    catch (const Excepciones_de_Parametros_Fisicos& e) {
        verificar(e.parametro() == "masa" && casiIgual(e.valor(), -2.0f),
                  "La excepcion guarda el parametro y el valor");
    }

    // Se puede atrapar como la base Excepciones_del_Juego
    bool atrapadaPorBase = false;
    try {
        EnemigoPrueba e(vector2(), 1.0f, 0.0f, 0.0f, 1.0f);
    }
    catch (const Excepciones_del_Juego&) {
        atrapadaPorBase = true;
    }
    verificar(atrapadaPorBase, "Se atrapa con catch (const Excepciones_del_Juego&)");

    // Si setMasa falla, el personaje conserva la masa anterior
    JugadorPrueba j(vector2(), 3.0f);
    try { j.setMasa(-1.0f); } catch (const Excepciones_del_Juego&) {}
    verificar(casiIgual(j.masa(), 3.0f), "Si setMasa falla, la masa no cambia");

    // setAnguloVision acepta cualquier angulo (no lanza)
    bool lanzo = false;
    try {
        EnemigoPrueba e(vector2(), 1.0f, 3.0f, 0.0f, 1.0f);
        e.setAnguloVision(-100.0f);
        e.setAnguloVision(50.0f);
    }
    catch (...) {
        lanzo = true;
    }
    verificar(!lanzo, "setAnguloVision no lanza excepciones");
}

} // fin del namespace sin nombre

// La llama main_pruebas.cpp
void pruebasPersonajeEnemigo(int& correctas, int& totales)
{
    pruebasPersonaje();
    pruebasDeteccion();
    pruebasHerencia();
    pruebasExcepciones();

    correctas = pruebasOk;
    totales = pruebasOk + pruebasFallidas;
}