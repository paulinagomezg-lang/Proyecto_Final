#ifndef EXCEPCIONES_DEL_JUEGO_H
#define EXCEPCIONES_DEL_JUEGO_H

#include <stdexcept>
#include <string>
#include <sstream>

using namespace std;

class Excepciones_del_Juego : public runtime_error
{
public:
    explicit Excepciones_del_Juego(const string& mensaje)

        : runtime_error(mensaje) {}

    // Destructor virtual: se van a atrapar por referencia a la base.
    virtual ~Excepciones_del_Juego() noexcept {}
};

class Excepciones_de_Parametros_Fisicos : public Excepciones_del_Juego
{
public:
    Excepciones_de_Parametros_Fisicos(const string& parametro, float valor, const string& condicion)

        : Excepciones_del_Juego(construirMensaje(parametro, valor, condicion)),

        parametro_(parametro),

        valor_(valor) {}

    const string& parametro() const { return parametro_; }

    float valor() const { return valor_; }

    // Ayudas para validar en una sola línea desde los constructores
    // y los setters de las físicas.
    static void exigirPositivo(const string& parametro, float valor)
    {
        if (!(valor > 0.0f))   // también rechaza NaN

            throw Excepciones_de_Parametros_Fisicos(parametro, valor, "debe ser mayor que 0");
    }

    static void exigirNoNegativo(const string& parametro, float valor)
    {
        if (!(valor >= 0.0f))

        throw Excepciones_de_Parametros_Fisicos(parametro, valor, "no puede ser negativo");
    }

    static void exigirEnRango(const string& parametro, float valor, float minimo, float maximo)
    {
        if (!(valor >= minimo && valor <= maximo)) {

            ostringstream condicion;

            condicion << "debe estar entre " << minimo << " y " << maximo;

            throw Excepciones_de_Parametros_Fisicos(parametro, valor, condicion.str());
        }
    }

private:
    string parametro_;

    float valor_;

    static string construirMensaje(const string& parametro, float valor, const string& condicion)
    {
        ostringstream m;

        m << "Parametro fisico invalido: " << parametro << " = " << valor << " (" << condicion << ")";

        return m.str();
    }
};


class Excepciones_de_Recursos : public Excepciones_del_Juego
{
public:
    explicit Excepciones_de_Recursos(const string& ruta, const string& motivo = "no existe o no se pudo abrir")

        : Excepciones_del_Juego("No se pudo cargar el recurso '" + ruta + "': " + motivo),

        ruta_(ruta) {}

    const string& ruta() const { return ruta_; }

private:
    string ruta_;
};

#endif // EXCEPCIONES_DEL_JUEGO_H