#include "fabricaniveles.h"
#include "nivelbloques.h"
#include "NivelLaberinto.h"
#include "Excepciones_del_Juego.h"
#include <QFile>
#include <QByteArray>
#include <string>

using namespace std;

namespace {

const char* const RUTA_LABERINTO = ":/mapas/laberinto1.txt";   // dentro de recursos.qrc

// Lee un archivo del .qrc completo. Se usa QFile porque ifstream no puede
// abrir recursos de Qt (las rutas que empiezan con ":/").
string leerRecurso(const char* ruta)
{
    QFile archivo(QString::fromUtf8(ruta));
    if (!archivo.open(QIODevice::ReadOnly))
        throw Excepciones_de_Recursos(ruta);
    const QByteArray datos = archivo.readAll();
    return string(datos.constData(), static_cast<size_t>(datos.size()));
}


const string& textoLaberinto()
{
    static const string texto = leerRecurso(RUTA_LABERINTO);
    return texto;
}

}
// Nivel 1: los bloques (vista lateral). Nivel 2: el laberinto (vista cenital).
int totalNiveles()
{
    return 2;
}

const char* nombreNivel(int indice)
{
    switch (indice) {
    case 0:  return "LOS BLOQUES";
    case 1:  return "EL LABERINTO";
    default: return "?";
    }
}

Nivel* crearNivel(int indice, NivelDificultad dificultad)
{
    switch (indice) {
    case 0:
        // El mapa del nivel 1 está en el código y cambia con la dificultad
        // (cuántos ítems hay). El constructor lanza si el mapa no es válido.
        return new NivelBloques(NivelBloques::mapaPorDefecto(dificultad), dificultad);
    case 1:
        return new NivelLaberinto(textoLaberinto(), "laberinto1.txt", dificultad);
    default:
        throw Excepciones_del_Juego("FabricaNiveles: no existe el nivel pedido");
    }
}