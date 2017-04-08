# Cross-platform sandboxing utility

Runs an executable in a controlled, isolated mode,
setting tight limits on both permissions and resources.

Run with `-h` switch to get help.

Designed as part of [CATS](https://github.com/klenin/cats-judge) programming contest control system.

## Features

* High-precision resource control: time to milliseconds, memory to kilobytes.
* Low interference, stable and repeatable measurements.
* Multiple platforms: Windows XP to Windows 10, Windows Server, various Linux flavours, OS X, OpenBSD
* Isolated execution mode.
* Compatibility layer for other sandbox solutions.
* Running multiple processes simultaneously, connected through generalized pipes.
* Detailed, easily parseable reports.

## Compatibility

Use `--legacy` switch or set `SP_LEGACY` environment variable to choose command-line interface.

## Building

### Visual Studio 2013+

```
git clone git@github.com:klenin/Spawner.git
cd Spawner
git submodule init
git submodule update
mkdir build
cd build
cmake ..
sp.sln
```

Visual Studio 2013+ solution `sp.sln` will be generated in `./Spawner/build`.

### GCC

There are numerous gcc distributions for Windows.
- [MinGW](http://www.mingw.org/) is supported
- [MinGW-w64](http://sourceforge.net/projects/mingw-w64/) has not been tested
- [Nuwen MinGW](http://nuwen.net/mingw.html) is supported
- [MSYS2](https://msys2.github.io/) is supported

Basic steps are the same:

```
git clone git@github.com:klenin/Spawner.git
cd Spawner
git submodule init
git submodule update
mkdir build
cd build
```

Now g++ and make must be in your PATH.

With Nuwen-MinGW you might need to use
```
set LIB=C:\Nuwen-MinGW\MinGW\x86_64-w64-mingw32\lib
```
in order for cmake to find libuserenv.

With MinGW use:
```
cmake -G "MinGW Makefiles" -D CMAKE_CXX_COMPILER=g++.exe -D CMAKE_MAKE_PROGRAM=make.exe ..
make
```

And with MSYS2:
```
cmake -G "MSYS Makefiles"  -D CMAKE_CXX_COMPILER=g++ -D CMAKE_MAKE_PROGRAM=make ..
make
```

If you are using 64-bit system and want to build 32-bit:
```
cmake -D BIT32=True ..
make
```
