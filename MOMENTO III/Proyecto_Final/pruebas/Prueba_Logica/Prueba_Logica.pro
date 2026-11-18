TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        enemigo.cpp \
        main.cpp \
        personaje.cpp

HEADERS += \
    Excepciones_del_Juego.h \
    enemigo.h \
    personaje.h \
    vector2.h
