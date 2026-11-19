# PGM-index y B+ tree

## Integrantes:
- Joaquin Mauricio Salinas Salas
- Isaac Emanuel Javier Simeón Sarmiento
- Adrian Antonio Auqui Perez

Proyecto en C++17. Requiere Clang o GCC y Make. Ejecuta `make test` para
compilar y verificar; `make clean` elimina solamente los binarios de `build/`.
Las pruebas usan `assert`: compilar sin `-DNDEBUG`.

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
5. `pgm/buffered.h`: PGM con buffer ordenado.
6. `pgm/logarithmic.h`: PGM por niveles.

Las pruebas comparan búsquedas con `std::lower_bound`, incluyendo vacío,
duplicados, extremos, secuencias, separaciones constantes y saltos irregulares.
El B+ dinámico se contrasta después de cada inserción, con capacidades pequeñas
para forzar divisiones internas y crecimiento de raíz.

Por ejemplo, para `{1, 2, 3, 5000}`, `lookup(5000)` devuelve 3. Después de
insertar 4 en el B+ dinámico devuelve 4. El PGM y el B+ estáticos deben
reconstruirse si se modifica su vector; sus métodos no permiten insertar.

## PGM dinámicos: dos formas de reutilizar el estático

`BufferedPGM(keys, eps=64, capacity=256)` mantiene un PGM de la base y
un buffer ordenado. `lookup(x)` suma los ranks de ambos. Cuando el buffer
se llena, se fusiona con toda la base y se construye otro PGM. Es fácil de
seguir, pero cada reconstrucción recorre toda la base y causa una pausa.
El buffer admite inserciones entre consultas: no hace falta vaciarlo para leer.

`LogarithmicPGM(keys, eps=64, capacity=256)` mantiene el mismo buffer y
varios niveles de capacidad `capacity * 2^k`. Cada nivel no vacío tiene un PGM
estático. Al llenarse el buffer se fusionan los niveles pequeños hasta encontrar
capacidad suficiente. La búsqueda suma el rank de todos los niveles y del buffer.
Así evita reconstruir siempre toda la base, a cambio de consultar varios índices.

En ambos casos hay `insert(x)`, `lookup(x)`, `size()` y `check()`. Las llaves
iniciales se copian: el vector del llamador puede cambiar o dejar de existir.
Los duplicados se mantienen. No hay borrados, tombstones, Fenwick ni treap.
Las pruebas fuerzan reconstrucciones y fusiones con capacidades pequeñas.

### Ejemplo para seguir los cambios

Con `{1, 2, 3, 5000}`, `lookup(5000)` es 3 en todas las estructuras.
Con un buffer de capacidad 2, insertar 4 cambia inmediatamente la respuesta a 4,
aunque el modelo de la base no cambia todavía. Insertar 5 llena el buffer:
el PGM con buffer reconstruye las seis llaves; el logarítmico coloca las dos
nuevas en el nivel pequeño disponible. Ambos devuelven ahora 5. El B+ actualiza
sus hojas y divide los nodos que se llenen. Los estáticos necesitan reconstruirse.

### Interfaz

- Estáticos: `PGMIndex(keys, eps)` y `StaticBPlusTree(keys, capacidad)`.
- Dinámicos: `BufferedPGM`, `LogarithmicPGM` y `DynamicBPlusTree`.
- En todas las estructuras, buscar significa contar llaves estrictamente menores.
- `seg_count()` suma segmentos base en los PGM dinámicos; el buffer sin
  modelo no cuenta. En el logarítmico, `levels_used()` cuenta niveles ocupados;
  no es la altura recursiva de un PGM. En el buffered, `height()` es la de la base.
- `rebuilds` y `merges` cuentan mantenimientos. `moved_keys`
  cuenta llaves escritas en el destino de cada mantenimiento; no todas las
  copias intermedias ni el desplazamiento dentro del buffer.
- `size_in_bytes()` cuenta segmentos y offsets. `ram_bytes()` incluye objetos,
  vectores de datos, buffers e índices; excluye el overhead del asignador y el
  pico temporal durante una reconstrucción. No son mediciones de RSS del proceso.
- Los buffers son vectores ordenados: insertar desplaza hasta O(capacity) llaves.
  La variante buffered paga además una reconstrucción completa cada lote;
  no se le atribuyen los costos amortizados del método logarítmico.

`make test` verifica los índices. En `bench/`, `make results` repite los experimentos
(los conjuntos de llaves van en `bench/data`) y `make figuras` genera las gráficas y tablas.
El informe está en `informe/`.

Los constructores estáticos rechazan vectores temporales para evitar referencias
colgantes. Los estáticos no admiten copias, y los PGM dinámicos son propietarios
de sus datos. El PLA recibe puntos con llaves crecientes.
