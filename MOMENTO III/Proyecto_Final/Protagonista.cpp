#include "Protagonista.h"
#include "Excepciones_del_Juego.h"
using namespace std;


//los tiempo de dormilon segun la dificultad va en segundos( se elige unicamente para Mario)
//tiempo para dormir: cuanto puede quedarse quieto
//duracion sueno: cuanto se duerme
//dutacion despertar: tiempo que se tarda mario para poder seguir jugando

namespace{

struct TiempoSueno{
    float paraDormir;
    float sueno;
    float despertar;
};

//cuanto dura el poder de pesado segun la difi donde perjudica a Mario

float duracionPesadoSegunDificultad(NivelDificultad dificultad){
    switch (dificultad) {
    case NivelDificultad::FACIL: return 10.0f;
    case NivelDificultad::NORMAL: return 15.0f;
    case NivelDificultad::DIFICIL: return 20.0f;
    }
    return 15.0f;
}



TiempoSueno tiempoSegunDificultad(NivelDificultad dificultad){
    switch (dificultad){
    case NivelDificultad::FACIL: return{5.0f, 1.5f, 0.5f}; //2s sin poder moverse
    case NivelDificultad::NORMAL: return{3.5f,2.0f,1.0f}; //3s sin poder moverse
    case NivelDificultad::DIFICIL: return{2.5f,4.0f,2.0f}; //6s sin poder moverse
        }
    return {3.5f,2.0f,1.0f}; //retornamos el valor normal por si hay alguna falla tenga un valor de respaldo
}
}

Protagonista::Protagonista(vector2 posicion, NivelDificultad dificultad, float masaNormal, float factorPesado, float impulsoSalto, float lentitudPesado) : Personaje(posicion, masaNormal), masaNormal_(masaNormal), factorPesado_(factorPesado), impulsoSalto_(impulsoSalto), lentitudPesado_(lentitudPesado),
    duracionPesado_(duracionPesadoSegunDificultad(dificultad)), tiempoPesadoRestante_(0.0f), esPesado_(false), direccion_(0.0f , 0.0f), quiereCorrer_(false), saltoPedido_(false), corriendo_(false), mirada_(Mirada::DERECHA), miradaCuatroDirecciones_(false), estado_(EstadoProtagonista::NORMAL), suenoHabilitado_(false),
    tiempoQuieto_(0.0f), tiempoEnEstado_(0.0f), tiempoParaDormir_(0.0f), duracionSueno_(0.0f), duracionDespertar_(0.0f), energia_(energiacarrera::segunDificultad(dificultad)){


    Excepciones_de_Parametros_Fisicos::exigirEnRango("factorPesado", factorPesado, 1.0f, 100.0f); //del minimo al maximo
    Excepciones_de_Parametros_Fisicos::exigirPositivo("impulsoSalto", impulsoSalto);
    Excepciones_de_Parametros_Fisicos::exigirPositivo("lentitudPesado", lentitudPesado);
    Excepciones_de_Parametros_Fisicos::exigirEnRango("lentitudPesado", lentitudPesado, 0.0f, 1.0f);//estrictamente mayor que 0.0% y menor o igual a 1.0%

    const TiempoSueno t = tiempoSegunDificultad(dificultad); //funcion auxiliar que calcula y decuelve los tres tiempos
    tiempoParaDormir_ = t.paraDormir; //desempacan de la caja TiempoSueno los datos de t
    duracionSueno_ = t.sueno;
    duracionDespertar_ = t.despertar;
}

//Entrada: se llama solo cuando el jugaro presiona la tecla , as que noramlizarla con una raiz no cuesta nada en el ciclo del juego

void Protagonista::setEntrada(vector2 direccion, bool correr){
    direccion_ = direccion.normalizado(); //0.0 se queda en 0.0
    quiereCorrer_ = correr;
    //si se queda quieto conserva la ultima mirada mas no se mueve solo
    const float ax = direccion_.x < 0.0f ? -direccion_.x : direccion_.x; //|x|
    const float ay = direccion_.y < 0.0f ? -direccion_.y : direccion_.y; //|y|

    if (miradaCuatroDirecciones_ && ay>ax){
        //el laberinto manda el eje en el que se mueve, en diagonal es ax==ay  gana el horizontal para que no parpadee
        mirada_ = direccion_.y > 0.0f ? Mirada::ARRIBA : Mirada::ABAJO;
    } else if (direccion_.x > 0.0f){
        mirada_ = Mirada::DERECHA;
    }else if (direccion_.x < 0.0f){
        mirada_ = Mirada::IZQUIERDA;
    }
    // Nivel 1 moviendose solo en vertical o quieto no cambia
}

//Al entrar al laberinto, Mario aparece de frente, al volver a dos direcciones si estaba mirando arriba o abajo pasa a mirar a la derecha

void Protagonista::usarMiradaCuatroDirecciones(bool usar){
    miradaCuatroDirecciones_ = usar;
    if (usar)
        mirada_ = Mirada::ABAJO;
    else if (mirada_ == Mirada::ARRIBA || mirada_ == Mirada::ABAJO)
        mirada_ = Mirada::DERECHA;
}

void Protagonista::solicitarSalto(){
    saltoPedido_ = true;
}

bool Protagonista::consumirSalto(){
    const bool salta = saltoPedido_ && puedeMoverse();
    saltoPedido_ = false; //un pedido = un salto
    return salta;
}


//poder pesado: lo llama el nivel 1 donde mario se come un item. si ya estaba pesado solo se reinicia el tiempo , la masa se reinicia otra vez. el set masa no se llama repetidamente sino solo una vez

void Protagonista::activarPoderPesado(){
    tiempoPesadoRestante_ = duracionPesado_;
    if (esPesado_) return;
    esPesado_ = true;
    setMasa(masaNormal_ * factorPesado_);
}

void Protagonista::desactivarPoderPesado(){
    tiempoPesadoRestante_ = 0.0f;
    if (!esPesado_) return;
    esPesado_ = false;
    setMasa(masaNormal_);
}

//Dormilon

void Protagonista::habilitarSueno(bool habilitado){
    suenoHabilitado_ = habilitado;
    tiempoQuieto_ = 0.0f;
}

bool Protagonista::estaDormido() const{
    return estado_ == EstadoProtagonista::DORMIDO || estado_ == EstadoProtagonista::DESPERTANDO;
}

//NORMAL (quieto tiempoparadormir), DORMIDO (duracionsueno), DESPERTANDO (duraciondespertar).
//es un ciclo dpnde cada vez que se vuelve a quedarse quieto el tiempo completo se vuelve a dormir sin limite de vece. Mientras se duerme o se despierta las teclas no le pueden ckabiar el estado

void Protagonista::actualizarSueno(float dt) {
    // al cambiar el estado el tiempo que sobra del fotograma pasa al siguiente estado pero nunca se pone en 0 asi duran exactamente los tiempos sin importar fotogramas por segundos

    switch (estado_) {
    case EstadoProtagonista::NORMAL:
        if (tiempoQuieto_ >= tiempoParaDormir_){ //revisa si el tiempo que el personaje lleva sin mover  ya alcanzo el limite
            estado_ = EstadoProtagonista::DORMIDO;
            tiempoEnEstado_ = tiempoQuieto_ - tiempoParaDormir_;
            tiempoQuieto_ = 0.0f;
        }
        break;

    case EstadoProtagonista::DORMIDO:
        tiempoEnEstado_ += dt;
        if (tiempoEnEstado_ >= duracionSueno_){ //comprueba si el tiempo acumulado en este estado ya supero la duracion
            estado_ = EstadoProtagonista::DESPERTANDO;
            tiempoEnEstado_ -= duracionSueno_; //resta duracionSueno_ para conservar nuevamente los milisegundos excedentes
        }
        break;

    case EstadoProtagonista::DESPERTANDO:
        tiempoEnEstado_ += dt; //Suma el tiempo transcurrido del fotograma actual (dt) a tiempoEnEstado_
        if (tiempoEnEstado_ >= duracionDespertar_){
            estado_ = EstadoProtagonista::NORMAL;
            tiempoQuieto_ = tiempoEnEstado_ - duracionDespertar_;
            tiempoEnEstado_ = 0.0f;
        }
        break;

    case EstadoProtagonista::CAPTURADO:
        break;
    }
}

//Captura

void Protagonista::capturar(){
    estado_ = EstadoProtagonista::CAPTURADO;
    corriendo_ = false;
    saltoPedido_ = false;
}

//Cada fotograma: corre 60 veces por segunfo soo comparaciones y sumas, sin memoria dinamica y sin excepciones

void Protagonista::actualizar(float dt){
    if (!(dt > 0.0f)) return;

    const bool seMueve = direccion_.normaCuadrada() > 0.0f;

    //tiempo quieto: se cuenta los dos niveles (solon si se puede mover)
    if (estado_ == EstadoProtagonista::NORMAL){
        if (seMueve || saltoPedido_) tiempoQuieto_ = 0.0f;
        else
            tiempoQuieto_ += dt;
    }

    if (suenoHabilitado_)
        actualizarSueno(dt);

    //el poder pesado se acaba solocuando cumple su tiempo
    if (esPesado_){
        tiempoPesadoRestante_ -= dt;
        if (tiempoPesadoRestante_ <= 0.0f)
            desactivarPoderPesado();
    }

    //solo corre si quiere, se esta moviendo o puede moverse con la energia
    corriendo_ = quiereCorrer_&& seMueve && puedeMoverse() && energia_.puedeCorrer();

    //con el poder pesado gasta energia mas rapido (proporcional a la masa)
    energia_.actualizar(dt, corriendo_, proporcionMasa());

    //moviento con las fuerzas que el nivel aplico (euler semi-implicito)
    integrar(dt);

    //un pedido de salto dura solo fotograma porque sin esto en el laberintoun pedido quedaria guardado para siempre y mario nunca se dormiria
    saltoPedido_ = false;
}