# Linux driver for Jomaa LTP-03 multitouch trackpad

## General information

Driver supports 4 fingers. Tested on kernel version 6.1.28-1-lts.

## Build and installation

```console
git clone 


dkms add linux/drivers/hid
dkms build hid-jomaa-dkms/6.1+jomaa
dkms install hid-jomaa-dkms/6.1+jomaa

modprobe hid-jomaa
```
