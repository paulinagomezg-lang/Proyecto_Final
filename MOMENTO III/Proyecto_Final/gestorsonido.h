#ifndef GESTORSONIDO_H
#define GESTORSONIDO_H

#include <QSoundEffect>

class gestorlogico;
class Nivel;
class GestorSonido
{
public:
    GestorSonido();

    GestorSonido(const GestorSonido&) = delete;
    GestorSonido& operator=(const GestorSonido&) = delete;

    void cargar();
    void actualizar(const gestorlogico& juego);

private:
    void prepararEfecto(QSoundEffect& efecto, const char* ruta, float volumen);
    void reiniciarSeguimiento(const Nivel* nivel);

    QSoundEffect musica_;
    QSoundEffect salto_;
    QSoundEffect item_;
    QSoundEffect captura_;
    QSoundEffect victoria_;
    QSoundEffect musicaLaberinto_;
    QSoundEffect alerta_;
    QSoundEffect apagon_;

    // Lo que había en el fotograma anterior.
    const Nivel* nivelAnterior_;   // sin dueño: solo para comparar
    float tiempoAnterior_;
    int   estadoAnterior_;         // EstadoJuego guardado como int
    int   apoyoAnterior_;          // bloque que pisaba Mario (-1 en el aire)
    int   itemsRecogidos_;
    bool  perseguianAntes_;        // algún Minion perseguía a Mario
    bool  lucesApagadasAntes_;
};

#endif // GESTORSONIDO_H