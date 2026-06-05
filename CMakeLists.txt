cmake_minimum_required(VERSION 3.13)
project(LLVMPass)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# ================= 第一部分：编译两个 Pass 插件 =================
add_library(ChangePass MODULE HelloWorld_change.cpp)
set_target_properties(ChangePass PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

add_library(ObfuscatePass MODULE HelloWorld_obfuscate.cpp)
set_target_properties(ObfuscatePass PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
)

# ================= 第二部分：生成 LLVM IR 和应用 Pass =================

# 1. 将 test.c 编译为 LLVM IR
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test.ll
    COMMAND clang -Xclang -disable-O0-optnone -O0 -S -emit-llvm
            -Xclang -disable-llvm-passes
            ${CMAKE_CURRENT_SOURCE_DIR}/test.c
            -o ${CMAKE_CURRENT_BINARY_DIR}/test.ll
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/test.c
    COMMENT "正在将 test.c 编译为 LLVM IR (test.ll)..."
)

add_custom_target(gen_test_ll DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test.ll)

# 2. 使用 ChangePass（加法 → 减法）
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_change.ll
    COMMAND opt -load-pass-plugin=$<TARGET_FILE:ChangePass>
            -passes=simple-obf
            ${CMAKE_CURRENT_BINARY_DIR}/test.ll
            -o ${CMAKE_CURRENT_BINARY_DIR}/test_change.ll
    DEPENDS gen_test_ll ChangePass
    COMMENT "正在使用 ChangePass 进行混淆 (加法变减法)..."
)

add_custom_target(gen_test_change_ll DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_change.ll)

# 3. 使用 ObfuscatePass（加法 → a - (-b)）
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/test_obfuscate.ll
    COMMAND opt -load-pass-plugin=$<TARGET_FILE:ObfuscatePass>
            -passes=simple-obf
            ${CMAKE_CURRENT_BINARY_DIR}/test.ll
            -o ${CMAKE_CURRENT_BINARY_DIR}/test_obfuscate.ll
    DEPENDS gen_test_ll ObfuscatePass
    COMMENT "正在使用 ObfuscatePass 进行混淆 (a - (-b))..."
)

add_custom_target(gen_test_obfuscate_ll DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/test_obfuscate.ll)

# ================= 第三部分：生成可执行文件 =================

# 使用 clang 直接从 .ll 文件生成可执行文件（推荐方式）
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/app_change
    COMMAND clang -O0 -g ${CMAKE_CURRENT_BINARY_DIR}/test_change.ll
            -o ${CMAKE_CURRENT_BINARY_DIR}/app_change
    DEPENDS gen_test_change_ll
    COMMENT "正在生成可执行文件 app_change..."
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/app_obfuscate
    COMMAND clang -O0 -g ${CMAKE_CURRENT_BINARY_DIR}/test_obfuscate.ll
            -o ${CMAKE_CURRENT_BINARY_DIR}/app_obfuscate
    DEPENDS gen_test_obfuscate_ll
    COMMENT "正在生成可执行文件 app_obfuscate..."
)

# 添加 ALL 目标，执行 cmake --build . 时自动构建
add_custom_target(app_change ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/app_change)
add_custom_target(app_obfuscate ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/app_obfuscate)

# 可选：添加一个总目标，一次性生成两个可执行文件
add_custom_target(all_apps ALL DEPENDS app_change app_obfuscate)
