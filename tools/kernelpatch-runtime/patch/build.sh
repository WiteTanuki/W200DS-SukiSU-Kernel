#!/bin/bash
# 设置NDK路径（示例路径，请根据实际情况修改）
export ANDROID_NDK=ANDROID_NDK=/root/.android/sdk/ndk/28.0.13004108

xxd -i res/kpimg.enc > include/kpimg_enc.h
xxd -i res/kptools-linux > include/kptools_linux.h
xxd -i res/kptools-android > include/kptools_android.h

# 创建构建目录
rm -rf build-android
mkdir -p build-android
cd build-android

# 生成编译配置
cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-33 \
    -DANDROID_STL=c++_static

cmake --build .

cd ..

rm -rf build-linux
mkdir -p build-linux
cd build-linux

cmake .. && make
mv patch patch_linux