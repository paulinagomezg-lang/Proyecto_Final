#include "Energiacarrera.h"
#include "Excepciones_del_Juego.h"
using namespace std;

energiacarrera::energiacarrera(float maxima,float consumoPorSegundo,float recuperacionPorSegundo,float umbralCansancio,float factorMinimo)
    : energia_(maxima),maxima_(maxima),consumo_(consumoPorSegundo),recuperacion_(recuperacionPorSegundo),umbral_(umbralCansancio),factorMinimo_(factorMinimo),
    inversoMaxima_(0.0f),inversoUmbral_(0.0f),factor_(1.0f),agotada_(false){


    Excepciones_de_Parametros_Fisicos::exigirPositivo("energiaMaxima", maxima);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("consumoPorSegundo", consumoPorSegundo);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("recuperacionPorSegundo", recuperacionPorSegundo);
    Excepciones_de_Parametros_Fisicos::exigirEnRango("umbralCansancio", umbralCansancio, 0.0f, maxima);
    Excepciones_de_Parametros_Fisicos::exigirEnRango("factorMinimo", factorMinimo, 0.0f, 1.0f);
    inversoMaxima_ = 1.0f / maxima_;

    //con umbral 0nunca se cansa antes de agotarse, el inverso no se usa
    inversoUmbral_ = (umbral_ > 0.0f) ? 1.0f / umbral_ : 0.0f;
}

//valores de cada dificultad , el switch se evalua una sola vez al crear a Mario
energiacarrera energiacarrera::segunDificultad(NivelDificultad dificultad){

    //maxima gasta recupera umbral factormin
    switch (dificultad) {
    case NivelDificultad::FACIL:
        return energiacarrera(100.0f, 18.0f, 20.0f,  25.0f, 0.5f);
    case NivelDificultad::NORMAL:
        return energiacarrera(100.0f, 25.0f, 15.0f,  30.0f, 0.4f);
    case NivelDificultad::DIFICIL:
        return energiacarrera(100.0f, 35.0f, 10.0f,  40.0f, 0.3f);
    }
    return energiacarrera(); //por seguridad valores normales
}

void energiacarrera::actualizar(float dt, bool corriendo, float multiplicadorConsumo){
    if(!(dt > 0.0f)) return;
    if(corriendo && !agotada_){
        energia_ -= consumo_ *multiplicadorConsumo * dt;
        if(energia_ <= 0.0f){
            energia_ = 0.0f;
            agotada_ =true;
        }
    }else{
        energia_ += recuperacion_ * dt;
        if(energia_ > maxima_)energia_ = maxima_;
        //vuelve a correr solo cuando recupera el umbral
        if(agotada_ && energia_ >= umbral_) agotada_ = false;
    }
    recalcularFactor();
}

//por encima del umbral, factor 1 (velocidad normal). por debajo del umbral, interpolacion lineal entre factor minimo (energia 0).
//(energía = umbral) una multiplicación y una suma, sin divisiones

void energiacarrera::recalcularFactor(){
    if (energia_ >= umbral_){
        factor_ = 1.0f;
    }else{
        const float t = energia_ * inversoUmbral_; //entre 0 y 1
        factor_ = factorMinimo_ + (1.0f - factorMinimo_)*t;
    }
}

void energiacarrera::reiniciar(){
    energia_ = maxima_;
    agotada_ = false;
    factor_ = 1.0f;
}


