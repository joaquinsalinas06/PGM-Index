# PGM-index y B+ tree

## Integrantes:
- Joaquin Mauricio Salinas Salas
- Isaac Emanuel Javier Simeón Sarmiento
- Adrian Antonio Auqui Perez

Este proyecto fue hecho con C++ 17. Requiere Clang o GCC y Make. Hay que ejecutar `make test` para
compilar y verificar; `make clean` elimina solamente los binarios de `build/`.
Las pruebas usan `assert`: compilar sin `-DNDEBUG`.

En esta entrega se presentan los algoritmos y pruebas de corrección. Los experimentos,
datasets, gráficas y benchmarks se preparan por separado.

## Geometría

`pgm/pla.h` agrupa puntos, pendientes y segmentos. Las comparaciones usan
`__int128`, que esta disponible en Clang y GCC para las llaves de 64 bits.

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
Para una llave ausente, la respuesta puede coincidir con `hi`, incluido
el final del vector.

`ULLONG_MAX` está reservado para el centinela: se rechaza como dato, pero
consultarlo devuelve el tamaño. Se valida el orden inicial, ε >= 1 y
n <= INT_MAX y ε <= INT_MAX/4. La búsqueda vacía y los extremos también están cubiertos.
Los segmentos se localizan descendiendo por los niveles recursivos.

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

## B+ dinámico

`DynamicBPlusTree(keys, capacidad)` copia las llaves a sus propias hojas;
`DynamicBPlusTree(capacidad)` empieza vacío. Requiere capacidad >= 4.
`lookup(x)` devuelve el número de llaves menores que `x`, sumando tamaños
de subárboles. `size()` devuelve el total y `check()` verifica orden,
separadores, ocupación, profundidad y tamaños de subárboles.
`size_in_bytes()` cuenta separadores y punteros internos; `ram_bytes()`
incluye hojas y las llaves propias, por lo que no se debe sumar otro vector de datos.

### Inserción y hojas

`insert(x)` conserva duplicados y divide las hojas llenas. Las divisiones
se propagan por los nodos internos hasta crear una nueva raíz si hace falta.
No hay borrados ni estructuras auxiliares de sumas. Las operaciones de
inserción no prometen recuperación ante agotamiento de memoria.

## Cómo leer y probar

1. `pgm/pla.h`: geometría y rectas con error acotado.
2. `pgm/static.h`: el índice sobre un vector ordenado.
3. `bplus/static.h`: un árbol construido de abajo hacia arriba.
4. `bplus/dynamic.h`: hojas propias e inserciones con splits.

Las pruebas comparan búsquedas con `std::lower_bound`, incluyendo vacío,
duplicados, extremos, secuencias, separaciones constantes y saltos irregulares.
El B+ dinámico se contrasta después de cada inserción, con capacidades pequeñas
para forzar divisiones internas y crecimiento de raíz.

Por ejemplo, para `{1, 2, 3, 5000}`, `lookup(5000)` devuelve 3. Después de
insertar 4 en el B+ dinámico devuelve 4. El PGM y el B+ estáticos deben
reconstruirse si se modifica su vector; sus métodos no permiten insertar.

Los constructores estáticos rechazan vectores temporales para evitar referencias
colgantes. No se admiten copias; las estructuras con nodos liberan su memoria al salir
de alcance.
