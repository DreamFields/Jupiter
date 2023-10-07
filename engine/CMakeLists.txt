# 设置目录变量
set(ENGINE_ROOT_DIR "${MERCURY_ROOT_DIR}/engine")
set(LIBRARY_DIR "${ENGINE_ROOT_DIR}/library")

# 设置vulkan的include目录
set(vulkan_include ${LIBRARY_DIR}/VulkanSDK/include)

# 对于不同平台拥有不同设置
if(WIN32)
    set(vulkan_lib ${LIBRARY_DIR}/VulkanSDK/lib/Win32/vulkan-1.lib)
    set(glslangValidator_executable ${LIBRARY_DIR}/VulkanSDK/bin/Win32/glslangValidator.exe)
    add_compile_definitions("MERCURY_VK_LAYER_PATH=${LIBRARY_DIR}/VulkanSDK/bin/Win32")
elseif(UNIX)
    if(APPLE)
      set(vulkan_lib ${LIBRARY_DIR}/VulkanSDK/lib/MacOS/libvulkan.1.dylib)
      set(glslangValidator_executable ${LIBRARY_DIR}/VulkanSDK/bin/MacOS/glslangValidator)
      add_compile_definitions("MERCURY_VK_LAYER_PATH=${LIBRARY_DIR}/VulkanSDK/bin/MacOS")
      add_compile_definitions("MERCURY_VK_ICD_FILENAMES=${LIBRARY_DIR}/VulkanSDK/bin/MacOS/MoltenVK_icd.json")
    else()
      set(vulkan_lib ${LIBRARY_DIR}/VulkanSDK/lib/Linux/libvulkan.so.1)
      set(glslangValidator_executable ${LIBRARY_DIR}/VulkanSDK/bin/Linux/glslangValidator)
      add_compile_definitions("MERCURY_VK_LAYER_PATH=${LIBRARY_DIR}/VulkanSDK/bin/Linux")
    endif()
else()
  message(FATAL_ERROR "Unknown Platform")
endif()

add_subdirectory(library)

add_subdirectory(source/runtime)
add_subdirectory(source/editor)