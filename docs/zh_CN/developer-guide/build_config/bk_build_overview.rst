构建概述
=====================

:link_to_translation:`en:[English]`

基本概念
-------------------------

 - ``项目`` - 特指 armino/projects/ 下一个目录或者子目录，其中包含了构建可执行应用程序所需的全部文件和配置，以及其他支持型文件，例如分区表、数据/文件系统分区和引导程序。
 - ``项目配置`` - 保存在项目根目录下名为 sdkconfig 的文件中，可以通过 make menuconfig 进行修改，且一个项目只能包含一个项目配置。
 - ``应用程序`` - 是由 ARMINO 构建得到的可执行文件。一个项目通常会构建两个应用程序：项目应用程序（可执行的主文件，即用户自定义的固件）和引导程序（启动并初始化项目应用程序）。
 - ``组件`` - 是模块化且独立的代码，会被编译成静态库（.a 文件）并链接到应用程序。
 - ``目标`` - 特指运行构建后应用程序的硬件设备，如 bk7231n，bk7256 等。

项目示例
-------------------------

::

    - armino/
        - components/
        - include/
        - middleware/
            - arch/
                - bk7231n/
                    - bk7231n.defconfig
                    - ...
        - projects/
            - my_project/
                - CMakeLists.txt
                - sdkconfig.defaults
                - sdkconfig.defaults.bk7231n
                - sdkconfig.debug
                - sdkconfig.release
                - Kconfig.projbuild                
                - main/
                    - main.c
                    - Kconfig
                    - CMakeLists.txt
                - components/
                    - c1/
                        - c1.c
                        - Kconfig
                        - CMakeLists.txt
                    - c2/


使用 make 触发构建
-------------------------

在 armino 根目录下运行　make， 最常用的 make 命令如下：

 - ``make bkxxx`` - 构建目标 bkxxx，项目为默认项目 armino/projects/legacy_app。
 - ``make bkxxx`` PROJECT=a/b -　 构建 bkxxx，项目为 armino/projects/a/b
 - ``make clean`` - 清理

make 构建是通过 armino/Makefile 间接调用 armino 工具来进行构建，它省去了使用 armino 工具构建时要
设置环境变量的麻烦，推荐您使用这种方式进行构建。

使用 armino　工具触发构建
-------------------------

使用 armino 工具构建分两步：

 - 在 armino 根目录下运行 ``. export.sh`` 设置 armino 构建所需要的环境变量。
 - 运行 armino 工具进行构建。

armino 工具需要在项目根目录下运行，常用的命令如下：

 - ``armino set-target bkxxx`` - 设置要构建的目标，默认目标为 bk7231n。目标设定后，armino build 每次都只需要设置一次即可。
 - ``armino build`` - 构建目标。
 - ``armino clean`` - 会把构建输出的文件从构建目录中删除，从而清理整个项目。下次构建时会强制“重新完整构建”这个项目。清理时，不会删除 CMake 配置输出及其他文件。
 - ``armino fullclean`` - 会将整个 build 目录下的内容全部删除，包括所有 CMake 的配置输出文件。下次构建项目时，
   CMake 会从头开始配置项目。请注意，该命令会递归删除构建目录下的 所有文件，请谨慎使用。项目配置文件不会被删除。

使用 cmake 触发构建
-------------------------

使用 Ninja Makefile 构建::

  cd $PROJECT_PATH
  mkdir -p build
  cd build
  cmake .. -G Ninja
  ninja

使用 GNU Makefile 构建::

  cd $PROJECT_PATH
  mkdir -p build
  cd build
  cmake ..
  cmake --build


