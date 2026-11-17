#ifndef ENERGIACARRERA_H
#define ENERGIACARRERA_H
#include "NivelDificultad.h"

class energiacarrera
{
public:
    explicit energiacarrera(float maxima = 100.0f, float consumoPorSegundo = 25.0f, float recuperacionPorSegundo = 15.0f, float umbralCansancio = 30.0f, float factorMinimo = 0.4f);

    //crea energia con los valores de dificultad elegidos por el menu
    //facil : gasta 18/s, recupera 20/s, se cansa bajo 25, minimo 50%
    //normal : gasta 25/s, recupera 15/s, se cansa bajo 30, minimo 40%

    static energiacarrera segunDificultad(NivelDificultad dificultad);

    //el jugador mantiene la tecla de correr y mario se mueve
    // multiplicador consumo: 1 normalmente, con poder pesado se pasa, masa pesada/ masa normal para gaste mas rapido
    void actualizar(float dt, bool corriendo, float multiplicadorConsumo = 1.0f);

    //1.0: velocidad normal baja hasta el factormin cuando esta agotado
    float factorVelocidad() const{
        return factor_;
    }
    float porcentaje() const{
        return energia_ * inversoMaxima_;
    }
    float energia() const{
        return energia_;
    }
    //true mientras no pueda correr por agotamiento
    bool agotada() const{
        return agotada_;
    }
    //mira si puede correr en ese momento antes de aplicar l avelocidad en el nivel
    bool puedeCorrer() const{
        return !agotada_;
    }
    //energia llena
    void reiniciar();

private:

    void recalcularFactor();
    float energia_;
    float maxima_;
    float consumo_;
    float recuperacion_;
    float umbral_;
    float factorMinimo_;
    float inversoMaxima_; //1 / maxima, calculado una vez
    float inversoUmbral_; //1 / umbral, calculado una vez
    float factor_; //factor velocidad actual, calculado en actualizar
    bool agotada_;
};

#endif // ENERGIACARRERA_H
