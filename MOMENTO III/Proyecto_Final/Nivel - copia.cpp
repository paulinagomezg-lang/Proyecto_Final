#include "Nivel.h"
#include "personaje.h"
#include "Excepciones_del_Juego.h"
#include <algorithm>
using namespace std;

Nivel::Nivel(): protagonista_(nullptr), tiempoTranscurrido_(0.0f){}

//libera todo los personajes se recorre del ultimo creado al primero liberado en orden de destructor.
//con delete se llama al destructor de la clase hiaj correcta

Nivel::~Nivel(){
    //recorre vector personaje en orden inverso de atras para adelante
    for (auto it = personajes_.rbegin(); it != personajes_.rend(); ++it)
        delete *it;

    personajes_.clear();
    protagonista_ = nullptr; //apuntaba a uno de los ya liberados
}

void Nivel::reservarPersonajes(size_t cantidad){
    personajes_.reserve(cantidad);
}

//solo se usa al cargar el nivel, por eso se valida y se puede lanzar excepciones

void Nivel::agregarPersonaje(Personaje* personaje){
    if(personaje == nullptr)
        throw Excepciones_del_Juego("Nivel: No se puede gregar un personaje nulo");


    //agregar el mismo puntero dos veces lo borraria doble
    if(find(personajes_.begin(), personajes_.end(), personaje) != personajes_.end())
        throw Excepciones_del_Juego("Nivel: El ppersonaje ya estaba agregado");

    try{
        personajes_.push_back(personaje); //agrega al final del vector y crec en tiempo de ejecucion
    } catch(...){ //Atrapa cualquier tipo de excepcion,sin que la aplicación colapse en tiempo de ejecución
        delete personaje; // se libera para qeu no quede fuga
        throw;
    }
}

// Actualiza uno por uno a los personajes en cada fotograma usando polimorfismo(adoptar diferentes comportamientos o formas),no reserva memoria ni lanza excepciones

void Nivel::actualizarPersonajes(float dt){
    for (Personaje* p : personajes_) {
        p ->actualizar(dt);
    }
}
