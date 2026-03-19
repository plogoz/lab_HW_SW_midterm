# midterm task report:

- pas de biais, ni de ReLU en HW pour le moment
- tests faits sur l'image dog.9499.jpg.rgba.planar

## Table:

| Name | Description | Time (s) | Frequency (MHz) | LUTs | FFs | BRAMs | DSPs | Cost | Pareto ? (Yes / No) |
|-----------------|-----------------|---------|---------|---------|---------|---------|---------|---------|---------|
| SW | macOS M1 Max | 0.457 | n/a | n/a | n/a | n/a | n/a | 0 | Yes |
| SW | SW only on PYNQ | 28.079 | x | n/a | n/a | n/a | n/a | 0 | no |
| HW | without optimization | 87.635 | 100 | 5785 | 6938 | 0 | 45 | 9,46 | No |
| HW | task 2 | 38.21 | 100 | 3293 | 5076 | 5 | 22 | not yet done | - |

## Task 2: coeffs caching:
- we can't use any pragmas (it's for newer versions), so need to make an array and pipeline / unroll its loading loop.
Max coeffs number: on layer 4 (in conv2d.h):
```C++
#define CONV_SIZE 3

const uint32_t MAX_COEFF_NBR = 128 * 256 * CONV_SIZE * CONV_SIZE;
```

ça marche pas comme approche
