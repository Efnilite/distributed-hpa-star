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

### Peak memory usage

Using 8-directional octile.

| Implementation                       | Heap (bytes) | Stack (bytes) | CPU Time (s) | Revision                                 |
|--------------------------------------|--------------|---------------|--------------|------------------------------------------|
| stb_ds hashmaps                      | 405,139,880  | 1104          | 1.824        | a5e3d77098fc6b8824efdb3b6e9416d9940c94cf |
| 1d array                             | 37,860,856   | 728           | 0.243        | a0075bbf704c743649f8572e5c85ccb43f75ffd5 |
| 1d array, map stored in bitset       | 34,849,480   | 744           | 0.253        | de2a019e54e95b628832bb168a3378026b3dfe9c |
| came_from uses relative directions   | 24,524,456   | 688           | 0.270        | b97da60dbb1ce25a20338ec6432bd4f168354b35 |
| struct ClosedNode replaced with bool | 14,199,464   | 688           | 0.268        | e00ee5aa153190970142fccb4bc262c7f56d6359 |
| closed uses vbitset                  | 11,242,352   | 712           | 0.275        | 771e88a72524f87468fd6145448af850408df284 |
| came_from uses vbitset               | 9,037,192    | 712           | 0.307        | 5e64e3aec4c1312ceb8f14ec293279747fa35a85 |

### Links

- Cache hit/misses: RP2040 Table 154

## Libraries

- [Min-heap-indirect by Armon Dadgar](https://github.com/armon/c-minheap-indirect/)
- [stb_ds by nothings](https://nothings.org/stb_ds/)
