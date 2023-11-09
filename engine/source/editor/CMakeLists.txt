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

# begin DCMTK
set(DCMTK_DIR ${LIBRARY_DIR}/dcmtk)
# Add include directories
include_directories(${DCMTK_DIR}/include)

# Add link directories
link_directories(${DCMTK_DIR}/lib)

# Add libraries
add_library(ofstd SHARED IMPORTED)
set_target_properties(ofstd PROPERTIES
    IMPORTED_LOCATION ${DCMTK_DIR}/bin/ofstd.dll
    IMPORTED_IMPLIB ${DCMTK_DIR}/lib/ofstd.lib)

add_library(oflog SHARED IMPORTED)
set_target_properties(oflog PROPERTIES
    IMPORTED_LOCATION ${DCMTK_DIR}/bin/oflog.dll
    IMPORTED_IMPLIB ${DCMTK_DIR}/lib/oflog.lib)

add_library(dcmdata SHARED IMPORTED)
set_target_properties(dcmdata PROPERTIES
    IMPORTED_LOCATION ${DCMTK_DIR}/bin/dcmdata.dll
    IMPORTED_IMPLIB ${DCMTK_DIR}/lib/dcmdata.lib)

add_library(zlib SHARED IMPORTED)
set_target_properties(zlib PROPERTIES
    IMPORTED_IMPLIB ${DCMTK_DIR}/lib/zlib.lib)
target_link_libraries(${TARGET_NAME} netapi32 wsock32 ofstd dcmdata)

# 将.dll文件复制到输出目录
configure_file(${DCMTK_DIR}/bin/ofstd.dll ${CMAKE_CURRENT_BINARY_DIR}/ofstd.dll COPYONLY)
configure_file(${DCMTK_DIR}/bin/oflog.dll ${CMAKE_CURRENT_BINARY_DIR}/oflog.dll COPYONLY)
configure_file(${DCMTK_DIR}/bin/dcmdata.dll ${CMAKE_CURRENT_BINARY_DIR}/dcmdata.dll COPYONLY)
# end DCMTK


# 将该模块保存到Engine目录下
set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "MercuryEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")