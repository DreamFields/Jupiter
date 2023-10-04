# 设置目录变量
set(ENGINE_ROOT_DIR "${MERCURY_ROOT_DIR}/engine")
set(LIBRARY_DIR "${ENGINE_ROOT_DIR}/library")

add_subdirectory(library)

add_subdirectory(source/runtime)
add_subdirectory(source/editor)