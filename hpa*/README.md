## HPA*

### Peak memory usage

Using 8-directional octile. Using release candidate build, measured by Valgrind + Massif (stacks enabled), and perf.
All tests performed with a cluster size of 100.

| Implementation                                          | Heap (bytes) | Stack (bytes) | Preprocess CPU Time (s) | Path CPU Time (s) | Cache misses (%) | Revision                                 |
|---------------------------------------------------------|--------------|---------------|-------------------------|-------------------|------------------|------------------------------------------|
| initial implementations                                 | 28,976,600   | 856           |                         | 34.56             | 28.28%           | 2dbb63f64eaca8a83dffa0b92b431fda681cd0a6 |
| minor optimizations                                     | 405,139,880  | 1104          |                         | 14.48             | 28.28%           | c9eaf3de4391259986e6180d0b4aaae7a2e4a170 |
| reduce arr sizes in cluster_a to only be within cluster | 405,139,880  | 1104          |                         | 0.910             | 28.28%           | f8e8e9e6bcc9df3f28198feddbea41b9cafb2e6e |
| fix memory leak in min heap                             | 784,248      | 864           | 3.752                   | 0.029             | 1%               | 41e3fcae3f140a104c9714da5e89c6ab9ae66b4b |
| performance improvements                                | 780,696      | 848           | 1.929                   | 0.023             | 1%               | bcd338b39eec1edeea21482d0318f68b694c98c3 |

### Graph peak memory 
- old graph 24,861,296
- new graph 899,123

### Analysis of nodes per side of cluster

| Nodes per side of cluster | Node picked   | CPU Time (s) | Path length | Error (from 1634) |
|---------------------------|---------------|--------------|-------------|-------------------|
| 3                         | 0, 1/2, 1     | 8.364        | 1683        | 3%                |
| 3                         | 1/3, 1/2, 2/3 | 7.989        | 1668        | 2%                |
| 2                         | 1/3, 2/3      | 3.624        | 1668        | 2%                |
| 1                         | 1/2           | 0.897        | 1765        | 8%                |
| 1                         | Random        | 0.932        | ~2004       | 22%               |

All tests run with revision f8e8e9e6bcc9df3f28198feddbea41b9cafb2e6e with release mode.
The node picked indicates the indices of all nodes that were picked out of all possible options.
For example, 0 picks the first node out of all available ones, 1/2 picks the middle node, and 1 the last one.