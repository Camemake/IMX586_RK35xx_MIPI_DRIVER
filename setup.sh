#!/usr/bin/env bash
set -e

echo "[imx586] installing prerequisites..."
sudo apt update
sudo apt install -y dkms raspberrypi-kernel-headers

echo "[imx586] adding dkms module..."
sudo dkms add  -m imx586 -v 0.1 || true
sudo dkms build -m imx586 -v 0.1
sudo dkms install -m imx586 -v 0.1

echo "[imx586] enabling device‑tree overlay..."
if ! grep -q "^dtoverlay=imx586" /boot/firmware/config.txt; then
	echo "dtoverlay=imx586,4lane" | sudo tee -a /boot/firmware/config.txt
fi

echo "Done. Reboot your Pi 5 and run: libcamera-hello --list-cameras"
