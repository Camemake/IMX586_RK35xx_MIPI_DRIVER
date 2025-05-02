# Sony IMX586 – V4L2 Driver for Rockchip RK35xx

Linux kernel V4L2 sub-device driver for the Sony IMX586 image sensor on Rockchip RK35xx SoCs (RK356x / RK3588) over 4‑lane MIPI‑CSI2.

## Features
- 4‑lane CSI‑2 D‑PHY (≤ 2.5 Gbps/lane)
- V4L2 subdev + media controller
- Mode table: 4000 × 3000 @ 30 fps (QBC)
- Regulator / GPIO / clock integration
- libcamera‑compatible

## Hardware

| Signal | Value |
|--------|-------|
| I²C addr | 0x1a |
| XCLK | 24 MHz (or 37.125 MHz) |
| RESET | board‑specific GPIO |
| Data‑lanes | 4 |

## Device‑tree

Add `imx586-rk35xx.dtsi` to your board DT and wire the `remote-endpoint` to the chosen MIPI controller:

```dts
#include "imx586-rk35xx.dtsi"
&mipi_dphy0_in {
      remote-endpoint = <&imx586_out>;
};
```

Adjust I²C bus, GPIO, and clock as required.

## Build & install

```bash
./setup.sh
```

DKMS builds the module and installs it for the running kernel.

Load manually or reboot:

```bash
sudo modprobe imx586
```

## Test

```bash
media-ctl -p
libcamera-hello --list-cameras
```

## Directory

```
imx586-rk35xx-driver/
├── imx586.c
├── imx586_reg_tables.h
├── imx586-rk35xx.dtsi
├── Makefile, Kconfig
├── dkms.conf, setup.sh
└── LICENSE
```

## TODO
- Register tables for 4K60 / 1080p120
- Add exposure / gain controls
- Optionally integrate AF VCM

© 2025 YOUR NAME — GPL‑2.0‑only
