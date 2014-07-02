// default_mixer.cpp

#include "e-ognisko_mixer.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>

void mixer(
    struct mixer_input* inputs, size_t n,  // tablica struktur mixer_input, po jednej strukturze na każdą
                                           // kolejkę w stanie ACTIVE
    void* output_buf,                      // bufor, w którym mikser powinien umieścić dane do wysłania
    size_t* output_size,                   // początkowo rozmiar output_buf, następnie mikser umieszcza
                                           // w tej zmiennej liczbę bajtów zapisanych w output_buf
    unsigned long tx_interval_ms           // wartość zmiennej TX_INTERVAL
)
{
  size_t bytes = std::min(*output_size, 176 * tx_interval_ms);
  for(size_t pos = 0; pos < bytes/2; pos++)
  {
    ((int16_t*) output_buf)[pos] = 0;
    for(size_t input = 0; input < n; input++)
    {
      if(pos >= inputs[input].len)
        continue;
      inputs[input].consumed += 2;
      int32_t result = (((int16_t*) output_buf)[pos] + ((int16_t*) inputs[input].data)[pos]); 
      if(result < INT16_MIN)
        result = INT16_MIN;
      else if(result > INT16_MAX)
        result = INT16_MAX;
      ((int16_t*) output_buf)[pos] = (int16_t) result;
    }
  }
  *output_size = bytes;
}
