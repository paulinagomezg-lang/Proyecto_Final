#include "GestorSonido.h"
#include "gestorlogico.h"
#include "Nivel.h"
#include "nivelbloques.h"
#include "itempoder.h"
#include "Protagonista.h"
#include "Excepciones_del_Juego.h"

#include <QFile>
#include <QUrl>
#include <QString>

using namespace std;

namespace {

// Rutas dentro de Recursos.qrc
const char* const RUTA_MUSICA   = ":/sonidos/musica_nivel1.wav";
const char* const RUTA_SALTO    = ":/sonidos/salto.wav";
const char* const RUTA_ITEM     = ":/sonidos/item.wav";
const char* const RUTA_CAPTURA  = ":/sonidos/captura.wav";
const char* const RUTA_VICTORIA = ":/sonidos/victoria.wav";

int contarRecogidos(const NivelBloques& nivel)
{
    int n = 0;
    for (const ItemPoder& item : nivel.items())
        if (item.recogido()) ++n;
    return n;
}

} // namespace

GestorSonido::GestorSonido()
    : nivelAnterior_(nullptr),
    tiempoAnterior_(0.0f),
    estadoAnterior_(static_cast<int>(EstadoJuego::MENU)),
    apoyoAnterior_(-1),
    itemsRecogidos_(0)
{
}

void GestorSonido::prepararEfecto(QSoundEffect& efecto, const char* ruta, float volumen)
{
    if (!QFile::exists(QString::fromUtf8(ruta)))
        throw Excepciones_de_Recursos(ruta, "no está en Recursos.qrc");
    // Los recursos de Qt se abren como URL "qrc:/..." (sin los dos puntos).
    efecto.setSource(QUrl(QString("qrc") + QString::fromUtf8(ruta)));
    efecto.setVolume(volumen);
}

void GestorSonido::cargar()
{
    prepararEfecto(musica_,   RUTA_MUSICA,   0.35f);
    prepararEfecto(salto_,    RUTA_SALTO,    0.7f);
    prepararEfecto(item_,     RUTA_ITEM,     0.8f);
    prepararEfecto(captura_,  RUTA_CAPTURA,  0.8f);
    prepararEfecto(victoria_, RUTA_VICTORIA, 0.8f);
    musica_.setLoopCount(QSoundEffect::Infinite);
}

// Cuando empieza un nivel (o se reinicia), lo anterior ya no sirve.
void GestorSonido::reiniciarSeguimiento(const Nivel* nivel)
{
    nivelAnterior_ = nivel;
    tiempoAnterior_ = nivel != nullptr ? nivel->tiempoTranscurrido() : 0.0f;
    apoyoAnterior_ = -1;
    itemsRecogidos_ = 0;
    if (const NivelBloques* bloques = dynamic_cast<const NivelBloques*>(nivel)) {
        apoyoAnterior_ = bloques->indiceBloqueApoyo();
        itemsRecogidos_ = contarRecogidos(*bloques);
    }
}

void GestorSonido::actualizar(const gestorlogico& juego)
{
    const EstadoJuego estado = juego.estado();
    const Nivel* nivel = juego.nivelactual();

    // Un nivel nuevo puede quedar en la misma dirección de memoria que el
    // que se borró, así que también se mira si el tiempo volvió a empezar.
    if (nivel != nivelAnterior_ ||
        (nivel != nullptr && nivel->tiempoTranscurrido() < tiempoAnterior_))
        reiniciarSeguimiento(nivel);

    // --- Efectos del nivel 1 ---
    const NivelBloques* bloques = dynamic_cast<const NivelBloques*>(nivel);
    if (bloques != nullptr) {
        const int apoyo = bloques->indiceBloqueApoyo();
        if (apoyoAnterior_ >= 0 && apoyo < 0 && bloques->protagonista()->velocidad().y > 0.0f)
            salto_.play();                       // dejó el bloque subiendo: saltó

        const int recogidos = contarRecogidos(*bloques);
        if (recogidos > itemsRecogidos_)
            item_.play();
        apoyoAnterior_ = apoyo;
        itemsRecogidos_ = recogidos;
    }

    // --- Efectos de los dos niveles, al cambiar de estado ---
    if (static_cast<int>(estado) != estadoAnterior_) {
        if (estado == EstadoJuego::VIDA_PERDIDA || estado == EstadoJuego::DERROTA)
            captura_.play();
        else if (estado == EstadoJuego::NIVEL_SUPERADO || estado == EstadoJuego::VICTORIA)
            victoria_.play();
        estadoAnterior_ = static_cast<int>(estado);
    }

    // --- Música del nivel 1 ---
    const bool conMusica = bloques != nullptr && estado == EstadoJuego::JUGANDO;
    if (conMusica && !musica_.isPlaying())
        musica_.play();
    else if (!conMusica && musica_.isPlaying())
        musica_.stop();

    if (nivel != nullptr) tiempoAnterior_ = nivel->tiempoTranscurrido();
}

