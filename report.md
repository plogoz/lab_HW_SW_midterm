# midterm task report:

- pas de biais, ni de ReLU en HW pour le moment
- tests faits sur l'image dog.9499.jpg.rgba.planar

## Table:

| Name | Description | Time (s) | Frequency (MHz) | LUTs | FFs | BRAMs | DSPs | Cost | Pareto ? (Yes / No) |
|-----------------|-----------------|---------|---------|---------|---------|---------|---------|---------|---------|
| SW | macOS M1 Max | 0.457 | n/a | n/a | n/a | n/a | n/a | 0 | Yes |
| SW | SW only on PYNQ | 28.079 | x | n/a | n/a | n/a | n/a | 0 | no |
| HW | without optimization | 87.635 | 100 | 5785 | 6938 | 0 | 45 | 9,46 | No |
