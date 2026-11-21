#ifndef ITEMPODER_H
#define ITEMPODER_H

#include "vector2.h"

class NivelBloques;

class ItemPoder
{
public:
    ItemPoder(vector2 posicionBase, float amplitud = 0.15f, float omega = 3.0f, float fase = 0.0f)
        : posicionBase_(posicionBase), amplitud_(amplitud), omega_(omega), fase_(fase),
        posicion_(posicionBase), recogido_(false) {}

    vector2 posicion() const     { return posicion_; }
    vector2 posicionBase() const { return posicionBase_; }
    bool    recogido() const     { return recogido_; }

private:
    friend class NivelBloques;   // solo el nivel lo mueve y lo marca

    vector2 posicionBase_;
    float   amplitud_;
    float   omega_;
    float   fase_;
    vector2 posicion_;
    bool    recogido_;
};

#endif // ITEMPODER_H