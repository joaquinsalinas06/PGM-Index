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

## PGM estático

`PGMIndex(keys, eps)` recibe un vector ordenado de `unsigned long long`.
El vector debe vivir más que el índice y no modificarse mientras se use.
`lookup(x)` devuelve cuántas llaves son menores que `x`, igual que
`std::lower_bound`; los duplicados se conservan. `approx(x)`
da la predicción y la ventana `[lo, hi)` donde se corrige con búsqueda binaria.
Si la respuesta es el final del vector, puede coincidir con `hi`.

`ULLONG_MAX` está reservado para el centinela: se rechaza como dato, pero
consultarlo devuelve el tamaño. Se valida el orden inicial, ε >= 1 y
n <= INT_MAX. La búsqueda vacía y los extremos también están cubiertos.
Por ahora los segmentos se localizan con búsqueda binaria en un único nivel.

### Niveles recursivos

Cada nivel superior indexa las llaves iniciales de los segmentos del inferior.
Se construyen hasta tener un solo segmento; la búsqueda baja desde la raíz.
`eps_rec` vale 4 por defecto. `height()` cuenta niveles y
`seg_count()` cuenta segmentos producidos por el PLA del nivel base,
sin los centinelas auxiliares. `size_in_bytes()` mide segmentos y offsets;
`ram_bytes()` añade el objeto y la capacidad reservada, sin contar el vector externo.

## B+ estático

`StaticBPlusTree(keys, capacidad)` construye hojas y padres de abajo hacia arriba.
Las hojas referencian páginas del vector externo: este debe mantenerse vivo
e inmutable. Los nodos se liberan al destruir el árbol; no se permite copiarlo.
`bytes` cuenta separadores y punteros internos; `ram_bytes` cuenta los nodos
y sus vectores, sin los datos externos. `height` cuenta niveles internos.

La carga masiva sigue el algoritmo de STX/TLX B+ tree
([tlx/container/btree.hpp](https://github.com/tlx/tlx/blob/master/tlx/container/btree.hpp)).

`StaticBPlusTree::lookup(x)` devuelve la misma posición de `lower_bound`
que el PGM, incluyendo la primera ocurrencia de una llave repetida.
