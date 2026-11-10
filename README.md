# PGM-index y B+ tree

Proyecto en C++17. Requiere Clang o GCC y Make. Ejecuta `make test` para
compilar y verificar; `make clean` elimina solamente los binarios de `build/`.
Las pruebas usan `assert`: compilar sin `-DNDEBUG`.

Esta entrega contiene algoritmos y pruebas de corrección. Los experimentos,
datasets, gráficas y benchmarks se preparan por separado.

## Geometría

`pgm/pla.h` agrupa puntos, pendientes y segmentos. Las comparaciones usan
`__int128`, disponible en Clang y GCC, para las llaves de 64 bits.

## Segmentación

El PLA construye rectas con tolerancia ε; `make_segs` adapta los duplicados
y añade centinelas para las consultas fuera del rango. IDs consecutivos o
con separación constante pueden describirse con una sola recta.

El PGM está adaptado de [PGM-index](https://github.com/gvinciguerra/PGM-index),
de Giorgio Vinciguerra (Apache-2.0), en particular `piecewise_linear_model.hpp`
y `pgm_index.hpp`. Véase `LICENSE-PGM` para la licencia original.
