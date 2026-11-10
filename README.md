# PGM-index y B+ tree

Proyecto en C++17. Requiere Clang o GCC y Make. Ejecuta `make test` para
compilar y verificar; `make clean` elimina solamente los binarios de `build/`.
Las pruebas usan `assert`: compilar sin `-DNDEBUG`.

Esta entrega contiene algoritmos y pruebas de corrección. Los experimentos,
datasets, gráficas y benchmarks se preparan por separado.

## Geometría

`pgm/pla.h` agrupa puntos, pendientes y segmentos. Las comparaciones usan
`__int128`, disponible en Clang y GCC, para las llaves de 64 bits.
