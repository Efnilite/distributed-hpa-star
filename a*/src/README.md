## Notes

### Directions
#### scene_mp_2p_01
A more diagonal map.

| Type                        | Path length | CPU Time (s) | Visited (% of all nodes) | 
|-----------------------------|-------------|--------------|--------------------------|
| 4-directional Manhattan     | 2872        | 0.472        | 14.65%                   |
| 4-directional Chebyshev     | 2872        | 1.893        | 53.38%                   |
| 4-directional Euclidean     | 2872        | 2.001        | 52.90%                   |
| **8-directional Manhattan** | 1724        | 0.056        | 1.38%                    |
| 8-directional Chebyshev     | 1646        | 1.340        | 25.35%                   |
| 8-directional Euclidean     | 1646        | 0.666        | 12.91%                   |

#### scene_mp_2p_04
A more traditional, rectangular map.

| Type                        | Path length | CPU Time (s) | Visited (% of all nodes) | 
|-----------------------------|-------------|--------------|--------------------------|
| 4-directional Manhattan     | 4262        | 0.730        | 11.06%                   |
| 4-directional Chebyshev     | 4262        | 2.950        | 48.22%                   |
| 4-directional Euclidean     | 4262        | 3.082        | 46.86%                   |
| **8-directional Manhattan** | 3351        | 0.502        | 5.55%                    |
| 8-directional Chebyshev     | 3310        | 3.222        | 33.51%                   |
| 8-directional Euclidean     | 3184        | 2.282        | 23.61%                   |

## Libraries
- [Min-heap-indirect by Armon Dadgar](https://github.com/armon/c-minheap-indirect/)
- [stb_ds](https://nothings.org/stb_ds/)
