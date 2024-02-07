# RefresherPS3

An on-console patcher for PS3 games.

[![Discord](https://img.shields.io/discord/1049223665243389953?label=Discord)](https://discord.gg/xN5yKdxmWG)

## Compiling

### Setting up the toolchain and libraries

Install the latest version of PSL1GHT using [ps3toolchain](https://github.com/ps3dev/ps3toolchain/), then install all the libraries from [ps3libraries](https://github.com/ps3dev/ps3libraries).

Install [PS3-SDL2](https://github.com/ultra0000/PS3-SDL2), see [this readme](https://github.com/ultra0000/PS3-SDL2/blob/main/README.PSL1GHT) on specific instructions.

### Cloning and compiling RefresherPS3

Clone RefresherPS3 with submodules, and enter the folder
```sh
$ git clone https://github.com/LittleBigRefresh/RefresherPS3 --recurse-submodules
$ cd RefresherPS3
```

Run the following commands to compile RefresherPS3 and generate a package file
```sh
$ make -j$(nproc)
$ make pkg
```

### Quick testing on RPCS3

The makefile specifies a target which will compile the app and run it under RPCS3, just run the following, and RPCS3 should automatically open to RefresherPS3.
```bash
$ make rpcs3
```

### Quick testing on real hardware

Install [ps3loadx](https://github.com/bucanero/ps3loadx) on your PS3, then open it. After its opened, just run the following, and RefresherPS3 will open on your console!
```bash
$ PS3LOAD=tcp:PS3_IP_GOES_HERE make run
```