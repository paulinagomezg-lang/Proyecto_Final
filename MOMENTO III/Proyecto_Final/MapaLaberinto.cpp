#include "MapaLaberinto.h"
#include "Excepciones_del_Mapa.h"
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
using namespace std;

namespace {

//concierte un float a int sin desbordarse (posiciones absurdas o NaN que quedan fuera del mapa y fuera de la pared)
int aEnteroSeguro(float v){
    if (!(v > -1000.0f)) return -1000; //limite inferior y filtro anti NaN
    if (v > 1000.0f) return 1000; //limite superior
    return static_cast<int>(floor(v)); //el floor redondea el decimal hacie el entero inferior mas cercano
}

//recorre eñ texto linea por linea sin copiar las lineas, quita el final '\r'
struct LectorLineas
{
    const string& texto;
    size_t pos;

    explicit LectorLineas(const string& t) : texto(t), pos(0) {}

    bool siguiente(size_t& inicio, size_t& longitud){
        if (pos >= texto.size()) return false;
        inicio = pos;
        size_t fin = texto.find('\n', pos);
        if (fin == string::npos) fin = texto.size();
        pos = fin + 1;
        longitud = fin - inicio;
        if (longitud > 0 && texto[inicio + longitud - 1] == '\r') --longitud;
        return true;
    }
};
}

MapaLaberinto::MapaLaberinto(const string& contenido, const string& nombre, float tamCelda) : nombre_(nombre), tamCelda_(tamCelda), inversaTam_(0.0f),
    ancho_(0), alto_(0), inicio_{-1, -1}, salida_{-1, -1}, marca_(0){
    Excepciones_de_Parametros_Fisicos::exigirPositivo("tamCelda", tamCelda);
    inversaTam_ = 1.0f / tamCelda;
    cargar(contenido);
}

MapaLaberinto MapaLaberinto::desdeArchivo(const string& ruta, float tamCelda){
    ifstream archivo(ruta, ios::binary); //abre el archivo en ruta como modo de lectura, ios hace que lea los bytes tal cual estan en el disco sin traducir ni alterar saltos de linea
    if (!archivo)
        throw Excepciones_de_Recursos(ruta);// si el archivo no se pudo abrir abre hay mismo excepciones
    ostringstream texto; //crea un flujo de texto en la memoria ram es un contenedor temporal que recibe grandes cantidades de texto
    texto << archivo.rdbuf(); //accede directamente al bufer interno
    return MapaLaberinto(texto.str(), ruta, tamCelda);
}

//se lee en dos pasadas del mismo texto donde cuenta las filas y revisa que todas esten en el mismo archivo y con el tamaño conocido reserva una sola vez y llena
void MapaLaberinto::cargar(const string& contenido){
    //pasa 1 : tamano y las lineas vacias del final se ignoran
    int filas = 0;
    int ultimaNoVacia = 0;
    {
        LectorLineas lector(contenido);
        size_t ini = 0, lon = 0;
        while (lector.siguiente(ini, lon)) {
            ++filas;
            if (lon > 0){
                ultimaNoVacia = filas;
                const int lonEntera = static_cast<int>(lon);
                if (ancho_ == 0) ancho_ = lonEntera;
                else if (lonEntera != ancho_){
                    ostringstream m; //Crea un constructor de texto en memoria para armar un mensaje de error detallado
                    m <<"La fila mide" <<lonEntera <<" y debe medir" << ancho_ <<" (todas las filas deben tener el mismo ancho)"; //Concatena en el mensaje cuánto midió la fila defectuosa y cuánto debería medir
                    throw Excepciones_del_Mapa(nombre_ , m.str(), filas);
                }
            }
        }
    }

    alto_ = ultimaNoVacia;
    if (alto_ ==0)
        throw Excepciones_del_Mapa(nombre_, "El mapa esta vacio");
    if (ancho_ > MAXIMO_LADO || alto_ > MAXIMO_LADO){
        ostringstream m;
        m <<"El mapa mide" << ancho_ <<" x"<<alto_<<" y el maximo es "<< MAXIMO_LADO << "x"<< MAXIMO_LADO;
        throw Excepciones_del_Mapa(nombre_ , m.str());
    }

    //Pasada 2: llenar
    //reserva el espacio exacto de tpdp el tablero dentro de un vector unidimensional y lo llena con espacios en 0
    paredes_.assign(static_cast<size_t>(ancho_)*static_cast<size_t>(alto_),0); //sinze_t solo admite 0 y numeros+ diseñado para representar tamaños de memoria, longitudes de texto e indice de arreglo
    LectorLineas lector(contenido);
    size_t ini = 0, lon = 0;
    for (int fila = 0; fila < alto_ && lector.siguiente(ini, lon); ++fila) {
        const int linea = fila+1; //para los mensajes , la linea empiezan en 1 para reportar errores claros al usuario
        if (lon == 0)
            throw Excepciones_del_Mapa(nombre_, "Fila vacia en medio del mapa", linea);

        for (int col = 0; col < ancho_; ++col) {
            const char c = contenido[ini + static_cast<size_t>(col)];
            const Celda celda{col, fila};
            switch (c) { //reserva memoria para el tablero e interpreta carácter por carácter para ubicar las paredes, a Mario (M), la salida (S), los guardias (G) y los bloques (B)
            case '#': //convierte una coordenada 2d fila columna en el indice de un arreglo lineal de 1d
                paredes_[static_cast<size_t>(fila * ancho_ + col)] = 1;
                break;

            case '.':
            case ' ':
                break;

            case 'M': //inicio de mario
                if (inicio_.columna >= 0)
                    throw Excepciones_del_Mapa(nombre_, "Hay mas de un inicio de Mario (M)", linea);
                inicio_ = celda;
                break;

            case 'S': // garantiza que solo haya una salida en el nivel
                if (salida_.columna >= 0)
                    throw Excepciones_del_Mapa(nombre_, "Hay mas de una salida (S)", linea);
                salida_ = celda;
                break;

            case 'G': //guardia
                guardias_.push_back(celda);
                break;

            case 'B': //bloque
                bloques_.push_back(celda);
                break;

            default: { //si encientra un simbolo diferente a los permitidos y lanza errores
                ostringstream m;
                m <<"caracter '" << c <<"' no valido en la columna "<< (col + 1) <<" (se permiten # . M S G B y espacio) ";
                throw Excepciones_del_Mapa(nombre_, m.str(), linea);
            }
            }
        }
    }

    if (inicio_.columna < 0)
        throw Excepciones_del_Mapa(nombre_, "Falta el inicio de Mario (M)");
    if (salida_.columna < 0)
        throw Excepciones_del_Mapa(nombre_, "Falta la salida (S)");

    //listas de los pasillos y memoria de trabajp del camino mas corto
    const size_t total = paredes_.size();
    libres_.reserve(total);
    for (int f = 0; f < alto_; ++f)
        for (int c = 0; c < ancho_; ++c)
            if (!esPared(c, f)) libres_.push_back(Celda{c, f});
    cola_.assign(total, 0);
    visitado_.assign(total, 0);

    //los vectores ya no van a crecer porque se libera la capacidad sobrante
    guardias_.shrink_to_fit(); //que libere la memoria RAM sobrante que reservó de más mientras se leía el mapa
    bloques_.shrink_to_fit();
    libres_.shrink_to_fit();
}

//hace la conversion celda mundo y crece hacia arriba en el archivo la fila 0 es la de arriba por eso filadesdeabajo = alto - 1- fila
MapaLaberinto::Celda MapaLaberinto::celdaDe(vector2 posicion) const{
    const int columna = aEnteroSeguro(posicion.x * inversaTam_);
    const int filaDesdeAbajo = aEnteroSeguro(posicion.y * inversaTam_);
    return Celda{columna, alto_- 1 - filaDesdeAbajo};
}

vector2 MapaLaberinto::centroDe(Celda c)const{
    return vector2((static_cast<float>(c.columna) + 0.5f) * tamCelda_, (static_cast<float>(alto_ - 1 - c.fila) + 0.5f) * tamCelda_);
}

//linea de vision, avanza celda por celda a lo largo del segmento desde -> hasta (revisa solo las celdas que el segmento toca sin reices, ni memoria dinamica)
bool MapaLaberinto::hayLineaDeVision(vector2 desde, vector2 hasta) const{
    const float ax = desde.x * inversaTam_, ay = desde.y * inversaTam_; //coordenadas de cuadricula
    const float bx = hasta.x * inversaTam_, by = hasta.y * inversaTam_;

    int c = aEnteroSeguro(ax), r = aEnteroSeguro(ay); //celda actual (r desde abajo)
    const int cFin = aEnteroSeguro(bx), rFin = aEnteroSeguro(by);

    if (esPared(c, alto_ - 1 - r)) return false; //si el punto de origen ya esta metido dentro de un muro
    const float dx = bx - ax, dy = by - ay;
    const int pasoC = dx > 0.0f ? 1 : (dx < 0.0f ? -1 : 0);// 1 avanza, -1 retrocede, 0 esta en el eje correcto
    const int pasoR = dy > 0.0f ? 1 : (dy < 0.0f ? -1 : 0);

    //tmax: fraccion en la que se cruza el siguiente borde
    //tDelta: fraccion que ocupa una celda completa
    const float INFINITO = 1e30f;
    const float tDeltaX = pasoC != 0 ? 1.0f / fabs(dx) : INFINITO;
    const float tDeltaY = pasoR != 0 ? 1.0f / fabs(dy) : INFINITO;
    float tMaxX = pasoC > 0 ? (static_cast<float>(c + 1) - ax) * tDeltaX : pasoC < 0 ? (ax - static_cast<float>(c)) * tDeltaX : INFINITO;
    float tMaxY = pasoR > 0 ? (static_cast<float>(r + 1) - ay) * tDeltaY : pasoR < 0 ? (ay - static_cast<float>(r)) * tDeltaY : INFINITO;

    //numero exacto de pasos uno por cada borde que se cruza, asi el ciclo termina aunque haya redondeos
    const int pasos = abs(cFin - c) + abs(rFin - r);
    for (int i = 0; i < pasos; ++i) {
        //si una coordenada y allego a su celda final solo avanza a la otra y evita pasar por un redondeo
        const bool avanzaX = (r == rFin) || (c != cFin && tMaxX < tMaxY);
        if (avanzaX){
            tMaxX += tDeltaX;
            c += pasoC;
        } else{
            tMaxY += tDeltaY;
            r += pasoR;
        }
        if (esPared(c, alto_ - 1 - r)) return false;
    }
    return true;
}


//camino mas corto: empieza en hasta y se expande por capas.
//costo: como maximo una vista por celda ( el laberinto de 21x13 menos de 273) sin pedir memoria

bool MapaLaberinto::siguientePaso(Celda desde, Celda hasta, Celda& paso) const{
    if (esPared(desde) || esPared(hasta)) return false;
    if (desde == hasta){
        paso = desde;
        return true;
    }

    //nueva marca: lo marcado con otro numero cuenta como visitado
    if (++marca_ == 0){ //dio la vuelta luego de 4 ml millones
        visitado_.assign(visitado_.size(), 0);
        marca_ = 1;
    }

    const int inicio = indice(hasta.columna, hasta.fila);
    const int objetivo = indice(desde.columna, desde.fila);
    size_t cabeza = 0, fin = 0;
    cola_[fin++] = inicio;
    visitado_[static_cast<size_t>(inicio)] = marca_;

    const int dc[4] = {0, 0, -1, 1}; //cuanto cambia la columna -1 izquierda y 1 derecha
    const int df[4] = {-1, 1, 0, 0}; // cuanto cambia la fila

    while (cabeza < fin) { // para encontrar el camino mas corto en el laberinto
        const int actual = cola_[cabeza++];
        const int c = actual % ancho_; //guarda cada celda como unico numero
        const int f = actual / ancho_;
        for (int k = 0; k < 4; ++k) {
            const int nc = c + dc[k], nf = f + df[k];
            //calcula las coordenadas de las casillas arri, aba, izqui y dere
            if (esPared(nc, nf)) continue; //si el vecino es un muro o se sale del mapa , lo descarta y pasa a siguiente
            const int vecino = indice(nc, nf);
            const size_t v = static_cast<size_t>(vecino);
            if (visitado_[v] == marca_) continue;
            visitado_[v] = marca_;
            if (vecino == objetivo){ //busca el primer paso que el personaje debe dar para seguir la ruta más corta
                paso = Celda{c, f}; //la celda desde la que se llego
                return true;
            }
            cola_[fin++] = vecino; // si el vecino no era el objetivo, lo guarda al final de la cola
        }
    }
    return false;
}
