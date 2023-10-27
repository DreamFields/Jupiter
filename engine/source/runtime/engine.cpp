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
        calculateFPS(delta_time);

        rendererTick(delta_time);
    
        g_runtime_global_context.m_window_system->pollEvents();
        g_runtime_global_context.m_window_system->setTitle(
            std::string("Mercury - " + std::to_string(getFPS()) + " FPS").c_str()
        );
        const bool should_window_close = g_runtime_global_context.m_window_system->shouldClose();
        return !should_window_close;
    }

    const float MercuryEngine::s_fps_alpha = 1.f / 100;
    void MercuryEngine::calculateFPS(float delta_time) {
        ++m_frame_count;

        if (1 == m_frame_count) {
            m_average_duration = delta_time;
        }
        else {
            // 上一帧的m_average_duration与delta_time以99：1的权重求加权平均得到当前帧的m_average_duration
            m_average_duration = m_average_duration * (1 - s_fps_alpha) + delta_time * s_fps_alpha;
        }
        m_fps = static_cast<int>(1.0f / m_average_duration);

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


    bool MercuryEngine::rendererTick(float delta_time) {
        g_runtime_global_context.m_render_system->tick(delta_time);
        return true;
    }

} // namespace Mercury
