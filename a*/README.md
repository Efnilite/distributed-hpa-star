## Notes

### Design
- Cache lines: 64 bits
- DMA: 32 bits (requires manual start of transfer)

### Todo

### Directions
#### scene_mp_2p_01
A more diagonal map.

| Type                    | Path length | Path cost | CPU Time (s) | Visited (% of all nodes) | 
|-------------------------|-------------|-----------|--------------|--------------------------|
| 4-directional Manhattan | 2871        | 2871      | 0.044        | 10.55%                   |
| 4-directional Chebyshev | 2871        | 2871      | 0.234        | 60.32%                   |
| 4-directional Euclidean | 2871        | 2871      | 0.254        | 60.29%                   |
| 8-directional Octile    | 1634        | 2145      | 0.254        | 59.04%                   |
| 8-directional Euclidean | 1634        | 2146      | 0.258        | 59.03%                   |

#### scene_mp_2p_04
A more traditional, rectangular map.

| Type                    | Path length | Path cost | CPU Time (s) | Visited (% of all nodes) | 
|-------------------------|-------------|-----------|--------------|--------------------------|
| 4-directional Manhattan | 4261        | 4261      | 0.088        | 11.78%                   |
| 4-directional Chebyshev | 4261        | 4261      | 0.399        | 55.46%                   |
| 4-directional Euclidean | 4261        | 4261      | 0.411        | 55.46%                   |
| 8-directional Octile    | 3182        | 3628      | 0.447        | 55.44%                   |
| 8-directional Euclidean | 3182        | 3628      | 0.452        | 55.44%                   |

8-directional octile used

### Peak memory usage 

| Implementation                 | Heap (bytes) | Stack (bytes) |
|--------------------------------|--------------|---------------|
| stb_ds hashmaps                |              |               |
| 1d array                       | 37,860,856   | 728           |
| 1d array, map stored in bitset | 34,849,480   | 744           |

### Links
- Cache hit/misses: RP2040 Table 154

## Libraries
- [Min-heap-indirect by Armon Dadgar](https://github.com/armon/c-minheap-indirect/)
- [stb_ds by nothings](https://nothings.org/stb_ds/)
