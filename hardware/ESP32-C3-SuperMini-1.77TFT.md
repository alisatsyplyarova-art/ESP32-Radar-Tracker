# ESP32-C3 LD-Radar — hardware profile

## Board identified from the user's photos

- MCU board: ESP32-C3 SuperMini.
- Display carrier: separate blue adapter PCB marked **1.77" TFT**.
- Display interface labels on the carrier: SPI-style `SCK`, `SDA`, `RES`, `RS`, `CS`, plus two `LEDA` pads.
- Likely panel format: 1.77-inch, 128x160. The controller is not marked in the supplied photos; ST7735/ST7735S is a working hypothesis, not yet verified.
- The adapter also exposes `GT_CS`, `GT_SI`, `GT_SO`, `GT_SCL`. Their function/controller is not confirmed.

## Adapter connector map (copied from PCB silkscreen)

The numbers below are the adapter's connector position numbers, **not ESP32 GPIO numbers**.

| Adapter position | PCB signal |
|---:|---|
| 1 | GND |
| 2 | VCC |
| 3 | SCK |
| 4 | SDA |
| 5 | RES |
| 6 | RS |
| 7 | CS |
| 8 | LEDA |
| 9 | LEDA |
| 10 | GT_CS |
| 11 | GT_SI |
| 12 | GT_SO |
| 13 | GT_SCL |
| 14 | GND |

## Important before assigning GPIO

The photos show the SuperMini and the display adapter separately; they do not show the two boards mated or the copper routing between adapter positions and SuperMini pads. Therefore the adapter-position-to-GPIO mapping cannot safely be inferred from these photos alone.

Do not treat adapter positions 1..14 as GPIO1..GPIO14. In particular, positions 1 and 14 are GND and position 2 is VCC.

Before finalizing `pins_c3.h`, verify the carrier's traces or provide a continuity table between adapter positions 1..14 and the SuperMini pads. Do not connect the radar UART until the display GPIO allocation is known.

## ESP32-C3 constraints

- GPIO logic is 3.3 V; do not apply 5 V to GPIO.
- GPIO8 and GPIO9 are strapping/boot-sensitive pins on common SuperMini variants; avoid assigning them to signals that can force an unsafe level during reset.
- GPIO18/GPIO19 are used by native USB on ESP32-C3; avoid them for peripherals if USB serial/programming is required.
- The classic ESP32 project's GPIO26/GPIO27 UART mapping does not exist on the C3.
- The LD2401/LD2450 UART can be routed to suitable free GPIOs using ESP32-C3 UART peripheral routing, after the display pin map is confirmed.

## Porting status

This branch is a hardware-specific port scaffold. The classic ESP32 firmware remains unchanged on `main`. Do not label a C3 build as tested until the display pin map, panel controller/initialization, sensor UART, and an Arduino compile have all been verified.
