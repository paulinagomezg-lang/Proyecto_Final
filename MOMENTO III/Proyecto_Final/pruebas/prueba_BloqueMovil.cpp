#include "BloqueMovil.h"
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
bool casi(float a, float b, float tol = 1e-4f) { return fabs(a - b) < tol; }

const float DT = 1.0f / 60.0f;
const float K = 17.5f;          // la misma constante del nivel (N/m)

// Un paso como lo hara NivelLaberinto: fuerza del resorte e integrar.
void paso(BloqueMovil& b, float k = K)
{
    b.aplicarFuerza(b.eje() * (-k * b.desplazamiento()));
    b.actualizar(DT);
}

// Mide el periodo contando cuantas veces cruza el centro.
float medirPeriodo(BloqueMovil& b, float segundos)
{
    float anterior = b.desplazamiento(), tiempo = 0.0f;
    float primero = -1.0f, ultimo = -1.0f;
    int cruces = 0;
    const int pasos = static_cast<int>(segundos * 60.0f);
    for (int i = 0; i < pasos; ++i) {
        paso(b);
        tiempo += DT;
        const float s = b.desplazamiento();
        if ((anterior > 0.0f) != (s > 0.0f)) {
            if (primero < 0.0f) primero = tiempo;
            ultimo = tiempo;
            ++cruces;
        }
        anterior = s;
    }
    // Entre dos cruces del centro pasa medio periodo.
    return cruces > 1 ? 2.0f * (ultimo - primero) / static_cast<float>(cruces - 1) : 0.0f;
}

// Para comprobar que se libera bien a traves de Personaje*.
int vivos = 0;
class BloqueContado : public BloqueMovil
{
public:
    BloqueContado() : BloqueMovil(vector2(), vector2(1, 0), 1.0f, 1.0f) { ++vivos; }
    ~BloqueContado() override { --vivos; }
};

void ejecutar(int& correctas, int& totales)
{
    printf("Construccion\n");
    {
        BloqueMovil b(vector2(4.5f, 2.5f), vector2(1, 0), 3.0f, 4.0f);
        verificar(b.posicion() == vector2(7.5f, 2.5f), "empieza en un extremo: centro + eje * amplitud");
        verificar(b.velocidad() == vector2(0, 0), "empieza quieto");
        verificar(casi(b.desplazamiento(), 3.0f), "su desplazamiento inicial es la amplitud");
        verificar(b.centro() == vector2(4.5f, 2.5f) && casi(b.amplitud(), 3.0f) && casi(b.masa(), 4.0f),
                  "guarda el centro, la amplitud y la masa");

        BloqueMovil n(vector2(), vector2(5, 0), 1.0f, 1.0f);
        verificar(n.eje() == vector2(1, 0), "el eje se normaliza: (5,0) -> (1,0)");

        BloqueMovil v(vector2(2, 2), vector2(0, 1), 2.0f, 1.0f);
        verificar(v.posicion() == vector2(2, 4) && v.eje() == vector2(0, 1), "tambien puede ir en vertical");

        BloqueMovil quieto(vector2(1, 1), vector2(1, 0), 0.0f, 1.0f);
        verificar(quieto.posicion() == vector2(1, 1), "con amplitud 0 queda quieto en el centro");
    }

    printf("Validaciones (excepciones)\n");
    {
        bool lanzo = false;
        try { BloqueMovil b(vector2(), vector2(1, 0), 1.0f, 0.0f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "masa 0 lanza excepcion");
        lanzo = false;
        try { BloqueMovil b(vector2(), vector2(1, 0), -1.0f, 1.0f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "amplitud negativa lanza excepcion");
        lanzo = false;
        try { BloqueMovil b(vector2(), vector2(0, 0), 1.0f, 1.0f); } catch (const Excepciones_de_Parametros_Fisicos&) { lanzo = true; }
        verificar(lanzo, "eje (0,0) lanza excepcion (no tendria direccion)");
        lanzo = false;
        try { BloqueMovil b(vector2(), vector2(0, 0), 1.0f, 1.0f); } catch (const Excepciones_del_Juego&) { lanzo = true; }
        verificar(lanzo, "se puede atrapar como Excepciones_del_Juego");
    }

    printf("Movimiento con el resorte del nivel (F = -k x)\n");
    {
        BloqueMovil b(vector2(4.5f, 2.5f), vector2(1, 0), 3.0f, 4.0f);
        const float T = 6.28318530718f * sqrt(4.0f / K);       // 2*pi*sqrt(m/k)
        const float medido = medirPeriodo(b, 12.0f);
        printf("     periodo medido %.3f s, teorico %.3f s\n", medido, T);
        verificar(casi(medido, T, 0.02f * T), "el periodo es 2*pi*sqrt(m/k) (unos 3 s)");

        BloqueMovil c(vector2(4.5f, 2.5f), vector2(1, 0), 3.0f, 4.0f);
        float maximo = 0.0f;
        bool enRiel = true;
        for (int i = 0; i < 60 * 30; ++i) {          // 30 s
            paso(c);
            if (fabs(c.desplazamiento()) > maximo) maximo = fabs(c.desplazamiento());
            if (!casi(c.posicion().y, 2.5f)) enRiel = false;
        }
        verificar(maximo <= 3.0f * 1.01f && maximo >= 3.0f * 0.99f,
                  "en 30 s la amplitud se conserva (no gana ni pierde energia)");
        verificar(enRiel, "solo se mueve sobre su eje (y no cambia)");

        BloqueMovil liviano(vector2(), vector2(1, 0), 1.0f, 1.0f);
        BloqueMovil pesado(vector2(), vector2(1, 0), 1.0f, 4.0f);
        const float tL = medirPeriodo(liviano, 12.0f);
        const float tP = medirPeriodo(pesado, 12.0f);
        verificar(casi(tP / tL, 2.0f, 0.05f), "con 4 veces la masa tarda el doble (T crece con raiz de m)");

        BloqueMovil sinFuerza(vector2(), vector2(1, 0), 2.0f, 1.0f);
        for (int i = 0; i < 60; ++i) sinFuerza.actualizar(DT);
        verificar(sinFuerza.posicion() == vector2(2, 0), "sin la fuerza del nivel no se mueve (la fisica esta en el nivel)");
    }

    printf("ajustarAlRiel (despues de un choque)\n");
    {
        BloqueMovil b(vector2(4, 2), vector2(1, 0), 3.0f, 4.0f);
        b.setPosicion(vector2(5, 2.4f));                // un golpe lo saco de lado
        b.setVelocidad(vector2(1.5f, 2.0f));
        b.ajustarAlRiel();
        verificar(b.posicion() == vector2(5, 2), "vuelve a su riel (se quita lo que se movio de lado)");
        verificar(b.velocidad() == vector2(1.5f, 0), "se quita la velocidad de lado y se conserva la del riel");

        b.setPosicion(vector2(9, 2));                   // 5 m del centro: mas que la amplitud
        b.setVelocidad(vector2(2, 0));                  // y siguiendo hacia afuera
        b.ajustarAlRiel();
        verificar(casi(b.desplazamiento(), 3.0f), "no pasa del extremo del riel (asi no entra en una pared)");
        verificar(b.velocidad() == vector2(0, 0), "en el extremo se anula la velocidad hacia afuera");

        b.setPosicion(vector2(9, 2));
        b.setVelocidad(vector2(-2, 0));                 // ya va de regreso
        b.ajustarAlRiel();
        verificar(b.velocidad() == vector2(-2, 0), "si ya va de regreso conserva su velocidad");

        b.setPosicion(vector2(-3, 2));                  // del otro lado
        b.setVelocidad(vector2(-1, 0));
        b.ajustarAlRiel();
        verificar(casi(b.desplazamiento(), -3.0f) && b.velocidad() == vector2(0, 0),
                  "lo mismo en el extremo del otro lado");

        BloqueMovil quieto(vector2(1, 1), vector2(1, 0), 0.0f, 1.0f);
        quieto.setPosicion(vector2(2, 1.5f));
        quieto.setVelocidad(vector2(3, 3));
        quieto.ajustarAlRiel();
        verificar(quieto.posicion() == vector2(1, 1) && quieto.velocidad() == vector2(0, 0),
                  "un bloque de amplitud 0 no se mueve aunque lo golpeen");
    }

    printf("Memoria\n");
    {
        Personaje* p = new BloqueContado();
        verificar(vivos == 1, "se crea como cualquier personaje");
        delete p;
        verificar(vivos == 0, "delete por Personaje* lo libera (destructor virtual, como hace Nivel)");
    }

    correctas = total - fallas;
    totales = total;
}

}

void pruebasBloqueMovil(int& correctas, int& totales)
{
    ejecutar(correctas, totales);
}