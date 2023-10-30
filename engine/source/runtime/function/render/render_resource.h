#pragma once

#include "runtime/function/render/render_resource_base.h"
#include <cstdint> // for uint8_t


namespace Mercury
{
    class RenderResource :public RenderResourceBase {
    public:
        void resetRingBufferOffset(uint8_t current_frame_index);
    };
} // namespace Mercury

