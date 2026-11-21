#include "gestorlogico.h"
#include "Nivel.h"
#include "Excepciones_del_Juego.h"
using namespace std;

gestorlogico::gestorlogico(Fabricanivel fabrica, int totalniveles):fabrica_(fabrica),totalniveles_(totalniveles),indiceactual_(0),vidas_(VIDAS_INICIALES),nivelactual_(nullptr),estado_(EstadoJuego::MENU),dificultad_(NivelDificultad::NORMAL){
    if(fabrica_ == nullptr)
        throw Excepciones_del_Juego("GestorLogico: La fabrica de niveles no puede ser nula");
    if(totalniveles_ < 1)
        throw Excepciones_del_Juego("GestorLogico: Debe haber a lo menos un nivel");
}

gestorlogico::~gestorlogico(){
    liberarNivel();
}

//Memoria

void gestorlogico::liberarNivel(){
    delete nivelactual_; //delete de null, no hace nada y no hace falta revisar
    nivelactual_ = nullptr; //evita puntero colgante
}

//carga un nivel liberando primero el anterior
//Se libera antes de crear el nuevo para que nunca haya dos niveles en la memoria al mismo tiempo
//no hay fuga, no queda puntero invalido(nivelactual es null) y juego vuelve al menu y excepciones sube a la interfaz

void gestorlogico::cargarNivel(int indice){
    liberarNivel();
    estado_ = EstadoJuego::MENU; //estado seguro donde se mientras se crea nivel
    Nivel* nuevo = fabrica_(indice, dificultad_);
    if(nuevo == nullptr)
        throw Excepciones_del_Juego("GestorLogico: La fabrica devolvio un nivel nulo");

    nivelactual_ = nuevo;
    indiceactual_ = indice;
    estado_ = EstadoJuego::JUGANDO;
}

//flujo de la partida

void gestorlogico::iniciar(){
    vidas_ = VIDAS_INICIALES;
    cargarNivel(0);
}

//se llama una vez por fotograma desde el qtimer de la interfaz: es el unico lugar donde corre la logica del juego asi se mantiene lo mas corto posible
//un corte dt, una llamda virtual y dos consultas (no reserva memoria ni lanza excepciones)

void gestorlogico::actualizar(float dt){
    if(estado_ != EstadoJuego::JUGANDO || nivelactual_ == nullptr)return;
    if(!(dt > 0.0f)) return; //tambien descarta NaN
    if(dt > DT_MAXIMO) dt = DT_MAXIMO;

    nivelactual_ -> actualizar(dt);

    //la derrota se revisa primero : si en el mismo fotograma mario llega a la meta y lo capturan, cuenta como capturado

    if(nivelactual_ -> condicionDerrota()){
        --vidas_;
        estado_ = (vidas_ > 0) ? EstadoJuego::VIDA_PERDIDA : EstadoJuego::DERROTA;
    } else if(nivelactual_ -> condicionVictoria()){
        const bool esUltimo = (indiceactual_ + 1 >= totalniveles_);
        estado_ = esUltimo ? EstadoJuego::VICTORIA : EstadoJuego::NIVEL_SUPERADO;
    }
}

void gestorlogico::pausar(){
    if(estado_ == EstadoJuego::JUGANDO) estado_ = EstadoJuego::PAUSA;
}

void gestorlogico::reanudar(){
    if(estado_ == EstadoJuego::PAUSA) estado_ = EstadoJuego::JUGANDO;
}

void gestorlogico::avanzarNivel(){
    if(estado_ != EstadoJuego::NIVEL_SUPERADO) return;
    cargarNivel(indiceactual_ +1);
}

void gestorlogico::reiniciarNivel(){
    //se puede reiniciar tras perder una vida desde la pausa o jugando, reiniciar no cuesta vida y tras derrota o victoria la partida reinicia
    if(estado_ != EstadoJuego::VIDA_PERDIDA && estado_ != EstadoJuego::PAUSA && estado_ != EstadoJuego::JUGANDO) return;
    cargarNivel(indiceactual_);
}

void gestorlogico::volverALMenu(){
    liberarNivel();
    indiceactual_ = 0;
    vidas_ = VIDAS_INICIALES;
    estado_ = EstadoJuego::MENU;
}

bool gestorlogico::setdificultad(NivelDificultad dificultad){
    //cambia la dificultad a mitad de un nivel dejaria a los minions con parametros mezclados; solo se permite desde el menu
    if(estado_ != EstadoJuego::MENU) return false;
    dificultad_ = dificultad;
    return true;
}

