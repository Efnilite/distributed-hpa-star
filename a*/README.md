## Notes

### Design
- Cache lines: 64 bits
- DMA: 32 bits (requires manual start of transfer)

### Todo
- [ ] float -> uint. scale scores by 2 st 2*sqrt(2)=~=3
- [ ] byte-align structures
- [ ] use one map
- [ ] precalculate memory usage to avoid realloc

### Directions
#### scene_mp_2p_01
A more diagonal map.

| Type                    | Path length | Path cost | CPU Time (s) | Visited (% of all nodes) | 
|-------------------------|-------------|-----------|--------------|--------------------------|
| 4-directional Manhattan | x        | 2871      | 0.472        | 14.65%                   |
| 4-directional Chebyshev | x        | 2871      | 1.893        | 53.38%                   |
| 4-directional Euclidean | x        | 2871      | 2.102        | 52.90%                   |
| 8-directional Chebyshev | x        | 2257      | 0.449        | 9.31%                    |
| 8-directional Euclidean | x        | 2221      | 0.621        | 11.16%                   |

#### scene_mp_2p_04
A more traditional, rectangular map.

| Type                    | Path length | Path cost | CPU Time (s) | Visited (% of all nodes) | 
|-------------------------|-------------|-----------|--------------|--------------------------|
| 4-directional Manhattan | x        | 4261      | 0.725        | 11.06%                   |
| 4-directional Chebyshev | x        | 4261      | 3.002        | 48.22%                   |
| 4-directional Euclidean | x        | 4261      | 3.423        | 46.86%                   |
| 8-directional Chebyshev | x        | 4141      | 2.240        | 25.97%                   |
| 8-directional Euclidean | x        | 3728      | 2.604        | 26.31%                   |

### Links
- Cache hit/misses: RP2040 Table 154

## Libraries
- [Min-heap-indirect by Armon Dadgar](https://github.com/armon/c-minheap-indirect/)
- [stb_ds](https://nothings.org/stb_ds/)
