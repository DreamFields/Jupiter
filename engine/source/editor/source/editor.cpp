#include "editor/include/editor.h"
#include "runtime/function/global/global_context.h"

namespace Mercury
{
    MercuryEditor::MercuryEditor() {}

    MercuryEditor::~MercuryEditor(){}

    void MercuryEditor::initailize(MercuryEngine* engine_runtime) {
        assert(engine_runtime); // 保证engine被正确初始化
        this->m_engine_runtime = engine_runtime;

        // 通过全局上下文的信息，初始化编辑器界面
        m_editor_ui = std::make_shared<EditorUI>();
        WindowUIInitInfo ui_init_info = { g_runtime_global_context.m_window_system };
        m_editor_ui->initialize(ui_init_info);
    }

    void MercuryEditor::clear(){}

    void MercuryEditor::run() {
        assert(m_engine_runtime);
        assert(m_editor_ui);
        float delta_time;
        while (true) {
            delta_time = m_engine_runtime->calculateDeltaTime();

            // do sth.

            // 退出编辑器
            if (!m_engine_runtime->tickOneFrame(delta_time)) return;

        }
    }
} // namespace Mercury
