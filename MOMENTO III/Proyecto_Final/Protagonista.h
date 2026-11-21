#ifndef PROTAGONISTA_H
#define PROTAGONISTA_H

#include "personaje.h"
#include "Energiacarrera.h"
#include "NivelDificultad.h"

//el estado pesado de mario no se encuentra aca ya que e sun poder que se puede tener en el mismo tiempo de cualquier estado
//es un bool pesado

enum class EstadoProtagonista{
    NORMAL, //se puede mover
    DORMIDO, //se durmio or quedarse quieto, no se mueve
    DESPERTANDO, //tiempo extra para despertar
    CAPTURADO //pierde porque lo cogio un minion
};

enum class Mirada{
    DERECHA,
    IZQUIERDA,
    ARRIBA, //de espaldas
    ABAJO //de frente
};

class Protagonista : public Personaje
{

public:
    //masanormal: masa sin el poder (kg)
    //factor pesado: cuantas veces aumenta la masa con el poder
    //impulsosalto: fuerza del salto (N*s) con el poder salta menos vo =J7m
    //lentitudpesado: velocidad con el poder, como fraccion de la normal

    Protagonista(vector2 posicion, NivelDificultad dificultad, float masaNormal = 1.0f, float factorPesado = 3.0f, float impulsoSalto = 6.0f, float lentitudPesado = 0.6f);

    //entrada al juego la llama cuandola interfaz lee el teclado
    //direccion (-1..1, -1..1) se normaliza para qur no vaya en diagonal
    //mas rapido para correr dejan oprimida la tecla que lo hace correr

    void setEntrada(vector2 direccion, bool correr);
    void solicitarSalto();

    //el nivel la llama al aplicar el salto . un pedido dura un solo fotograma como en el nivel 2 el salto se descarta al final de actulizar()

    bool consumirSalto();

    //lo que el nivel usa para calcular el movimiento , direccion que relmente aplica  (0,0) si esta dormido o capturado
    vector2 direccionEfectiva() const{
        return puedeMoverse() ? direccion_ : vector2();
    }

    bool estaCorriendo() const{
        return corriendo_;
    }

     //segundos seguidos sinq eu el jugador lo mueva cuenta en los dos niveles con sus respectivas asignaciones de poder
    float tiempoQuieto() const{
        return tiempoQuieto_;
    }

    //hacia donde mira, para que la interfaz elija los sprites
    Mirada mirada() const{
        return mirada_;
    }

    //nivel 1: false(solo derecha/ izquierda).
    //nivel 2: la pone en true para que tambien mire hacia arriba y hacia abajo
    void usarMiradaCuatroDirecciones(bool usar);

    //factor por el que el nivel multiplica la velocidad de cansancio y lentitud de paso
    float factorVelocidad() const{
        return energia_.factorVelocidad() * (esPesado_ ? lentitudPesado_ : 1.0f);
    }

    float impulsoSalto() const{
        return impulsoSalto_;
    }

    //poder pesado: nivel 1 se activa comiendo el item y dependiendo la dificultad dura el efecto
    void activarPoderPesado();
    void desactivarPoderPesado(); //por si un nivel lo necesita quitar antes
    bool esPesado() const{
        return esPesado_;
    }
    //segundos que le quedan pesado (0 sin el poder) el interfaz puede mostrarlo en barra o un contador
    float tiempoPesadoRestante() const{
        return tiempoPesadoRestante_;
    }
    float duracionPesado() const{
        return duracionPesado_;
    }
    float proporcionMasa() const{
        return esPesado_ ? factorPesado_:1.0f; // masa actual/ masa normal con 1 sin poder , factor pesado sin poder
    }

    //caracteristica dormilon: nivel 2 donde solo este nivel la activa
    void habilitarSueno(bool habilitado);
    bool estaDormido() const; //dormido o despierto es vunerable
    float tiempoParaDormir() const{
        return tiempoParaDormir_;
    }

    //Captura
    void capturar();
    bool estaCapturado() const{
        return estado_ == EstadoProtagonista::CAPTURADO;
    }

    //consultas que utiliza la interfaz
    EstadoProtagonista estado() const{
        return estado_;
    }
    bool puedeMoverse() const{
        return estado_ == EstadoProtagonista::NORMAL;
    }
    const energiacarrera& energia() const{
        return energia_;
    }

    //actualizar sueño y energia y luego integra el movimiento con las fuerzas qu el nivel aplico en este fotograma
    void actualizar(float dt) override; //override es una palabra clave que se coloca al final de un método en una clase hija para indicar que está sobrescribiendo reemplazando un método virtual heredado de su clase padre

private:
    void actualizarSueno(float dt);

    //poder pesado
    float masaNormal_;
    float factorPesado_;
    float impulsoSalto_;
    float lentitudPesado_;
    float duracionPesado_; //segun la dificultad
    float tiempoPesadoRestante_;
    bool esPesado_;

    //entrada
    vector2 direccion_; // ya normalizada
    bool quiereCorrer_;
    bool saltoPedido_;
    bool corriendo_; //calculado en actualizar()
    Mirada mirada_;
    bool miradaCuatroDirecciones_;

    //dormilon
    EstadoProtagonista estado_;
    bool suenoHabilitado_;
    float tiempoQuieto_;
    float tiempoEnEstado_;
    float tiempoParaDormir_;
    float duracionSueno_;
    float duracionDespertar_;

    energiacarrera energia_;

};

#endif // PROTAGONISTA_H
