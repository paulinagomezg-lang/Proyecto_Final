# Proyecto de pruebas: programa de consola, sin ventanas.
# Es independiente del juego y no se agrega a Proyecto_Final.pro.
QT -= gui
CONFIG += console c++17
CONFIG -= app_bundle

# Usa los mismos .h del juego (están en la carpeta de arriba)
INCLUDEPATH += ..

!msvc: QMAKE_CXXFLAGS += -Wall -Wextra

SOURCES += \
    prueba_Energiacarrera.cpp \
    prueba_gestorlogico.cpp \
    prueba_vector2.cpp
