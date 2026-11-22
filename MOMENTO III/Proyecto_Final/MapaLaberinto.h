#ifndef MAPALABERINTO_H
#define MAPALABERINTO_H
#include "Vector2.h"
#include <string>
#include <vector>
using namespace std;


class MapaLaberinto
{
public:
    //celda de la cuadricula 'columna' y 'fila' no chocan con std
    struct Celda
    {
        int columna;
        int fila;
        bool operator == (const Celda& o) const{
            return columna == o.columna && fila == o.fila;
        }
        bool operator != (const Celda& o) const{
            return !(*this == o);
        }
    };

    //lee el mapa desde el texto del archivo (no desde la ruta)
    MapaLaberinto(const string& contenido, const string& nombre, float tamCelda = 1.0f);

    //atajo para leer desde un archivo disco
    static MapaLaberinto desdeArchivo(const string& ruta, float tamCelda = 1.0f);

    //tamaño
    int ancho() const{
        return ancho_; //columnas
    }
    int alto() const{
        return alto_; //filas
    }
    float tamCelda() const{
        return tamCelda_; //metros por celda
    }
    const string& nombre() const{
        return nombre_;
    }

    //paredes
    bool dentro(int columna, int fila) const{
        return columna >= 0 && columna < ancho_ && fila>= 0 && fila <alto_;
    }
    //fuera del mapa cuenta como pared
    bool esPared(int columna, int fila) const{
        return !dentro(columna, fila) || paredes_[static_cast<size_t>(fila * ancho_ + columna)] !=0;
    }
    bool esPared(Celda c) const{
        return esPared(c.columna, c.fila);
    }
    bool esParedEn(vector2 posicion) const{
        return esPared(celdaDe(posicion));
    }

    //conversion celda <-> mundo
    Celda celdaDe(vector2 posicion) const; //celda que contiene el punto
    vector2 centroDe(Celda c) const; //centro de la celda en metros
    vector2 centroDe(int columna, int fila) const{
        return centroDe(Celda{columna, fila});
    }

    //lo que se marco en el archivo (ya convertido en metro)
    vector2 inicioMario() const{
        return centroDe(inicio_);
    }
    vector2 salida() const{
        return centroDe(salida_);
    }
    Celda celdaInicio() const{
        return inicio_;
    }
    Celda celdaSalida() const{
        return salida_;
    }
    const vector<Celda>& guardias() const{
        return guardias_;
    }
    const vector<Celda>& bloquesMoviles() const{
        return bloques_;
    }
    //todas las celdas de pasillo los minions eligen de aqui adonde patrullar
    const vector<Celda>& celdasLibres() const{
        return libres_;
    }

    //camino mas corto busqueda en patrulla moviendose en 4 direcciones
    //eficiencia: busca desde 'hasta' hacia 'desde' y se detiene apenas encuentra , usa memoria reservada al cargar el mapa
    //para no etner que limpiar a cada rato el arreglo de visitados en cada busqueda

    bool siguientePaso(Celda desde, Celda hasta, Celda& paso) const;

    //se puede ver desde hasta sin que la pared lo tape (el agente no conoce de paredes asi que pregunta, recorre solo las celdas que cruza el segmento

    bool hayLineaDeVision(vector2 desde, vector2 hasta) const;

    //tamaño maximo aceptado donde protege contra archivos ya dañados o enormes
    static const int MAXIMO_LADO = 200;

private:
    void cargar(const string& contenido);
    int indice(int columna, int fila) const{
        return fila * ancho_ + columna;
    }
    string nombre_;
    float tamCelda_;
    float inversaTam_; // 1/tamcelda, precalculada
    int ancho_;
    int alto_;
    vector<unsigned char> paredes_; //1:pared, 0:libre, fila*ancho+columna
    Celda inicio_;
    Celda salida_;
    vector<Celda> guardias_;
    vector<Celda> bloques_;
    vector<Celda> libres_;

    //memoria de trabajo de siguientePaso, reserva solo una vez al cargar, reutiliza estos arreglos para buscar camino no cambia el mapa
    mutable vector<int> cola_; // cola exige el orden de llegada
    mutable vector<unsigned> visitado_; //consultar el mapa sin alterar sus paredes, su tamaño ni sus objetos, le da permiso a esas variables específicas para cambiar de valor incluso dentro de métodos const
    mutable unsigned marca_;
};

#endif // MAPALABERINTO_H
