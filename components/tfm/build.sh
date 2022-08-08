export PATH=/opt/gcc-arm-none-eabi-9-2020-q2-update/bin:$PATH

#rm -rf cmake_build
cd tfm
cmake -S . -B ../cmake_build -DTFM_PLATFORM=beken/bk7236 -DTFM_TOOLCHAIN_FILE=toolchain_GNUARM.cmake -DCMAKE_BUILD_TYPE=Debug -DTEST_S=ON -DTEST_NS=ON -DUSER=$USER
cmake --build ../cmake_build -- install
