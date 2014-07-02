#ifndef EOGNISKO_MIXER_HPP
#define EOGNISKO_MIXER_HPP

// e-ognisko_mixer.hpp
//
// Contains information about what we expect from mixing function.

#include <cstdlib>

struct mixer_input {
  void* data;       // Wskaźnik na dane w FIFO
  size_t len;       // Liczba dostępnych bajtów
  size_t consumed;  // Wartość ustawiana przez mikser, wskazująca, ile bajtów należy
                    // usunąć z FIFO.
};

void mixer(
    struct mixer_input* inputs, size_t n,  // tablica struktur mixer_input, po jednej strukturze na każdą
                                           // kolejkę w stanie ACTIVE
    void* output_buf,                      // bufor, w którym mikser powinien umieścić dane do wysłania
    size_t* output_size,                   // początkowo rozmiar output_buf, następnie mikser umieszcza
                                           // w tej zmiennej liczbę bajtów zapisanych w output_buf
    unsigned long tx_interval_ms           // wartość zmiennej TX_INTERVAL
);

#endif
