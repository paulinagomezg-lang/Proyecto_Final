QT -= gui
CONFIG += console c++17
CONFIG -= app_bundle

INCLUDEPATH += ..

!msvc: QMAKE_CXXFLAGS += -Wall -Wextra

# Las pruebas (están en esta carpeta)
SOURCES += \
    main_pruebas.cpp \
    prueba_vector2.cpp \
    prueba_Energiacarrera.cpp \
    prueba_gestorlogico.cpp \
    prueba_Nivel.cpp

# Las clases del juego (están en la carpeta de arriba)
SOURCES += \
    ../Energiacarrera.cpp \
    ../gestorlogico.cpp \
    ../Nivel.cpp \
    ../personaje.cpp