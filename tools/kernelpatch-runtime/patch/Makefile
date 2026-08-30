CC = g++
ANDROID_CC = aarch64-linux-android-clang++
CXXFLAGS = -std=c++17 -Iinclude
OPTIMIZE = -O3
ANDROID_SYSROOT = $(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/sysroot
ANDROID_CC = aarch64-linux-android24-clang++
ANDROID_CFLAGS = -static-libstdc++ -fPIE -pie
ANDROID_LDFLAGS = -lc++_static -lc++fs -landroid_support

.PHONY: all android clean

all: patch

encrypt: encrypt.cpp
	$(CC) -o $@ $< $(OPTIMIZE) $(CXXFLAGS)

res/kpimg.enc: encrypt res/kpimg
	./encrypt res/kpimg res/kpimg.enc

include/kpimg_enc.h: res/kpimg.enc
	mkdir -p include
	xxd -i $< > $@

include/kptools_linux.h: res/kptools-linux
	mkdir -p include
	xxd -i $< > $@

include/kptools_android.h: res/kptools-android
	mkdir -p include
	xxd -i $< > $@

generate_headers: include/kpimg_enc.h include/kptools_linux.h include/kptools_android.h

patch: main.cpp generate_headers
	$(CC) $(CXXFLAGS) main.cpp -o $@

android: generate_headers
	$(ANDROID_CC) $(CXXFLAGS) \
	--target=aarch64-none-linux-android24 \
	--sysroot=$(ANDROID_SYSROOT) \
	main.cpp -o patch-android \
	-pie
	
android-static: generate_headers
	$(ANDROID_CC) $(CXXFLAGS) $(ANDROID_CFLAGS) \
	--target=aarch64-none-linux-android24 \
	--sysroot=$(ANDROID_NDK_HOME)/toolchains/llvm/prebuilt/linux-x86_64/sysroot \
	main.cpp -o patch-android \
	$(ANDROID_LDFLAGS)

clean:
	rm -f encrypt patch patch-android
	rm -f res/kpimg.enc
	rm -f include/*.h