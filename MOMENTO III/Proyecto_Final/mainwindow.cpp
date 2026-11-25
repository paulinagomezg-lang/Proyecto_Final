#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "VistaJuego.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , vista_(nullptr)
{
    ui->setupUi(this);

    // La vista del juego ocupa toda la ventana. Se crea con 'new' y no se
    // borra a mano: en Qt, un widget con padre lo borra su padre, y
    // setCentralWidget() hace a la ventana su dueña (también borra el
    // centralwidget vacío que venía en el .ui).
    vista_ = new VistaJuego(this);
    setCentralWidget(vista_);
    ui->menubar->hide();
    ui->statusbar->hide();

    setWindowTitle("Guerra de Tropical Oil Company");
    resize(1100, 760);
    vista_->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}
