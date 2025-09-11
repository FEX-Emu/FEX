[English](https://github.com/FEX-Emu/FEX/blob/main/Readme.md)
# FEX —— 快速的x86模拟器前端
FEX和qemu-user以及box86类似，允许你在AArch64的host端运行x86和x86-64二进制程序。
FEX原生支持rootfs（作为guest程序的运行环境），所以无需使用chroot。同时支持thunklibs将guest程序所用到的库转发到host，例如：libGL。
FEX为guest程序提供Linux 5.0的接口（系统调用），同时支持AArch64和x86-64做为host。
FEX处于重度开发阶段，所以会有很多改善。


## 快速指引
### Ubuntu 20.04, 21.04, 21.10, 22.04
在终端执行以下命令添加PPA去安装FEX。

`curl --silent https://raw.githubusercontent.com/FEX-Emu/FEX/main/Scripts/InstallFEX.py --output /tmp/InstallFEX.py && python3 /tmp/InstallFEX.py && rm /tmp/InstallFEX.py`

这条命令将会引导你通过PPA安装FEX，然后下载FEX所需的RootFS。

Ubuntu下的PPA 随FEX月度发布更新。

### 其他系统
参考[这里](https://wiki.fex-emu.com/index.php/QuickStartGuide)

## 开始
FEX在ARMv8.0，ARMv8.1+和x86-64(支持AVX或更新处理器)硬件上进行过编译和运行测试。
不支持ARMv7以及老旧的x86处理器。
同时需要确保操作系统为Linux。FEX在Ubuntu 20.04，20.10和21.04以及Arch Linux上测试过。
在AArch64 host端，用户需要准备x86-64 RootFS[创建RootFS](#RootFS-Generation)。

### 源码导览
详见[源码大纲](docs/SourceOutline.md)。

### 编译依赖
* cmake (version 3.14 minimum)
* ninja-build
* clang (version 10 minimum for C++20)
* libglfw3-dev (For GUI)
* libsdl2-dev (For GUI)
* libepoxy-dev (For GUI)
* g++-x86-64-linux-gnu (For building thunks)
* nasm (only if building tests)

### 编译FEX
安装完依赖后，通过以下命令进行编译。
```Shell
git clone https://github.com/FEX-Emu/FEX.git
cd FEX
git submodule update --init
mkdir Build
cd Build
CC=clang CXX=clang++ cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release -DENABLE_LTO=True -DBUILD_TESTING=False -G Ninja ..
ninja
```

### 安装
```Shell
sudo ninja install
```

### 关于AArch64 Hosts
在AArch64使用binfmt_misc（执行下述命令）可以支持32位和64位x86程序直接运行。如果已经安装了box86 binfmt_misc配置，在FEX达到可用状态前我并不建议安装FEX进行替代。请确保install命令在下述命令前执行，不然binfmt_misc将依旧使用旧版本的FEX，即使FEX已经更新。
```Shell
sudo ninja binfmt_misc_32
sudo ninja binfmt_misc_64
```

### 更多信息
更多关于FEX和平台相关的设置信息请参考以下维基页面：
https://wiki.fex-emu.com/index.php/Development:Setting_up_FEX

### 创建RootFS
AArch64 host端需要一个rootfs去运行guest程序。参考以下维基页面从头开始创建一个rootfs
https://wiki.fex-emu.com/index.php/Development:Setting_up_RootFS
