## HPA*

### Peak memory usage

Using 8-directional octile. Using release candidate build, measured by Valgrind + Massif (stacks enabled), and perf.
All tests performed with a cluster size of 100.

| Nodes per side of cluster | Path length |
|---------------------------|-------------|
| 3                         | 1648        |
| 2                         |             |
| 1                         | 1765        |

| Implementation                                          | Heap (bytes) | Stack (bytes) | CPU Time (s) | Cache misses (%) | Revision                                 |
|---------------------------------------------------------|--------------|---------------|--------------|------------------|------------------------------------------|
| initial implementations                                 | 28,976,600   | 856           | 34.56        | 28.28%           | 2dbb63f64eaca8a83dffa0b92b431fda681cd0a6 |
| minor optimizations                                     | 405,139,880  | 1104          | 14.48        | 28.28%           | c9eaf3de4391259986e6180d0b4aaae7a2e4a170 |
| reduce arr sizes in cluster_a to only be within cluster | 405,139,880  | 1104          | 0.910        | 28.28%           |  |

