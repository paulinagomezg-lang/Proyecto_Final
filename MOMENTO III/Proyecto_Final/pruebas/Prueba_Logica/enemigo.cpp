#include "Enemigo.h"
#include "Excepciones_del_Juego.h"
#include <cmath>

using namespace std;

namespace {
const float DOS_PI = 6.28318530718f;
}

Enemigo::Enemigo(vector2 posicion, float masa, float radioVision, float anguloVision, float aperturaCono)
    : Personaje(posicion, masa),

    radioVision_(1.0f), anguloVision_(0.0f), aperturaCono_(1.0f),

    radioCuadrado_(1.0f), direccion_(1.0f, 0.0f), cosMediaApertura_(1.0f)
{
    // Los setters validan y precalculan.
    setRadioVision(radioVision);

    setAnguloVision(anguloVision);

    setAperturaCono(aperturaCono);
}

void Enemigo::setRadioVision(float radio)
{
    Excepciones_de_Parametros_Fisicos::exigirPositivo("radioVision", radio);

    radioVision_ = radio;

    radioCuadrado_ = radio * radio;
}

void Enemigo::setAnguloVision(float angulo)
{
    // Sin validación: cualquier ángulo es válido y no se lanza nada,
    // porque en el nivel 2 este setter se llama en cada fotograma.
    anguloVision_ = angulo;

    direccion_ = vector2(cos(angulo), sin(angulo));
}

void Enemigo::setAperturaCono(float apertura)
{
    Excepciones_de_Parametros_Fisicos::exigirEnRango("aperturaCono", apertura, 0.0f, DOS_PI);

    aperturaCono_ = apertura;

    cosMediaApertura_ = cos(apertura * 0.5f);
}

bool Enemigo::detecta(vector2 objetivo) const
{
    vector2 d = objetivo - posicion_;

    float d2 = d.normaCuadrada();

    // Paso a) Distancia: si está lejos, ni se revisa el ángulo.
    if (d2 > radioCuadrado_)

    return false;

    if (d2 == 0.0f)          // el objetivo está encima del enemigo

        return true;

    // Paso b) Ángulo. Se quiere saber si  punto >= |d| * c
    float punto = d.x * direccion_.x + d.y * direccion_.y;

    float c = cosMediaApertura_;

    if (c >= 0.0f) {
        // Cono de 180° o menos: el objetivo debe estar "al frente" (punto >= 0)
        // y, al elevar al cuadrado, punto² >= c² * d².
        return punto >= 0.0f && punto * punto >= c * c * d2;
    } else {
        // Cono de más de 180°: todo lo que está al frente se ve;
        // atrás solo se ve si no cae en la zona ciega.
        return punto >= 0.0f || punto * punto <= c * c * d2;
    }
}