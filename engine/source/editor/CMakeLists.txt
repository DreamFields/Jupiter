# 添加构建目标
set(TARGET_NAME MercuryEditor)

# 收集对应目录的文件到对应变量
file(GLOB EDITOR_HEADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
file(GLOB EDITOR_RESOURCE CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/resource/editor.rc)

# 在IDE里面给编译需要的文件归类
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_RESOURCE})

add_executable(${TARGET_NAME} ${EDITOR_HEADERS} ${EDITOR_SOURCES} ${EDITOR_RESOURCE})

# Runtime已经作为静态库被构建了，所以要链接过来
target_link_libraries(${TARGET_NAME} MercuryRuntime)

# 将该模块保存到Engine目录下
set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "MercuryEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")