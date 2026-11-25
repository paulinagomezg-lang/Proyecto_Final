#ifndef VISTAJUEGO_H
#define VISTAJUEGO_H

#include "gestorlogico.h"
#include "DibujoLaberinto.h"
#include <QWidget>
#include <QTimer>
#include <QElapsedTimer>

class QKeyEvent;
class QPaintEvent;
class QFocusEvent;
class QPainter;


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

private slots:
    void tick();

private:
    // Teclas
    bool cambiarTecla(int tecla, bool presionada);   // true si era de movimiento
    void enviarEntrada();
    void accionPrincipal();                         // Enter
    void soltarTodo();

    // Llama a algo de gestorlogico que puede lanzar (cargar un nivel lee
    // el mapa): si falla, muestra el error y vuelve al menú.
    template <typename Accion>
    void ejecutarSeguro(Accion accion);

    // Dibujo
    void dibujarHud(QPainter& p, const QRect& barra);
    void dibujarMensaje(QPainter& p, const QString& titulo, const QString& detalle);
    void dibujarMenu(QPainter& p);

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
};

#endif // VISTAJUEGO_H
