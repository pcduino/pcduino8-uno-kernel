# How to compile this kernel source

This tutorial will tell you how to compile this kernel source code for pcDuino8 Uno and generate a uImage which includes kernel and uboot.

## PC: Ubuntu 14.04 (X64)
### 1. Install related tools
```
wget https://s3.amazonaws.com/pcduino/Tools/gcc-linaro-arm-linux-gnueabihf-4.7-2013.04-20130415_linux.tar.bz2
tar -xvf gcc-linaro-arm-linux-gnueabihf-4.7-2013.04-20130415_linux.tar.bz2
export PATH=$PATH:<your gcc-linaro-arm-linux-gnueabihf-4.7 path>/bin
sudo apt-get install libc6:i386 libstdc++6:i386 libncurses5:i386 zlib1g:i386
sudo apt-get install libncurses5-dev libncursesw5-dev device-tree-compiler u-boot-tools
```
Note: gcc-linaro-arm-linux-gnueabihf-4.7 is recommanded.

### 2. Install other tools
```
wget http://ftp.gnu.org/gnu/gawk/gawk-3.1.6.tar.bz2
tar -xvf gawk-3.1.6.tar.bz2
cd gawk-3.1.6
./configure
make
sudo make install
```

### 3. Compile Source Code
```
./build.sh config
	Welcome to mkscript setup progress
All available chips:
   0. sun6i
   1. sun8iw6p1
   2. sun9iw1p1
Choice: 1
All available platforms:
   0. android
   1. dragonboard
   2. linux
Choice: 2
not set business, to use default!
LICHEE_BUSINESS=
using kernel 'linux-3.4':
All available boards:
   0. eagle-p1
   1. eagle-p1-secure
   2. eagle-tvd-perf3
   3. pcduino8
   4. pcduino8-linux
Choice: 4
```

After all you will get uImage file at linux-3.4/output directory, copy uImage to the second partition of SD card for pcDuino8 Uno and replace the old one.

**Note: Pleae backup your old uImage file, if system crash, you can restore it again!**


