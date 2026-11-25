#ifndef VISTAJUEGO_H
#define VISTAJUEGO_H

#include "gestorlogico.h"
#include "DibujoLaberinto.h"
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>
#include <QPixmap>
#include <QRect>
#include <QString>

class QKeyEvent;
class QPaintEvent;
class QFocusEvent;
class QMouseEvent;
class QPainter;

/*
 * VistaJuego
 * ----------
 * El widget donde se juega. Une la interfaz con la lógica:
 *
 *   QTimer (cada ~16 ms = 60 fps)
 *      -> tick(): mide dt con QElapsedTimer
 *      -> gestorlogico::actualizar(dt)   (toda la lógica del juego)
 *      -> update() -> paintEvent()       (dibuja lo que pasó)
 *
 *   Teclado -> mario->setEntrada(...) / solicitarSalto()
 *
 * Menú (antes de jugar), con teclado o ratón:
 *   PRINCIPAL:  JUGAR · NIVELES · OPCIONES · CRÉDITOS   (X = salir)
 *   JUGAR o NIVELES -> elegir el nivel -> elegir la dificultad -> a jugar
 *   Flechas arriba/abajo: elegir · Enter: aceptar · Esc: atrás
 *
 * Controles en el juego:
 *   W A S D o flechas: moverse      Shift: correr      Espacio: saltar
 *   P: pausa       Enter: reintentar / siguiente nivel / jugar otra vez
 *   Esc: volver al menú
 *
 * Memoria: gestorlogico va por valor dentro de la vista (se destruye con
 * ella y él libera el nivel). El temporizador y el reloj también van por
 * valor: no hay new ni delete en esta clase.
 */
class VistaJuego : public QWidget
{
    Q_OBJECT

public:
    explicit VistaJuego(QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* evento) override;
    void keyPressEvent(QKeyEvent* evento) override;
    void keyReleaseEvent(QKeyEvent* evento) override;
    void focusOutEvent(QFocusEvent* evento) override;
    void mouseMoveEvent(QMouseEvent* evento) override;
    void mousePressEvent(QMouseEvent* evento) override;

private slots:
    void tick();

private:
    // Pantallas del menú
    enum class PantallaMenu { PRINCIPAL, NIVELES, DIFICULTAD, OPCIONES, CREDITOS };

    // Dónde va cada cosa del panel del menú (se usa para dibujar y para
    // saber qué botón tocó el ratón, así los dos siempre coinciden).
    static const int MAX_BOTONES = 6;
    static const int MAX_LINEAS = 8;
    struct DisenoMenu
    {
        QRect panel;
        QRect botones[MAX_BOTONES];
        int numBotones;
        QRect barraInferior;
        QRect botonCerrar;      // la X
        QRect zonaTexto;        // para OPCIONES y CRÉDITOS
    };

    // Teclas del juego
    bool cambiarTecla(int tecla, bool presionada);   // true si era de movimiento
    void enviarEntrada();
    void accionPrincipal();                         // Enter durante la partida
    void soltarTodo();

    // Menú
    bool manejarTeclaMenu(int tecla);               // true si la usó el menú
    void activarOpcion(int opcion);                 // Enter o clic sobre un botón
    void atrasMenu();                               // Esc o la X
    void irAPantalla(PantallaMenu pantalla, int opcion = 0);
    int etiquetasMenu(QString* etiquetas) const;    // textos de los botones
    int lineasInfo(QString* lineas) const;          // texto de OPCIONES y CRÉDITOS
    QString tituloPantalla() const;                 // lo que dice la barra de abajo
    DisenoMenu calcularDiseno() const;
    QRect rectPlaca() const;                        // placa del título

    // Llama a algo de gestorlogico que puede lanzar (cargar un nivel lee
    // el mapa): si falla, muestra el error y vuelve al menú.
    template <typename Accion>
    void ejecutarSeguro(Accion accion);

    // Dibujo
    void dibujarHud(QPainter& p, const QRect& barra);
    void dibujarMensaje(QPainter& p, const QString& titulo, const QString& detalle);
    void dibujarMenu(QPainter& p);
    void prepararFondoMenu();                       // cielo, colinas, ladrillos y título

    gestorlogico juego_;
    DibujoLaberinto dibujoLaberinto_;
    QTimer temporizador_;
    QElapsedTimer reloj_;          // dt entre fotogramas
    QElapsedTimer relojTotal_;     // tiempo desde que abrió (parpadeos)

    // Teclas de movimiento presionadas ahora.
    bool arriba_;
    bool abajo_;
    bool izquierda_;
    bool derecha_;
    bool correr_;

    // Menú
    PantallaMenu pantalla_;
    int opcionMenu_;               // botón resaltado en la pantalla actual
    int nivelElegido_;             // el que se eligió en NIVELES
    QPixmap fondoMenu_;            // la portada sin el panel, dibujada una vez
};

#endif // VISTAJUEGO_H
