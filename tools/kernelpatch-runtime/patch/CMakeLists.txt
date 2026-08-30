cmake_minimum_required(VERSION 3.22)
project(patch_project)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 主机编译部分
if(NOT ANDROID)
    # 创建构建目录中的必要文件夹
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/res)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include)

    # 编译encrypt工具（仅主机需要）
    add_executable(encrypt encrypt.cpp)
    target_compile_options(encrypt PRIVATE -O3)

    # 生成加密文件
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/res/kpimg.enc
        COMMAND $<TARGET_FILE:encrypt> 
                ${CMAKE_CURRENT_SOURCE_DIR}/res/kpimg 
                ${CMAKE_CURRENT_BINARY_DIR}/res/kpimg.enc
        DEPENDS encrypt
        COMMENT "Generating encrypted kpimg"
    )

    # 生成头文件的自定义命令
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/kpimg_enc.h
        COMMAND xxd -i ${CMAKE_CURRENT_BINARY_DIR}/res/kpimg.enc 
                > ${CMAKE_CURRENT_BINARY_DIR}/include/kpimg_enc.h
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/res/kpimg.enc
    )

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/kptools_linux.h
        COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/res/kptools-linux 
                > ${CMAKE_CURRENT_BINARY_DIR}/include/kptools_linux.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/kptools-linux
    )


if(NOT ANDROID)
    add_custom_target(generate_headers ALL
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/include/kpimg_enc.h
            ${CMAKE_CURRENT_BINARY_DIR}/include/kptools_linux.h
    )
else()
    add_custom_target(generate_headers ALL
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/include/kpimg_enc.h
            ${CMAKE_CURRENT_BINARY_DIR}/include/kptools_android.h
    )
endif()

    # 编译主程序（主机版本）
    add_executable(patch main.cpp)
    target_include_directories(patch PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
    add_dependencies(patch generate_headers)
    target_link_libraries(patch PRIVATE -static)
endif()

# Android可执行文件配置
if(ANDROID)
    # 生成Android头文件
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/include/kptools_android.h
        COMMAND xxd -i ${CMAKE_CURRENT_SOURCE_DIR}/res/kptools-android 
                > ${CMAKE_CURRENT_BINARY_DIR}/include/kptools_android.h
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/kptools-android
    )

    # 添加Android可执行文件
    add_executable(patch_android main.cpp)
    target_include_directories(patch_android PRIVATE 
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )

    # 静态链接配置
    set_target_properties(patch_android PROPERTIES
        OUTPUT_NAME "patch_android"
        ANDROID_ABI ${ANDROID_ABI}
        ANDROID_PLATFORM android-24
        ANDROID_STL "c++_static"
    )

    set_target_properties(patch_android PROPERTIES
        ANDROID_STL "c++_static"
        LINK_FLAGS "-static"
    )
    
    target_link_libraries(patch_android PRIVATE
        -lc++_static
    )
    
endif()