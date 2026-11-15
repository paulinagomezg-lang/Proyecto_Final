#ifndef VECTOR2_H
#define VECTOR2_H
#include <cmath>

struct vector2
{
    float x;
    float y;

    //tolerancia para comparrar numeros decimales. el float causa demasiados errores de redondeo por lo que dos valores pueden definirse en algo como 0.00001
    static constexpr float EPSILON = 1e-5f;

    //constructores
    constexpr vector2() : x(0.0f), y(0.0f){}
    constexpr vector2(float x_, float y_) : x(x_), y(y_){}

    //crea un vector de longitud 'magnitud' en la direccion angulo rad. Se utiliza para la vigilancia y el cono de vision
    static vector2 desdeAngulo(float anguloRad, float magnitud = 1.0f){
        return vector2(std::cos(anguloRad)*magnitud, std::sin(anguloRad)*magnitud);
    }

    //operacion entre vectores
    constexpr vector2 operator+(vector2 otro) const{
        return vector2(x + otro.x, y + otro.y);
    }
    constexpr vector2 operator-(vector2 otro) const{
        return vector2(x - otro.x, y-otro.y);
    }
    constexpr vector2 operator-()const{
        return vector2(-x,-y);
    }

    //las versiones compuestas(+= , -=) modifican el vector en su lugar y no crean uno temporal, ayudan a la fisica
    //velocidad += aceleracion * dt
    //posicion += velocidad * dt
    vector2& operator+=(vector2 otro){
        x += otro.x;
        y += otro.y;
        return *this;
    }
    vector2& operator-=(vector2 otro){
        x -= otro.x;
        y -= otro.y;
        return *this;
    }

    //operaciones con un escalar
    constexpr vector2 operator*(float k)const{
        return vector2(x * k, y * k);
    }
    vector2& operator*=(float k){
        x *= k;
        y *= k;
        return *this;
    }
    //dividir por cero no tiene sentido fisico ais que en lugar de producir infinito que se devuelve y rompe el juego, mejor devolemos vector nulo
    vector2 operator/(float k)const{
        if (std::fabs(k) < EPSILON) return vector2();
        const float inverso = 1.0f/k; //una division y dos multiplicaciones
        return vector2(x * inverso, y * inverso);
    }
    vector2& operator/=(float k){
        *this = *this / k;
        return *this;
    }

    //producto punto :indica si dos direcciones apuntan hacia el mismo lado.
    //es positivo si el angulo entre ellas es menos de 90 para eso sirve si esta adelante de un minion.
    constexpr float punto(vector2 otro) const{
        return x * otro.x + y * otro.y;
    }

    //producto cruz : en 2d con componente en z indica si otro esta a la izquierda(positivo) o a la derecha(negativo) de ese vector
    constexpr float cruz(vector2 otro)const{
        return x * otro.y - y * otro.x;
    }

    //longitudes y distancias
    constexpr float normaCuadrada() const{
        return x * x + y * y;
    }
    float norma() const{
        return std::sqrt(normaCuadrada());
    }

    constexpr float distanciaCuadrada(vector2 otro)const {
        return (*this - otro).normaCuadrada();
    }
    float distancia(vector2 otro)const{
        return (*this - otro).norma();
    }

    //devuelve un vector de longitus 1 en la misma direccion (el vector nulo no tiene direccion y se devuelve tal cual)
    vector2 normalizado()const{
        const float n2 = normaCuadrada();
        if (n2 < EPSILON * EPSILON) return vector2();
        const float inverso = 1.0f /std::sqrt(n2);
        return vector2(x * inverso, y * inverso);
    }

    //recorta la longitud del vector sin calcular su direccion.
    //sirve para velocidad maxima calcula la raiz cuando hay que rercortar y la norma al cuadrado se calcula una vez(se reutiliza para escalado y la comparacion)
    vector2 limitado(float maximo) const{
        const float n2 = normaCuadrada();
        if (n2 <= maximo*maximo)return *this;
        const float escala = maximo / std::sqrt(n2);
        return vector2(x * escala, y * escala);
    }

    //angulos (angulo de vector en radianes entre pi y -pi (0 asi la derecha))
    float angulo() const{ //std::atan2 devuelve el ángulo en radianes
        return std::atan2(y , x);//calcula el arco tangente de x y y (atan 2 hace los 4 cuadrantes del plano cartesiano)
    }

    //gira el vector 'angulorad' radianes alredor del origen
    vector2 rotado(float anguloRad) const{
        return rotado(std::cos(anguloRad), std::sin(anguloRad));
    }

    //misma rotacion, recibiendo cos y el sen ya calculados, cuando se giran varios punos en el mismo angulo se calculan una sola vez y se reutilizan
    constexpr vector2 rotado(float cosAngulo, float senAngulo) const{
        return vector2(x * cosAngulo - y * senAngulo, x * senAngulo + y * cosAngulo);
    }

    //comparacion, igualdad con tolerancia, por errores de redondeo de los float
    bool CasiIgual(vector2 otro, float tolerancia = EPSILON) const{
        return std::fabs(x - otro.x) <= tolerancia && std::fabs(y - otro.y) <= tolerancia;
    }
    bool operator == (vector2 otro) const{
        return CasiIgual(otro);
    }
    bool operator != (vector2 otro) const{
        return !CasiIgual(otro);
    }
};

//permitir escribir escalar a la izquierda 2.0f*v igual que en las ecuaciones
constexpr vector2 operator*(float k, vector2 v){
    return v * k;
}

#endif // VECTOR2_H
