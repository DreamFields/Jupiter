#include "runtime/engine.h"

#include "runtime/function/global/global_context.h"
#include "engine.h"

namespace Mercury
{
    void MercuryEngine::startEngine(const std::string& config_file_path) {
        g_runtime_global_context.startSystems(config_file_path);

    }


    void MercuryEngine::shutdownEngine() {
        g_runtime_global_context.shutdownSystems();
    }


    void MercuryEngine::initialize()
    {
    }


    void MercuryEngine::clear()
    {
    }


    void MercuryEngine::run()
    {
    }

    bool MercuryEngine::tickOneFrame(float delta_time)
    {
        g_runtime_global_context.m_window_system->pollEvents();
        g_runtime_global_context.m_window_system->setTitle(
            std::string("Mercury - " + std::to_string(getFPS()) + " FPS").c_str()
        );
        const bool should_window_close = g_runtime_global_context.m_window_system->shouldClose();
        return !should_window_close;
    }

    float MercuryEngine::calculateDeltaTime()
    {
        float delta_time;
        {
            // c++11标准的最佳计时方法，可以精确到微秒
            using namespace std::chrono;

            steady_clock::time_point tick_time_point = steady_clock::now(); //now() 表示计时的那“一瞬间”
            duration<float> time_span = duration_cast<duration<float>>(tick_time_point - m_last_tick_time_point);
            delta_time = time_span.count(); //count( ) 用来返回时间

            m_last_tick_time_point = tick_time_point;

        }
        return delta_time;
    }

} // namespace Mercury
