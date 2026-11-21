#ifndef BLOQUETIERRA_H
#define BLOQUETIERRA_H

class NivelBloques;

class BloqueTierra
{
public:
    // yArriba: altura de la cara superior en reposo. alto: hacia abajo.
    BloqueTierra(float xInicio, float ancho, float yArriba, float alto, bool esFirme)
        : xInicio_(xInicio), ancho_(ancho), yArriba_(yArriba), alto_(alto),
        esFirme_(esFirme), hundimiento_(0.0f), velHundimiento_(0.0f),
        rigidez_(0.0f), roto_(false) {}

    float xInicio() const     { return xInicio_; }
    float xFin() const        { return xInicio_ + ancho_; }
    float ancho() const       { return ancho_; }
    float alto() const        { return alto_; }
    float yArriba() const     { return yArriba_; }
    float superficie() const  { return yArriba_ - hundimiento_; }
    float yAbajo() const      { return superficie() - alto_; }
    bool  esFirme() const     { return esFirme_; }
    bool  roto() const        { return roto_; }
    float hundimiento() const { return hundimiento_; }
    float rigidez() const     { return rigidez_; }

    bool contieneX(float x, float margen = 0.0f) const
    {
        return x + margen > xInicio_ && x - margen < xFin();
    }

private:
    friend class NivelBloques;   // solo el nivel aplica la física

    float xInicio_;
    float ancho_;
    float yArriba_;
    float alto_;
    bool  esFirme_;

    // Estado del resorte (lo cambia NivelBloques)
    float hundimiento_;
    float velHundimiento_;
    float rigidez_;        // k actual: baja con el tiempo y con el peso
    bool  roto_;
};

#endif // BLOQUETIERRA_H
