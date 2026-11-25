#include "VistaJuego.h"
#include "FabricaNiveles.h"
#include "Nivel.h"
#include "NivelLaberinto.h"
#include "protagonista.h"
#include "Excepciones_del_Juego.h"

#include <QPainter>
#include <QKeyEvent>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QMessageBox>
#include <QFont>
#include <QColor>
#include <QPen>
#include <QString>
#include <exception>

using namespace std;

namespace {

const int MS_POR_FOTOGRAMA = 16;   // ~60 fotogramas por segundo
const int ALTO_HUD = 44;           // barra de arriba (vidas, energía...)
const int MARGEN = 8;

QString nombreDificultad(NivelDificultad d)
{
    switch (d) {
    case NivelDificultad::FACIL:   return "Fácil";
    case NivelDificultad::NORMAL:  return "Normal";
    case NivelDificultad::DIFICIL: return "Difícil";
    }
    return "Normal";
}

}


// Construcción

VistaJuego::VistaJuego(QWidget* parent)
    : QWidget(parent),
    juego_(crearNivel, totalNiveles()),
    arriba_(false),
    abajo_(false),
    izquierda_(false),
    derecha_(false),
    correr_(false)
{
    setFocusPolicy(Qt::StrongFocus);          // para recibir el teclado
    setMinimumSize(640, 480);

    temporizador_.setTimerType(Qt::PreciseTimer);
    connect(&temporizador_, &QTimer::timeout, this, &VistaJuego::tick);
    temporizador_.start(MS_POR_FOTOGRAMA);
    reloj_.start();
    relojTotal_.start();
}


//Llama a una acción de gestorlogico que puede lanzar una excepción crear un nivel lee el mapa, Las excepciones solo pueden salir al CARGAR, nunca en el ciclo de 60 fps.
//Si falla: se muestra el mensaje y el juego queda en el menú (gestorlogicoya liberó el nivel, sin fugas).

template <typename Accion>
void VistaJuego::ejecutarSeguro(Accion accion)
{
    try {
        accion();
    } catch (const exception& e) {
        temporizador_.stop();
        soltarTodo();
        QMessageBox::critical(this, "Error al cargar el nivel", QString::fromUtf8(e.what()));
        juego_.volverALMenu();
        reloj_.restart();
        temporizador_.start(MS_POR_FOTOGRAMA);
    }
}


// Ciclo del juego

//Cada ~16 ms: mide cuánto tiempo pasó de verdad (dt), avanza la lógica y pide redibujar. gestorlogico ya recorta un dt muy grande (DT_MAXIMO) y no hace nada si el juego no está en JUGANDO (menú, pausa)
void VistaJuego::tick()
{
    const float dt = static_cast<float>(reloj_.nsecsElapsed()) * 1e-9f;
    reloj_.restart();
    juego_.actualizar(dt);
    update();   // Qt llamará a paintEvent()
}

// Teclado


// Marca una tecla de movimiento como presionada o suelta
// Devuelve false si la tecla no es de movimiento
bool VistaJuego::cambiarTecla(int tecla, bool presionada)
{
    switch (tecla) {
    case Qt::Key_W: case Qt::Key_Up:    arriba_ = presionada;    return true;
    case Qt::Key_S: case Qt::Key_Down:  abajo_ = presionada;     return true;
    case Qt::Key_A: case Qt::Key_Left:  izquierda_ = presionada; return true;
    case Qt::Key_D: case Qt::Key_Right: derecha_ = presionada;   return true;
    case Qt::Key_Shift:                 correr_ = presionada;    return true;
    default:                            return false;
    }
}


//Le pasa a Mario la dirección según las teclas presionadas. Se llama solo cuando cambia una tecla (no en cada fotograma): Protagonista
//normaliza la dirección ahí, así que el ciclo del juego no hace raíces. En la lógica y crece hacia ARRIBA: W / flecha arriba = +y.

void VistaJuego::enviarEntrada()
{
    Nivel* nivel = juego_.nivelactual();
    if (nivel == nullptr || nivel->protagonista() == nullptr) return;
    const float x = static_cast<float>((derecha_ ? 1 : 0) - (izquierda_ ? 1 : 0));
    const float y = static_cast<float>((arriba_ ? 1 : 0) - (abajo_ ? 1 : 0));
    nivel->protagonista()->setEntrada(vector2(x, y), correr_);
}

void VistaJuego::soltarTodo()
{
    arriba_ = abajo_ = izquierda_ = derecha_ = correr_ = false;
}

void VistaJuego::keyPressEvent(QKeyEvent* evento)
{
    // Mantener una tecla presionada repite el evento: se ignora la repetición.
    if (evento->isAutoRepeat()) {
        evento->accept();
        return;
    }
    const int tecla = evento->key();
    if (cambiarTecla(tecla, true)) {
        enviarEntrada();
        return;
    }

    const EstadoJuego estado = juego_.estado();
    switch (tecla) {
    case Qt::Key_Space:
        if (estado == EstadoJuego::JUGANDO && juego_.nivelactual() != nullptr)
            juego_.nivelactual()->protagonista()->solicitarSalto();
        break;
    case Qt::Key_P:
        if (estado == EstadoJuego::JUGANDO) juego_.pausar();
        else if (estado == EstadoJuego::PAUSA) juego_.reanudar();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        accionPrincipal();
        break;
    case Qt::Key_Escape:
        juego_.volverALMenu();
        break;
    case Qt::Key_1:
        juego_.setdificultad(NivelDificultad::FACIL);    // solo funciona en el menú
        break;
    case Qt::Key_2:
        juego_.setdificultad(NivelDificultad::NORMAL);
        break;
    case Qt::Key_3:
        juego_.setdificultad(NivelDificultad::DIFICIL);
        break;
    default:
        QWidget::keyPressEvent(evento);
        return;
    }
    update();
}

void VistaJuego::keyReleaseEvent(QKeyEvent* evento)
{
    if (evento->isAutoRepeat()) {
        evento->accept();
        return;
    }
    if (cambiarTecla(evento->key(), false))
        enviarEntrada();
    else
        QWidget::keyReleaseEvent(evento);
}

// Si la ventana pierde el foco (por ejemplo, Alt+Tab), las teclas se
// sueltan sin que llegue el evento: se sueltan todas y se pausa.
void VistaJuego::focusOutEvent(QFocusEvent* evento)
{
    soltarTodo();
    enviarEntrada();
    juego_.pausar();
    QWidget::focusOutEvent(evento);
}

// Enter: hace lo que corresponde según el estado de la partida.
void VistaJuego::accionPrincipal()
{
    switch (juego_.estado()) {
    case EstadoJuego::MENU:
    case EstadoJuego::VICTORIA:
    case EstadoJuego::DERROTA:
        ejecutarSeguro([this] { juego_.iniciar(); });          // partida nueva
        break;
    case EstadoJuego::VIDA_PERDIDA:
        ejecutarSeguro([this] { juego_.reiniciarNivel(); });   // otra vez el mismo nivel
        break;
    case EstadoJuego::NIVEL_SUPERADO:
        ejecutarSeguro([this] { juego_.avanzarNivel(); });     // siguiente nivel
        break;
    case EstadoJuego::PAUSA:
        juego_.reanudar();
        break;
    case EstadoJuego::JUGANDO:
        break;
    }
    // El Mario nuevo recibe las teclas que ya están presionadas, y el
    // primer dt no cuenta el tiempo que tardó en cargar el nivel.
    enviarEntrada();
    reloj_.restart();
}


// Dibujo

void VistaJuego::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(20, 20, 28));

    const EstadoJuego estado = juego_.estado();
    const Nivel* nivel = juego_.nivelactual();
    if (estado == EstadoJuego::MENU || nivel == nullptr) {
        dibujarMenu(p);
        return;
    }

    const float tiempo = static_cast<float>(relojTotal_.elapsed()) * 0.001f;
    const QRect areaJuego(MARGEN, ALTO_HUD + MARGEN, width() - 2 * MARGEN, height() - ALTO_HUD - 2 * MARGEN);

    // Cada nivel se dibuja a su manera.
    if (const NivelLaberinto* laberinto = dynamic_cast<const NivelLaberinto*>(nivel))
        dibujoLaberinto_.dibujar(p, *laberinto, areaJuego, tiempo);
    // else if (const NivelBloques* bloques = dynamic_cast<...>(nivel))   // nivel 1 (tu compañero)
    //     dibujoBloques_.dibujar(p, *bloques, areaJuego, tiempo);

    dibujarHud(p, QRect(0, 0, width(), ALTO_HUD));

    switch (estado) {
    case EstadoJuego::PAUSA:
        dibujarMensaje(p, "Pausa", "P o Enter para continuar  ·  Esc: menú");
        break;
    case EstadoJuego::VIDA_PERDIDA:
        dibujarMensaje(p, "¡Te atraparon!",
                       QString("Te quedan %1 vidas  ·  Enter para intentarlo otra vez").arg(juego_.vidas()));
        break;
    case EstadoJuego::NIVEL_SUPERADO:
        dibujarMensaje(p, "¡Nivel superado!", "Enter para el siguiente nivel");
        break;
    case EstadoJuego::VICTORIA:
        dibujarMensaje(p, "¡Ganaste!", "Enter para jugar otra vez  ·  Esc: menú");
        break;
    case EstadoJuego::DERROTA:
        dibujarMensaje(p, "Fin del juego", "Te quedaste sin vidas  ·  Enter para empezar de nuevo");
        break;
    case EstadoJuego::MENU:
    case EstadoJuego::JUGANDO:
        break;
    }
}

// Barra de arriba: nivel, dificultad, vidas, energía y avisos del apagón.
void VistaJuego::dibujarHud(QPainter& p, const QRect& barra)
{
    p.fillRect(barra, QColor(10, 10, 16));
    QFont fuente = p.font();
    fuente.setPixelSize(16);
    fuente.setBold(true);
    p.setFont(fuente);
    p.setPen(Qt::white);

    const int centroY = barra.center().y();
    const QString izquierda = QString("Nivel %1/%2   ·   %3   ·   Vidas: %4")
                                  .arg(juego_.indicenivelactual() + 1)
                                  .arg(juego_.totalniveles())
                                  .arg(nombreDificultad(juego_.dificultad()))
                                  .arg(juego_.vidas());
    p.drawText(QRect(12, barra.y(), barra.width() / 2, barra.height()), Qt::AlignVCenter | Qt::AlignLeft, izquierda);

    // Barra de energía para correr (se pone roja cuando está agotada).
    const Nivel* nivel = juego_.nivelactual();
    if (nivel != nullptr && nivel->protagonista() != nullptr) {
        const Protagonista& m = *nivel->protagonista();
        const int anchoBarra = 180;
        const QRect marco(barra.width() - anchoBarra - 16, centroY - 8, anchoBarra, 16);
        p.setPen(Qt::white);
        p.drawText(QRect(marco.x() - 90, barra.y(), 85, barra.height()), Qt::AlignVCenter | Qt::AlignRight, "Energía");
        p.setBrush(Qt::NoBrush);
        p.drawRect(marco);
        const int lleno = static_cast<int>(static_cast<float>(marco.width() - 2) * m.energia().porcentaje());
        p.fillRect(QRect(marco.x() + 1, marco.y() + 1, lleno, marco.height() - 2),
                   m.energia().agotada() ? QColor(220, 60, 60) : QColor(70, 200, 90));
    }

    // Aviso del apagón (solo en el laberinto).
    if (const NivelLaberinto* laberinto = dynamic_cast<const NivelLaberinto*>(nivel)) {
        QString aviso;
        if (laberinto->lucesApagadas())
            aviso = QString("¡Apagón!  %1 s").arg(static_cast<int>(laberinto->tiempoParaCambioDeLuz()) + 1);
        else if (laberinto->avisoApagon())
            aviso = "Las luces van a fallar...";
        if (!aviso.isEmpty()) {
            p.setPen(QColor(255, 200, 60));
            p.drawText(barra, Qt::AlignCenter, aviso);
        }
    }
}

// Cuadro en el centro con un título y una línea de ayuda.
void VistaJuego::dibujarMensaje(QPainter& p, const QString& titulo, const QString& detalle)
{
    const QRect caja(width() / 2 - 300, height() / 2 - 70, 600, 140);
    p.setPen(QPen(QColor(255, 255, 255, 180), 2));
    p.setBrush(QColor(0, 0, 0, 200));
    p.drawRoundedRect(caja, 12, 12);

    QFont fuente = p.font();
    fuente.setBold(true);
    fuente.setPixelSize(34);
    p.setFont(fuente);
    p.setPen(Qt::white);
    p.drawText(QRect(caja.x(), caja.y() + 12, caja.width(), 60), Qt::AlignCenter, titulo);

    fuente.setBold(false);
    fuente.setPixelSize(16);
    p.setFont(fuente);
    p.drawText(QRect(caja.x(), caja.y() + 80, caja.width(), 40), Qt::AlignCenter, detalle);
}

// Menú sencillo (en el paso de los menús se cambia por pantallas con botones).
void VistaJuego::dibujarMenu(QPainter& p)
{
    QFont fuente = p.font();
    fuente.setBold(true);
    fuente.setPixelSize(40);
    p.setFont(fuente);
    p.setPen(QColor(250, 215, 40));
    p.drawText(QRect(0, height() / 2 - 160, width(), 60), Qt::AlignCenter, "Mario huelguista");

    fuente.setPixelSize(20);
    p.setFont(fuente);
    p.setPen(Qt::white);
    p.drawText(QRect(0, height() / 2 - 80, width(), 30), Qt::AlignCenter,
               QString("Dificultad: %1").arg(nombreDificultad(juego_.dificultad())));

    fuente.setBold(false);
    fuente.setPixelSize(16);
    p.setFont(fuente);
    p.drawText(QRect(0, height() / 2 - 45, width(), 30), Qt::AlignCenter,
               "1: Fácil   ·   2: Normal   ·   3: Difícil");
    p.drawText(QRect(0, height() / 2 + 10, width(), 30), Qt::AlignCenter, "Enter para jugar");
    p.setPen(QColor(170, 170, 190));
    p.drawText(QRect(0, height() / 2 + 70, width(), 60), Qt::AlignCenter,
               "W A S D o flechas: moverse   ·   Shift: correr   ·   P: pausa   ·   Esc: menú\n"
               "Si te quedas quieto, Mario se duerme y los Minions lo oyen roncar");
}