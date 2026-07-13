# MPU-9250 Kalman Filter — SPI + DMA

Fuses the MPU-9250's accelerometer and gyroscope over SPI1+DMA into roll/pitch
angle estimates using a 2-state (angle + gyro bias) scalar Kalman filter, per
axis. Unlike the fixed-weight complementary filter, the blend between gyro
integration and the accelerometer measurement is recalculated every iteration
from a tracked uncertainty — the filter continuously decides "how much do I
trust the accelerometer right now" instead of always using the same ratio.

Builds directly on `examples/complementary_filter_spi_dma`: same sensor
reads, same accelerometer tilt-angle math, same `dt` measurement via
`start_timer()`/`get_tick()`. The difference is entirely in how the
gyro/accel readings get combined — see `drivers/mpu9250/README.md` for the
underlying read/convert math shared by all three fusion examples.

## State (per axis, roll and pitch each have their own)

| Variable | Meaning |
|---|---|
| `angle` | Estimated tilt angle (degrees) |
| `bias` | Estimated constant gyro bias (deg/s) — actively corrected, unlike the complementary filter which only ever integrates the raw, still-biased rate |
| `P00`, `P01`, `P10`, `P11` | 2x2 uncertainty (covariance) matrix — `P00`: uncertainty in `angle`, `P11`: uncertainty in `bias`, `P01`/`P10`: their correlation |

## Tuning Constants (shared across both axes — same physical sensor)

| Constant | Meaning | Effect of increasing |
|---|---|---|
| `Q_angle` | Distrust in the prediction model (gyro integration) | Faster convergence to the accelerometer, but noisier |
| `Q_bias` | Distrust in the bias estimate's stability | Bias adapts faster to real drift, but can chase noise if too high |
| `R_measure` | Distrust in the accelerometer measurement | Smoother output, but slower to correct/settle |

Current values (`Q_angle=1.8`, `Q_bias=1.0`, `R_measure=0.45`) were reached
by empirical tuning on hardware, not calculated — see Tuning below.

## Wiring

| Nucleo-64 Pin | MPU-9250 Pin | Notes |
|---|---|---|
| 3.3V | VCC | |
| GND | GND | |
| PA4 | NCS | Software chip-select (active-low) |
| PB3 | SCL/CLK | SPI1 SCK (AF5) |
| PB4 | SDA/SDO | SPI1 MISO (AF5) |
| PB5 | SDI | SPI1 MOSI (AF5) |

## Build and Flash

```bash
cmake -B build -DEXAMPLE=kalman_filter_spi_dma -G "MinGW Makefiles"
cmake --build build
openocd -f openocd.cfg -c "program build/stm32_drivers.hex verify reset exit"
```

## Expected Output

Open a serial terminal at **115200 baud, 8N1** on the ST-LINK COM port.

```
WHO_AM_I: 0x71 (expect 0x71)
roll_angle: 3 degrees   pitch angle: -5 degrees
```

With the board still, both angles should settle to a stable value (small
nonzero resting offsets are expected — see Calibration below) and stay
there. Tilting the board should track the motion without excessive lag or
visible jitter, once tuned per the process below.

## Tuning

Tuning is empirical — there's no closed-form correct value. A practical
order:

1. **Hold the board still.** If the printed angle jitters, `R_measure` is
   too low (or `Q_angle` too high) relative to the accelerometer's actual
   noise — raise `R_measure` or lower `Q_angle`.
2. **Quickly tilt and hold at a fixed angle.** Too slow to settle, or
   overshoots/oscillates — adjust `Q_angle`/`R_measure` in the opposite
   direction from step 1 until you find a middle ground.
3. **Slowly rotate back and forth.** Visible lag between real motion and
   the printed angle means still too conservative.
4. **Leave it still for a long period.** Slow residual drift over tens of
   seconds points at `Q_bias` being too low to track the true gyro bias.

Steps 1 and 2 are in direct tension — the goal is the least-noisy setting
that's still fast enough to be useful for your application.

## Calibration

A small constant resting offset (e.g. `pitch_angle` reading `-5°` when the
board is physically flat) is **not** a filter bug — the Kalman filter's
`bias` state only corrects *gyro* bias, not the accelerometer's own static
bias/mounting offset. Measure each axis's resting value once and subtract
it from `accel_roll_angle`/`accel_pitch_angle` if a true zero reference is
needed. Not yet implemented in this example.

## Build Requirements

Uses `atan2f`/`sqrtf` (`math.h`) and `%f`-style float printing, same as
`examples/mpu9250_accel_spi_dma`. Requires `-lm` and `-Wl,-u,_printf_float`,
both already set in the top-level `CMakeLists.txt`.

## Debugging Notes

If a future change to this file makes the filter seem unresponsive or
sluggish despite the tuning constants looking reasonable, verify `dt` is
actually nonzero before suspecting the tuning — add a temporary printf for
raw `dt`, `P00`, and the Kalman gain (`K0`). This example originally had a
bug where `tim0 = get_tick()` was placed *after* `delay()` instead of right
after the `dt` computation, silently zeroing `dt` every iteration and making
the filter appear to barely respond regardless of tuning — the same class
of ordering mistake as an earlier bug in `examples/mpu_gyro_spi_dma`.

## Limitations

- **No yaw** — same limitation as the complementary filter; roll/pitch use
  the accelerometer's gravity vector as an absolute reference, which yaw
  rotation doesn't affect. A magnetometer (unsupported on this board — see
  `drivers/mpu9250/README.md`) would be needed for drift-free yaw.
- **No static calibration** — see Calibration above.
