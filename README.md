## sxlab-fortress

# SxLab Fortress

SxLab Fortress synth schematics and firmware

SxLab Fortress is a digital portable modular synth inspired by the phylosophy of sunch synthesizers as Bastl Kastle and Korg Volca Modular.  It consists of 4 separate modules (or blocks), that can be connected to each other with DuPont wires in any way you like, creating both typical and unusual combinations.

All fimware is compiled with Stm32CubeIDE 1.6.1 with -Ofast (although some files and code blocks have -O0 setting in IDE or via pragmas)

Additional info: https://en.sxlab.ru/fortress, https://sxlab.ru/fortress

![SxLab Fortress](Fortress.png?raw=true "SxLab Fortress")

### References:
- STM32F103 MSD Bootloader (modified here for F303): https://github.com/sfyip/STM32F103_MSD_BOOTLOADER
- The art of VA Filter design by Vadim Zavalishin https://noisehack.com/research/VAFilterDesign_1.0.3.pdf
- Cytomic SVF implementation https://gist.github.com/hollance/2891d89c57adc71d9560bcf0e1e55c4b
- Nonlinear filter approximation method: https://www.kvraudio.com/forum/viewtopic.php?t=349859

### Files:
- **Schematic/Fortress-schematic-v4.PDF** - v4 (new jacks) schematic
- **Hex/Fortress-firmware-bootloader.hex** - USB MSD F303 Bootloader for SxLab Fortress hex
- **Hex/Fortress-firmware-v1p1.hex** - v1p1 compiled firmware
- **Hex/Fortress-firmware-v1p2.hex** - v1p2 compiled firmware, see patch notes here: https://files.sxlab.ru/fortress-fw.html
- **FW-Bootloader** - USB MSD F303 Bootloader for SxLab Fortress source code
- **FW-Main** - Main current firmware (v1p2) source code

Additional source code files description coming soon.