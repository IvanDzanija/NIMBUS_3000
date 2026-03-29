#pragma once

#include <stdint.h>
#include <string.h>

#define CANVAS_X 0
#define CANVAS_Y 0
#define CANVAS_W 190
#define CANVAS_H 150
#define MNIST_DIM 28

extern "C" uint16_t framebuf_get_pixel(int x, int y);

static inline uint8_t rgb565_to_gray(uint16_t color) {
  uint8_t r = ((color >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((color >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (color & 0x1F) * 255 / 31;
  return (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
}

static inline void preprocess_canvas(float out[MNIST_DIM * MNIST_DIM]) {
  for (int row = 0; row < MNIST_DIM; row++) {
    for (int col = 0; col < MNIST_DIM; col++) {
      int src_x = CANVAS_X + (col * CANVAS_W) / MNIST_DIM;
      int src_y = CANVAS_Y + (row * CANVAS_H) / MNIST_DIM;

      uint16_t raw = framebuf_get_pixel(src_x, src_y);
      uint8_t gray = rgb565_to_gray(raw);

      out[row * MNIST_DIM + col] = 1.0f - (gray / 255.0f);
    }
  }
}
