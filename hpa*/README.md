## HPA*

### Peak memory usage

Using 8-directional octile. Using release candidate build, measured by Valgrind + Massif (stacks enabled), and perf.
All return a path length of 1648.

| Implementation                     | Heap (bytes) | Stack (bytes) | CPU Time (s) | Cache misses (%) | Revision                                 |
|------------------------------------|--------------|---------------|--------------|------------------|------------------------------------------|
| initial implementations            | 28,976,600   | 856           | 34.56        | 28.28%           | 2dbb63f64eaca8a83dffa0b92b431fda681cd0a6 |
| graph implementation optimizations | 405,139,880  | 1104          | 1.824        | 28.28%           | a5e3d77098fc6b8824efdb3b6e9416d9940c94cf |

