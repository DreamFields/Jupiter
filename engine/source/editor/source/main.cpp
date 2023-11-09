#include <filesystem>
#include <iostream>
#include <string>

#include "runtime/engine.h"
#include "editor/include/editor.h"

// #include <dcmtk/dcmdata/dctk.h>
#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmdata/dctk.h"

int main(int argc, char** argv) {
    // DICOM解析
    DcmFileFormat fileformat;
    OFCondition status = fileformat.loadFile("C:\\Users\\Dream\\Documents\\00.Dicom\\ede6fe9eda6e44a98b3ad20da6f9116a Anonymized29\\Unknown Study\\CT Head 5.0000\\CT000000.dcm");
    DcmDataset* dataset = fileformat.getDataset();
    if (!status.good())
    {
        std::cout << "Load Dimcom File Error: " << status.text() << std::endl;
        return -1;
    }
    OFString PatientName;
    status = fileformat.getDataset()->findAndGetOFString(DCM_PatientName, PatientName);
    if (status.good())
    {
        std::cout << "Get PatientName:" << PatientName << std::endl;
    }
    else
    {
        std::cout << "Get PatientName Error:" << status.text() << std::endl;
        return -1;
    }
    // 获取图像像素数据
    const Uint16 * pixelData = NULL;
    dataset->findAndGetUint16Array(DCM_PixelData, pixelData);



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