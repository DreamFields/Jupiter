#pragma once

#include<memory>
#include<cassert>

#include"runtime/engine.h"
#include"editor/include/editor_ui.h"

namespace Mercury
{
    // class MercuryEngine; // 尽可能地避免使用前置声明。使用 #include 包含需要的头文件即可。
    class MercuryEditor
    {
    private:
        /* data */
    public:
        MercuryEditor();
        virtual ~MercuryEditor();

        void initailize(MercuryEngine* engine_runtime);
        void clear();

        void run();
    protected:
        MercuryEngine* m_engine_runtime{ nullptr };
        std::shared_ptr<EditorUI> m_editor_ui;
    };

} // namespace Mercury
