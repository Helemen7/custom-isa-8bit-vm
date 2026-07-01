#pragma once

#include <cstdint>

#include <cstddef>

namespace VMconf {
inline constexpr std::size_t PROGRAM_STACK_SIZE_BYTES{1000};
inline constexpr std::size_t PROGRAM_HEAP_STARTING_SIZE_BYTES{1000};
inline constexpr std::size_t FRAMEBUFFER_X_SIZE{64};
inline constexpr std::size_t FRAMEBUFFER_Y_SIZE{64};
inline constexpr std::size_t FRAMEBUFFER_TOT_SIZE{FRAMEBUFFER_X_SIZE *
						  FRAMEBUFFER_Y_SIZE};

inline constexpr std::size_t FRAMEBUFFER_WINDOW_SCALE{10};
inline constexpr std::size_t FRAMEBUFFER_WINDOW_WIDTH{FRAMEBUFFER_WINDOW_SCALE *
						      FRAMEBUFFER_X_SIZE};
inline constexpr std::size_t FRAMEBUFFER_WINDOW_HEIGHT{
    FRAMEBUFFER_WINDOW_SCALE * FRAMEBUFFER_Y_SIZE};

enum Interface { HEADLESS, GUI };

constexpr uint32_t COLORS[] = {
    0xFF000000, // Black
    0xFFFFFFFF, // White
    0xFFFF0000, // Red
    0xFF00FF00, // Green
    0xFF0000FF, // Blue
    0xFFFFCC00, // Yellow
    0xFFFF00FF, // Magenta
    0xFF00FFFF, // Cyan
};

} // namespace VMconf
