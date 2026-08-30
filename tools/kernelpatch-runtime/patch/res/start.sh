g++ -o encrypt encrypt.cpp -O3 -std=c++17

./encrypt res/kpimg res/kpimg.enc

xxd -i res/kpimg.enc > include/kpimg_enc.h
xxd -i res/kptools-linux > include/kptools_linux.h
xxd -i res/kptools-android > include/kptools_android.h

g++ -std=c++17 main.cpp -o patch

D:\.gradle\android_sdk\ndk\28.0.13004108\ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=./jni/Android.mk
