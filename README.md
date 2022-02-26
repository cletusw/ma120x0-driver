# ma120x0-driver
Partial Raspberry Pi (Linux) ASoC driver for the MA12040 and MA12070 based on the MA120x0p one.

## Setup

```sh
sudo apt install build-essential device-tree-compiler
```

## Usage

```sh
make
make dts
```

Add to `/boot/config.txt`:

```
dtoverlay=ma120x0-example
```

Optionally add `force_eeprom_read=0` as well to disable any automatic overlay loading if connected to a HAT with EEPROM.

Currently the overlay installed by `make dts` expects the MA120x0 to be at I2C address 0x20.
