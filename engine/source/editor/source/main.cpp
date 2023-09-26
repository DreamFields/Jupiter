#include <filesystem>
#include <iostream>

#include "runtime/engine.h"
#include "editor/include/editor.h"

int main(int argc, char** argv) {
    // 获得可执行文件名称与config文件目录
    std::filesystem::path executable_path(argv[0]);
    std::filesystem::path config_file_path = executable_path.parent_path() / "";

    // 引擎创建与初始化
    Mercury::MercuryEngine* engine = new Mercury::MercuryEngine();
    engine->startEngine(config_file_path.generic_string());
    engine->initialize();

    // 编辑器创建与初始化
    Mercury::MercuryEditor* editor = new Mercury::MercuryEditor();
    editor->initailize(engine);

    // 运行编辑器
    editor->run();

    // 退出编辑器，引擎
    editor->clear();
    engine->clear();
    engine->shutdownEngine();

    return 0;
}