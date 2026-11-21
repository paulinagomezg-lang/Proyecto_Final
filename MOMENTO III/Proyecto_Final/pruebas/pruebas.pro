QT -= gui
CONFIG += console c++17
CONFIG -= app_bundle

INCLUDEPATH += ..

!msvc: QMAKE_CXXFLAGS += -Wall -Wextra

# Las pruebas (están en esta carpeta)
SOURCES += \
    main_pruebas.cpp \
    prueba_Protagonista.cpp \
    prueba_enemigopersonaje.cpp \
    prueba_vector2.cpp \
    prueba_Energiacarrera.cpp \
    prueba_gestorlogico.cpp \
    prueba_Nivel.cpp

# Las clases del juego (están en la carpeta de arriba)
SOURCES += \
    ../personaje.cpp \
    ../enemigo.cpp \
    ../Energiacarrera.cpp \
    ../gestorlogico.cpp \
    ../Nivel.cpp \
    ../Protagonista.cpp
