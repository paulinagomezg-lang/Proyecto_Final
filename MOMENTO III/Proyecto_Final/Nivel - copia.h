#ifndef NIVEL_H
#define NIVEL_H
#include <vector>
#include <cstddef>
using namespace std;

//Nivel solo guarda punteros en estas clases asi que no hay necesidad de los .h completos, copila mas rapido y no necesitara de protagonista
class Personaje;
class Protagonista;

class Nivel
{
public:
    Nivel();

    //Libera todos los personajes del nivel
    virtual ~Nivel();

    Nivel(const Nivel&) = delete;
    Nivel& operator = (const Nivel&) = delete;

    //lo que utilizamos en gestorlogico
    virtual void actualizar(float dt) = 0;
    virtual bool condicionVictoria() const = 0;
    virtual bool condicionDerrota() const = 0;

    //consultas, la utiliza el interfaz para dibujar
    const vector<Personaje*>& personajes() const{
        return personajes_;
    }
    Protagonista* protagonista() const{
        return protagonista_;
    }
    float tiempoTranscurrido() const{
        return tiempoTranscurrido_;
    }

protected:
    //reserva espacio para cantidad personajes y se llama una vez
    void reservarPersonajes(size_t cantidad); //constructor del hijo
    void agregarPersonaje(Personaje* personaje); //el nivel pasa a ser dueño de personaje (se libera en su destructor)
    void actualizarPersonajes(float dt); //movimiento con las fuerzas que el nivel aplico
    void avanzarTiempo(float dt){
        tiempoTranscurrido_ += dt;
    }

    //apunta a mario que esta dentro de personajes_ , no es dueño osea que no se le palica el delete
    Protagonista* protagonista_;

private:
    vector<Personaje*>personajes_;
    float tiempoTranscurrido_;

};

#endif // NIVEL_H
