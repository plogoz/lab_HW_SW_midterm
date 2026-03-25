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
|HW | task 3 | 8.6 | 100 | 19058 | 6227 | 125 | 46 | not done yet | - |

## Task 2: coeffs caching:
- we can't use any pragmas (it's for newer versions), so need to make an array and pipeline / unroll its loading loop.
Max coeffs number: on layer 4 (in conv2d.h):
```C++
#define CONV_SIZE 3

const uint32_t MAX_COEFF_NBR = 128 * 256 * CONV_SIZE * CONV_SIZE;
```

ça marche pas comme approche

## Task 4: to go faster, use more memory :

compute one whole frame (with one filter output) and then do the next (and thus pipeline everything !)

AXI buses are overloaded (so, 1 for in, 1 for filter and out)

To go faster, test if merging loops would increase the ability to pipeline tasks. To store everything, use array and the array shapes (easiest way to use multiple dimensions) and then array partitionning

If I want to go faster, ReLU and biais in HW

If again faster, split the computations in //

fastets way: Dataflow optimisation (mais relou, parce que faut réécrire la plupart du code)
