#include "VistaJuego.h"
#include "fabricaniveles.h"
#include "Nivel.h"
#include "NivelLaberinto.h"
#include "nivelbloques.h"
#include "Protagonista.h"
#include "Excepciones_del_Juego.h"
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QPaintEvent>
#include <QMessageBox>
#include <QFont>
#include <QColor>
#include <QPen>
#include <QLinearGradient>
#include <QString>
#include <exception>

using namespace std;

namespace {

const int MS_POR_FOTOGRAMA = 16;   // ~60 fotogramas por segundo
const int ALTO_HUD = 44;           // barra de arriba (vidas, energía...)
const int MARGEN = 8;

// ---- Textos de la portada (aquí se cambian) ----
const char* const TITULO_LINEA_1 = "LUCHA SINDICAL";
const char* const TITULO_LINEA_2 = "TROPICAL OIL COMPANY";
const char* const SUBTITULO = "UdeA · Informática II · 2026-2";
const char* const INTEGRANTE_1 = "Paulina Gómez García";
const char* const INTEGRANTE_2 = "Pablo Andrés Carrillo Yepes";

// ---- Medidas del panel del menú ----
const int ANCHO_PANEL = 330;
const int ALTO_BOTON = 44;
const int SEPARACION_BOTON = 12;
const int ALTO_BARRA = 46;         // barra de abajo con el título y la X
const int ALTO_LINEA = 24;         // texto de OPCIONES y CRÉDITOS

// ---- Colores de la portada (estilo retro) ----
const QColor CIELO(92, 148, 252);
const QColor LADRILLO(200, 76, 12);
const QColor LADRILLO_CLARO(252, 160, 68);
const QColor LADRILLO_OSCURO(120, 40, 0);
const QColor COLINA(0, 168, 0);
const QColor COLINA_BORDE(0, 90, 0);
const QColor ARBUSTO(128, 208, 16);
const QColor CREMA(255, 226, 160);

// ---- Colores del panel (estilo botones de juego) ----
const QColor PANEL_ARRIBA(120, 140, 255);
const QColor PANEL_ABAJO(95, 70, 210);
const QColor AZUL_OSCURO(32, 30, 84);
const QColor BOTON_CLARO(250, 250, 252);
const QColor BOTON_GRIS(205, 208, 222);
const QColor BOTON_ELEGIDO_CLARO(255, 240, 160);
const QColor BOTON_ELEGIDO(255, 205, 60);

// Fuente de letras de ancho fijo, en negrita (aspecto retro).
QFont fuenteRetro(int pixeles, bool negrita = true)
{
    QFont f("Courier New");
    f.setStyleHint(QFont::Monospace);
    f.setPixelSize(pixeles);
    f.setBold(negrita);
    return f;
}

// Texto con sombra (primero la sombra corrida, luego el texto).
void textoConSombra(QPainter& p, const QRect& r, int alineacion, const QString& t,
                    const QColor& color, const QColor& sombra, int corrimiento)
{
    p.setPen(sombra);
    p.drawText(r.translated(corrimiento, corrimiento), alineacion, t);
    p.setPen(color);
    p.drawText(r, alineacion, t);
}

QString nombreDificultad(NivelDificultad d)
{
    switch (d) {
    case NivelDificultad::FACIL:   return "Fácil";
    case NivelDificultad::NORMAL:  return "Normal";
    case NivelDificultad::DIFICIL: return "Difícil";
    }
    return "Normal";
}

} // namespace

// ------------------------------------------------------------------
// Construcción
// ------------------------------------------------------------------

VistaJuego::VistaJuego(QWidget* parent)
    : QWidget(parent),
    juego_(crearNivel, totalNiveles()),
    arriba_(false),
    abajo_(false),
    izquierda_(false),
    derecha_(false),
    correr_(false),
    pantalla_(PantallaMenu::PRINCIPAL),
    opcionMenu_(0),
    nivelElegido_(0)
{
    setFocusPolicy(Qt::StrongFocus);          // para recibir el teclado
    setMouseTracking(true);                   // para resaltar el botón bajo el ratón
    setMinimumSize(640, 560);

    // Los sonidos se cargan una sola vez, antes de arrancar el ciclo.
    sonido_.cargar();

    temporizador_.setTimerType(Qt::PreciseTimer);
    connect(&temporizador_, &QTimer::timeout, this, &VistaJuego::tick);
    temporizador_.start(MS_POR_FOTOGRAMA);
    reloj_.start();
    relojTotal_.start();
}

/*
 * Llama a una acción de gestorlogico que puede lanzar una excepción
 * (crear un nivel lee el mapa: puede faltar el archivo o estar mal).
 */
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
        irAPantalla(PantallaMenu::PRINCIPAL);
        reloj_.restart();
        temporizador_.start(MS_POR_FOTOGRAMA);
    }
}

// ------------------------------------------------------------------
// Ciclo del juego
// ------------------------------------------------------------------

/*
 * Cada ~16 ms: mide cuánto tiempo pasó de verdad (dt), avanza la lógica y
 * pide redibujar. gestorlogico ya recorta un dt muy grande (DT_MAXIMO) y
 * no hace nada si el juego no está en JUGANDO (menú, pausa...).
 */
void VistaJuego::tick()
{
    const float dt = static_cast<float>(reloj_.nsecsElapsed()) * 1e-9f;
    reloj_.restart();
    juego_.actualizar(dt);
    sonido_.actualizar(juego_);   // suena lo que pasó en este fotograma
    update();   // Qt llamará a paintEvent()
}

// ------------------------------------------------------------------
// Teclado durante la partida
// ------------------------------------------------------------------

// Marca una tecla de movimiento como presionada o suelta.
// Devuelve false si la tecla no es de movimiento.
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

/*
 * Le pasa a Mario la dirección según las teclas presionadas. Se llama
 * solo cuando cambia una tecla (no en cada fotograma): Protagonista
 * normaliza la dirección ahí, así que el ciclo del juego no hace raíces.
 * En la lógica y crece hacia ARRIBA: W / flecha arriba = +y.
 */
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

    // En el menú, las flechas y Enter son del menú.
    if (juego_.estado() == EstadoJuego::MENU) {
        if (manejarTeclaMenu(tecla)) update();
        else QWidget::keyPressEvent(evento);
        return;
    }

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
        soltarTodo();
        irAPantalla(PantallaMenu::PRINCIPAL);
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

// Enter durante la partida: hace lo que corresponde según el estado.
void VistaJuego::accionPrincipal()
{
    switch (juego_.estado()) {
    case EstadoJuego::VICTORIA:
    case EstadoJuego::DERROTA:
        ejecutarSeguro([this] { juego_.iniciarEn(nivelElegido_); });   // jugar otra vez
        break;
    case EstadoJuego::VIDA_PERDIDA:
        ejecutarSeguro([this] { juego_.reiniciarNivel(); });           // otra vez el mismo nivel
        break;
    case EstadoJuego::NIVEL_SUPERADO:
        ejecutarSeguro([this] { juego_.avanzarNivel(); });             // siguiente nivel
        break;
    case EstadoJuego::PAUSA:
        juego_.reanudar();
        break;
    case EstadoJuego::MENU:
    case EstadoJuego::JUGANDO:
        break;
    }
    // El Mario nuevo recibe las teclas que ya están presionadas, y el
    // primer dt no cuenta el tiempo que tardó en cargar el nivel.
    enviarEntrada();
    reloj_.restart();
}

// Menú: navegación

void VistaJuego::irAPantalla(PantallaMenu pantalla, int opcion)
{
    pantalla_ = pantalla;
    opcionMenu_ = opcion;
}

// Textos de los botones de la pantalla actual. Devuelve cuántos hay.
int VistaJuego::etiquetasMenu(QString* etiquetas) const
{
    switch (pantalla_) {
    case PantallaMenu::PRINCIPAL:
        etiquetas[0] = "JUGAR";
        etiquetas[1] = "NIVELES";
        etiquetas[2] = "OPCIONES";
        etiquetas[3] = "CRÉDITOS";
        return 4;
    case PantallaMenu::NIVELES: {
        const int n = qMin(juego_.totalniveles(), MAX_BOTONES - 1);
        for (int i = 0; i < n; ++i)
            etiquetas[i] = QString("%1. %2").arg(i + 1).arg(QString::fromUtf8(nombreNivel(i)));
        etiquetas[n] = "VOLVER";
        return n + 1;
    }
    case PantallaMenu::DIFICULTAD:
        etiquetas[0] = "FÁCIL";
        etiquetas[1] = "NORMAL";
        etiquetas[2] = "DIFÍCIL";
        etiquetas[3] = "VOLVER";
        return 4;
    case PantallaMenu::OPCIONES:
    case PantallaMenu::CREDITOS:
        etiquetas[0] = "VOLVER";
        return 1;
    }
    return 0;
}

// Texto que va encima de los botones en OPCIONES y CRÉDITOS.
int VistaJuego::lineasInfo(QString* lineas) const
{
    if (pantalla_ == PantallaMenu::OPCIONES) {
        lineas[0] = "CONTROLES";
        lineas[1] = "W A S D / flechas: moverse";
        lineas[2] = "Shift: correr";
        lineas[3] = "Espacio: saltar (nivel 1)";
        lineas[4] = "P: pausa · Esc: menú";
        lineas[5] = "Nivel 1: la pesa kg te vuelve pesado";
        lineas[6] = "Nivel 2: si te quedas quieto, ¡te duermes!";
        return 7;
    }
    if (pantalla_ == PantallaMenu::CREDITOS) {
        lineas[0] = QString("%1 %2").arg(TITULO_LINEA_1).arg(TITULO_LINEA_2);
        lineas[1] = "Proyecto final";
        lineas[2] = "Informática II · UdeA";
        lineas[3] = "";
        lineas[4] = QString::fromUtf8(INTEGRANTE_1);
        lineas[5] = QString::fromUtf8(INTEGRANTE_2);
        return 6;
    }
    return 0;
}

QString VistaJuego::tituloPantalla() const
{
    switch (pantalla_) {
    case PantallaMenu::PRINCIPAL:  return "MENÚ";
    case PantallaMenu::NIVELES:    return "NIVELES";
    case PantallaMenu::DIFICULTAD: return "DIFICULTAD";
    case PantallaMenu::OPCIONES:   return "OPCIONES";
    case PantallaMenu::CREDITOS:   return "CRÉDITOS";
    }
    return "MENÚ";
}

/*
 * Qué hace cada botón:
 *   PRINCIPAL:  JUGAR o NIVELES -> NIVELES;  OPCIONES;  CRÉDITOS
 *   NIVELES:    un nivel -> DIFICULTAD;  VOLVER
 *   DIFICULTAD: fácil/normal/difícil -> empieza el nivel elegido;  VOLVER
 */
void VistaJuego::activarOpcion(int opcion)
{
    QString etiquetas[MAX_BOTONES];
    const int n = etiquetasMenu(etiquetas);
    if (opcion < 0 || opcion >= n) return;
    const bool esVolver = (opcion == n - 1) && pantalla_ != PantallaMenu::PRINCIPAL;

    switch (pantalla_) {
    case PantallaMenu::PRINCIPAL:
        if (opcion == 0 || opcion == 1) irAPantalla(PantallaMenu::NIVELES, nivelElegido_);
        else if (opcion == 2) irAPantalla(PantallaMenu::OPCIONES);
        else irAPantalla(PantallaMenu::CREDITOS);
        break;

    case PantallaMenu::NIVELES:
        if (esVolver) {
            atrasMenu();
        } else {
            nivelElegido_ = opcion;
            // Empieza resaltada la dificultad que ya estaba elegida.
            irAPantalla(PantallaMenu::DIFICULTAD, static_cast<int>(juego_.dificultad()));
        }
        break;

    case PantallaMenu::DIFICULTAD:
        if (esVolver) {
            atrasMenu();
        } else {
            juego_.setdificultad(static_cast<NivelDificultad>(opcion));   // 0 fácil, 1 normal, 2 difícil
            soltarTodo();
            ejecutarSeguro([this] { juego_.iniciarEn(nivelElegido_); });
            enviarEntrada();
            reloj_.restart();
        }
        break;

    case PantallaMenu::OPCIONES:
    case PantallaMenu::CREDITOS:
        atrasMenu();
        break;
    }
}

// Esc o la X: vuelve a la pantalla anterior; en la principal, cierra el juego.
void VistaJuego::atrasMenu()
{
    switch (pantalla_) {
    case PantallaMenu::PRINCIPAL:  window()->close(); break;
    case PantallaMenu::NIVELES:    irAPantalla(PantallaMenu::PRINCIPAL, 0); break;
    case PantallaMenu::DIFICULTAD: irAPantalla(PantallaMenu::NIVELES, nivelElegido_); break;
    case PantallaMenu::OPCIONES:   irAPantalla(PantallaMenu::PRINCIPAL, 2); break;
    case PantallaMenu::CREDITOS:   irAPantalla(PantallaMenu::PRINCIPAL, 3); break;
    }
}

// Flechas (o W/S) para elegir, Enter o Espacio para aceptar, Esc para atrás.
bool VistaJuego::manejarTeclaMenu(int tecla)
{
    QString etiquetas[MAX_BOTONES];
    const int n = etiquetasMenu(etiquetas);
    switch (tecla) {
    case Qt::Key_Up: case Qt::Key_W:
        opcionMenu_ = (opcionMenu_ + n - 1) % n;
        return true;
    case Qt::Key_Down: case Qt::Key_S:
        opcionMenu_ = (opcionMenu_ + 1) % n;
        return true;
    case Qt::Key_Return: case Qt::Key_Enter: case Qt::Key_Space:
        activarOpcion(opcionMenu_);
        return true;
    case Qt::Key_Escape: case Qt::Key_Backspace:
        atrasMenu();
        return true;
    default:
        return false;
    }
}

// Ratón: resalta el botón que está debajo.
void VistaJuego::mouseMoveEvent(QMouseEvent* evento)
{
    if (juego_.estado() != EstadoJuego::MENU) return;
    const QPoint punto = evento->position().toPoint();
    const DisenoMenu d = calcularDiseno();
    for (int i = 0; i < d.numBotones; ++i)
        if (d.botones[i].contains(punto) && opcionMenu_ != i) {
            opcionMenu_ = i;
            update();
        }
}

// Ratón: clic en un botón lo activa; clic en la X es "atrás".
void VistaJuego::mousePressEvent(QMouseEvent* evento)
{
    if (juego_.estado() != EstadoJuego::MENU || evento->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(evento);
        return;
    }
    const QPoint punto = evento->position().toPoint();
    const DisenoMenu d = calcularDiseno();
    if (d.botonCerrar.contains(punto)) {
        atrasMenu();
    } else {
        for (int i = 0; i < d.numBotones; ++i)
            if (d.botones[i].contains(punto)) {
                opcionMenu_ = i;
                activarOpcion(i);
                break;
            }
    }
    update();
}

// ------------------------------------------------------------------
// Menú: dibujo
// ------------------------------------------------------------------

// Placa del título: arriba y centrada.
QRect VistaJuego::rectPlaca() const
{
    const int pw = qMin(560, width() - 80);
    return QRect((width() - pw) / 2, static_cast<int>(height() * 0.04), pw, 150);
}

/*
 * Calcula dónde va cada parte del panel. Lo usan dibujarMenu() y el ratón,
 * así lo que se ve y lo que se toca siempre coinciden.
 */
VistaJuego::DisenoMenu VistaJuego::calcularDiseno() const
{
    DisenoMenu d;
    QString etiquetas[MAX_BOTONES];
    QString lineas[MAX_LINEAS];
    d.numBotones = etiquetasMenu(etiquetas);
    const int numLineas = lineasInfo(lineas);

    const int altoTexto = numLineas > 0 ? numLineas * ALTO_LINEA + 16 : 0;
    const int altoBotones = d.numBotones * ALTO_BOTON + (d.numBotones - 1) * SEPARACION_BOTON;
    const int altoPanel = 22 + altoTexto + altoBotones + 22 + ALTO_BARRA;
    const int arriba = rectPlaca().bottom() + 40;

    d.panel = QRect((width() - ANCHO_PANEL) / 2, arriba, ANCHO_PANEL, altoPanel);
    d.zonaTexto = QRect(d.panel.left() + 16, d.panel.top() + 18, ANCHO_PANEL - 32, altoTexto);
    const int y0 = d.panel.top() + 22 + altoTexto;
    for (int i = 0; i < d.numBotones; ++i)
        d.botones[i] = QRect(d.panel.left() + 35, y0 + i * (ALTO_BOTON + SEPARACION_BOTON),
                             ANCHO_PANEL - 70, ALTO_BOTON);
    d.barraInferior = QRect(d.panel.left(), d.panel.bottom() - ALTO_BARRA + 1, ANCHO_PANEL, ALTO_BARRA);
    d.botonCerrar = QRect(d.barraInferior.right() - 40, d.barraInferior.top() + 8, 30, 30);
    return d;
}

//Portada

void VistaJuego::prepararFondoMenu()
{
    const int w = width(), h = height();
    fondoMenu_ = QPixmap(w, h);
    fondoMenu_.fill(CIELO);
    QPainter p(&fondoMenu_);
    p.setRenderHint(QPainter::Antialiasing, true);

    const int lado = 32;                   // tamaño de un ladrillo
    const int piso = h - 2 * lado;

    // Nubes: tres círculos blancos cada una.
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::white);
    const double nubes[3][3] = {{0.12, 0.30, 1.0}, {0.86, 0.26, 1.2}, {0.80, 0.55, 0.8}};
    for (const auto& n : nubes) {
        const double cx = n[0] * w, cy = n[1] * h, s = n[2];
        p.drawEllipse(QPointF(cx - 34 * s, cy + 6 * s), 26 * s, 26 * s);
        p.drawEllipse(QPointF(cx, cy - 6 * s), 34 * s, 34 * s);
        p.drawEllipse(QPointF(cx + 36 * s, cy + 6 * s), 26 * s, 26 * s);
    }

    // Colinas (medio óvalo que sale del piso) y arbustos.
    p.setPen(QPen(COLINA_BORDE, 3));
    p.setBrush(COLINA);
    p.drawEllipse(QPointF(0.12 * w, piso), 170.0, 160.0);
    p.drawEllipse(QPointF(0.88 * w, piso), 150.0, 120.0);
    p.setPen(Qt::NoPen);
    p.setBrush(ARBUSTO);
    for (double cx : {0.30 * w, 0.72 * w}) {
        p.drawEllipse(QPointF(cx - 30, piso - 18), 22.0, 22.0);
        p.drawEllipse(QPointF(cx, piso - 26), 30.0, 30.0);
        p.drawEllipse(QPointF(cx + 30, piso - 18), 22.0, 22.0);
    }

    // Piso de ladrillos: dos filas, la segunda corrida medio ladrillo.
    p.setRenderHint(QPainter::Antialiasing, false);
    p.fillRect(QRect(0, piso, w, h - piso), LADRILLO);
    for (int fila = 0; fila < 2; ++fila) {
        const int y = piso + fila * lado;
        p.setPen(QPen(Qt::black, 2));
        p.drawLine(0, y, w, y);
        const int corrido = (fila % 2) * (lado / 2);
        for (int x = -corrido; x < w; x += lado) {
            p.setPen(QPen(Qt::black, 2));
            p.drawLine(x, y, x, y + lado);
            p.setPen(QPen(LADRILLO_CLARO, 2));
            p.drawLine(x + 2, y + 2, x + lado - 3, y + 2);   // brillo de arriba
        }
    }

    // Placa del título, con sombra, borde, brillo y remaches.
    p.setRenderHint(QPainter::Antialiasing, true);
    const QRect placa = rectPlaca();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(60, 20, 0));
    p.drawRoundedRect(placa.translated(6, 6), 10, 10);
    p.setPen(QPen(LADRILLO_OSCURO, 4));
    p.setBrush(LADRILLO);
    p.drawRoundedRect(placa, 10, 10);
    p.setPen(QPen(LADRILLO_CLARO, 3));
    p.drawLine(placa.left() + 8, placa.top() + 8, placa.right() - 8, placa.top() + 8);
    p.setPen(QPen(LADRILLO_OSCURO, 1));
    p.setBrush(LADRILLO_CLARO);
    for (const QPoint& esquina : {placa.topLeft() + QPoint(14, 14), placa.topRight() + QPoint(-14, 14),
                                  placa.bottomLeft() + QPoint(14, -14), placa.bottomRight() + QPoint(-14, -14)})
        p.drawEllipse(QPointF(esquina), 5.0, 5.0);

    //letra de el titulo
    const QColor sombraTitulo(40, 10, 0);
    p.setFont(fuenteRetro(38));
    textoConSombra(p, QRect(placa.left(), placa.top() + 14, placa.width(), 50), Qt::AlignCenter,
                   TITULO_LINEA_1, CREMA, sombraTitulo, 3);
    p.setFont(fuenteRetro(38));
    textoConSombra(p, QRect(placa.left(), placa.top() + 64, placa.width(), 70), Qt::AlignCenter,
                   TITULO_LINEA_2, CREMA, sombraTitulo, 4);

    p.setFont(fuenteRetro(15));
    p.setPen(Qt::white);
    p.drawText(QRect(0, placa.bottom() + 8, w, 26), Qt::AlignCenter, QString::fromUtf8(SUBTITULO));
}

/*
 * Panel del menú: fondo morado-azul con borde oscuro, botones claros con
 * remaches (el elegido en amarillo), y abajo una barra con el nombre de la
 * pantalla y una X (atrás, o salir en la principal).
 */
void VistaJuego::dibujarMenu(QPainter& p)
{
    if (fondoMenu_.size() != size())       // primera vez o cambió el tamaño
        prepararFondoMenu();
    p.drawPixmap(0, 0, fondoMenu_);

    const DisenoMenu d = calcularDiseno();
    QString etiquetas[MAX_BOTONES];
    QString lineas[MAX_LINEAS];
    etiquetasMenu(etiquetas);
    const int numLineas = lineasInfo(lineas);

    p.setRenderHint(QPainter::Antialiasing, true);

    // Panel: sombra, degradado y borde.
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 90));
    p.drawRoundedRect(d.panel.translated(6, 6), 18, 18);
    QLinearGradient degradado(d.panel.topLeft(), d.panel.bottomLeft());
    degradado.setColorAt(0.0, PANEL_ARRIBA);
    degradado.setColorAt(1.0, PANEL_ABAJO);
    p.setPen(QPen(AZUL_OSCURO, 4));
    p.setBrush(degradado);
    p.drawRoundedRect(d.panel, 18, 18);

    // Texto (OPCIONES y CRÉDITOS): la primera línea como título.
    for (int i = 0; i < numLineas; ++i) {
        p.setFont(fuenteRetro(i == 0 ? 18 : 15, i == 0));
        textoConSombra(p, QRect(d.zonaTexto.left(), d.zonaTexto.top() + i * ALTO_LINEA, d.zonaTexto.width(), ALTO_LINEA),
                       Qt::AlignCenter, lineas[i], Qt::white, AZUL_OSCURO, 1);
    }

    // Botones.
    p.setFont(fuenteRetro(20));
    for (int i = 0; i < d.numBotones; ++i) {
        const QRect& b = d.botones[i];
        const bool elegido = (i == opcionMenu_);
        QLinearGradient relleno(b.topLeft(), b.bottomLeft());
        relleno.setColorAt(0.0, elegido ? BOTON_ELEGIDO_CLARO : BOTON_CLARO);
        relleno.setColorAt(1.0, elegido ? BOTON_ELEGIDO : BOTON_GRIS);
        p.setPen(QPen(AZUL_OSCURO, elegido ? 4 : 3));
        p.setBrush(relleno);
        p.drawRoundedRect(b, 10, 10);

        // Remaches a los lados.
        p.setPen(QPen(AZUL_OSCURO, 1));
        p.setBrush(QColor(150, 155, 175));
        p.drawEllipse(QPointF(b.left() + 12, b.center().y()), 3.5, 3.5);
        p.drawEllipse(QPointF(b.right() - 12, b.center().y()), 3.5, 3.5);

        p.setPen(AZUL_OSCURO);
        p.drawText(b, Qt::AlignCenter, etiquetas[i]);
    }

    // Barra de abajo con el nombre de la pantalla y la X.
    p.setPen(Qt::NoPen);
    p.setBrush(AZUL_OSCURO);
    p.drawRoundedRect(d.barraInferior, 14, 14);
    p.drawRect(QRect(d.barraInferior.left(), d.barraInferior.top(), d.barraInferior.width(), 16));   // esquinas de arriba rectas
    p.setFont(fuenteRetro(22));
    p.setPen(Qt::white);
    p.drawText(QRect(d.barraInferior.left(), d.barraInferior.top(), d.barraInferior.width() - 40, ALTO_BARRA),
               Qt::AlignCenter, tituloPantalla());

    const QRect& x = d.botonCerrar;
    p.setPen(QPen(Qt::white, 2));
    p.setBrush(QColor(70, 90, 220));
    p.drawRoundedRect(x, 6, 6);
    p.setPen(QPen(Qt::white, 4));
    p.drawLine(x.left() + 9, x.top() + 9, x.right() - 8, x.bottom() - 8);
    p.drawLine(x.left() + 9, x.bottom() - 8, x.right() - 8, x.top() + 9);

    // Ayuda debajo del panel.
    p.setFont(fuenteRetro(14, false));
    textoConSombra(p, QRect(0, d.panel.bottom() + 12, width(), 24), Qt::AlignCenter,
                   "↑ ↓ elegir     ENTER aceptar     ESC atrás", Qt::white, AZUL_OSCURO, 1);
    p.setRenderHint(QPainter::Antialiasing, false);
}

// ------------------------------------------------------------------
// Dibujo del juego
// ------------------------------------------------------------------

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
    else if (const NivelBloques* bloques = dynamic_cast<const NivelBloques*>(nivel))
        dibujoBloques_.dibujar(p, *bloques, areaJuego, tiempo);

    dibujarHud(p, QRect(0, 0, width(), ALTO_HUD));
    dibujarHudBloques(p, QRect(0, 0, width(), ALTO_HUD), tiempo);

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
        dibujarMensaje(p, "Fin del juego", "Te quedaste sin vidas  ·  Enter para empezar de nuevo  ·  Esc: menú");
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
    const QRect caja(width() / 2 - 320, height() / 2 - 70, 640, 140);
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

/*
 * Avisos del nivel 1 en el centro de la barra de arriba:
 *   - Pesado: cuántos segundos le quedan al poder.
 *   - Evento por tiempo: cuánto falta para que los bloques se debiliten
 *     (en naranja los últimos 3 s, y parpadea el último segundo y medio,
 *     al mismo tiempo que los bloques).
 */
void VistaJuego::dibujarHudBloques(QPainter& p, const QRect& barra, float tiempo)
{
    const NivelBloques* bloques = dynamic_cast<const NivelBloques*>(juego_.nivelactual());
    if (bloques == nullptr || bloques->protagonista() == nullptr) return;
    const Protagonista& m = *bloques->protagonista();

    const float falta = bloques->tiempoHastaDebilitar();
    const bool rotos = bloques->bloquesRotos();
    QString aviso;
    if (rotos)
        aviso = "¡Los bloques se rompieron!";
    else if (bloques->eventosDebilitamiento() == 0)
        aviso = QString("Bloques se agrietan en %1 s").arg(static_cast<int>(falta) + 1);
    else
        aviso = QString("Bloques se rompen en %1 s").arg(static_cast<int>(falta) + 1);
    if (m.esPesado())
        aviso = QString("¡PESADO! %1 s   ·   ").arg(static_cast<int>(m.tiempoPesadoRestante()) + 1) + aviso;

    const bool urgente = rotos || falta <= 3.0f;
    const bool parpadeo = !rotos && falta <= 1.5f && static_cast<int>(tiempo * 8.0f) % 2 == 0;
    if (parpadeo) return;
    p.setPen(urgente ? QColor(255, 160, 40) : QColor(200, 200, 210));
    p.drawText(barra, Qt::AlignCenter, aviso);
}