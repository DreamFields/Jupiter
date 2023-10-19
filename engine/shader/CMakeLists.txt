set(TARGET_NAME ${SHADER_COMPILE_TARGET})

# CONFIGURE_DEPENDS告诉 CMake 关于配置过程的其他输入文件。如果修改了任何命名文件，则生成系统将重新运行 CMake 以重新配置该文件并再次生成生成系统。
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.comp"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.geom"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.tesc"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.tese"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.mesh"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.task"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.rgen"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.rchit"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.rmiss"
    "${CMAKE_CURRENT_SOURCE_DIR}/glsl/*.rcall")

SOURCE_GROUP("shaders" FILES ${SHADER_FILES})

set(SHADER_INCLUDE_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 设置generated文件夹作为SPIR-V 字节码文件的生成文件夹，如果没有就创建
set(GENERATED_SHADER_FOLDER "generated")

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER})
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER})
endif()

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/spv)
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}/spv)
endif()

# 该文件包含了一个cmake函数，用来将glsl文件生成SPIR-V 字节码文件
include(${MERCURY_ROOT_DIR}/cmake/ShaderCompile.cmake)
compile_shader(
    "${SHADER_FILES}"
    "${TARGET_NAME}"
    "${SHADER_INCLUDE_FOLDER}"
    "${GENERATED_SHADER_FOLDER}"
    "${glslangValidator_executable}") # glslangValidator_executable变量在engine/CMakeLists.txt中设置

set_target_properties("${TARGET_NAME}" PROPERTIES FOLDER "Engine")