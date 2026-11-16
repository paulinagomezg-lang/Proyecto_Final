#ifndef EXCEPCIONES_DEL_MAPA_H
#define EXCEPCIONES_DEL_MAPA_H

#include "Excepciones_del_Juego.h"
#include <sstream>

using namespace std;

class Excepciones_del_Mapa : public Excepciones_del_Juego
{
public:
    Excepciones_del_Mapa(const string& archivo, const string& detalle,  int linea = 0)

        : Excepciones_del_Juego(construirMensaje(archivo, detalle, linea)), archivo_(archivo), linea_(linea) {}

    const string& archivo() const { return archivo_; }

    int linea() const { return linea_; }

private:
    string archivo_;

    int linea_;

    static string construirMensaje(const string& archivo, const string& detalle, int linea)
    {
        ostringstream m;

        m << "Error en " << archivo;

        if (linea > 0) m << ", linea " << linea;

        m << ": " << detalle;

        return m.str();
    }
};

#endif // EXCEPCIONES_DEL_MAPA_H