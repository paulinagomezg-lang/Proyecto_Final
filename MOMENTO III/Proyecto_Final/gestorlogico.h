#ifndef GESTORLOGICO_H
#define GESTORLOGICO_H
#include "NivelDificultad.h"

//aqui solo se usan punteros a nivel, asi no hace falta incluir nivel.h eso reduce el tiempo de copilacion y que cualquier cambio en nivel obligue recopilar a quie use gestorlogico

class Nivel;

//estado de partida los consulta para decidir que pantalla mostrar
enum class EstadoJuego{
    MENU, //menu principal
    JUGANDO, //nivel actual se esta autorizando
    PAUSA, //el nivel existe pero no se actualiza
    NIVEL_SUPERADO, //se gano un nivel y pasa al siguiente
    VICTORIA, //e gano el ultimo nivel
    VIDA_PERDIDA, //capturaron a mario, pero le quedan vida
    DERROTA //no le quedan vidas
};

//GESTOR LOGICO

class gestorlogico{
public:

    //crea el nivel
    using Fabricanivel = Nivel* (*)(int indice, NivelDificultad dificultad);

    //paso de tiempo por fotograma (50ms)el dt al siguiente llega enorme y mario atravesa los bloques
    static constexpr float DT_MAXIMO = 0.05f;

    //vidas con las que empieza cada partida
    static constexpr int VIDAS_INICIALES = 3;

    //lanza excepciones del juego si la fabrica es nula o no hay niveles
    gestorlogico(Fabricanivel fabrica, int totalniveles);
    ~gestorlogico();

    gestorlogico(const gestorlogico&) = delete;
    gestorlogico& operator = (const gestorlogico&) = delete;

    //flujo de la partida
    void iniciar(); //partida nuev, 3 vidas y primer nivel
    void iniciarEn(int indice); //partida nueva desde el nivel elegido en el menu (lanza si no existe)
    void actualizar(float dt);
    void pausar();
    void reanudar();
    void avanzarNivel(); //solo desde nivel superado
    void reiniciarNivel(); //vuelve a crear nivel actual desde perder vida o pausa
    void volverALMenu(); //libera nivel y vuelve a menu
    bool setdificultad(NivelDificultad dificultad); // solo se puede cambiar desde el menu

    //CONSULTAS QUE USA INTERFAZ
    EstadoJuego estado() const{
        return estado_;
    }
    int indicenivelactual() const{
        return indiceactual_;
    }
    int totalniveles() const{
        return totalniveles_;
    }
    int vidas() const{
        return vidas_;
    }
    NivelDificultad dificultad() const{
        return dificultad_;
    }
    Nivel* nivelactual() const{
        return nivelactual_;
    }

private:

    void liberarNivel();
    void cargarNivel(int indice);

    Fabricanivel fabrica_;
    int totalniveles_;
    int indiceactual_;
    int vidas_;
    Nivel* nivelactual_;
    EstadoJuego estado_;
    NivelDificultad dificultad_;
};

#endif // GESTORLOGICO_H
