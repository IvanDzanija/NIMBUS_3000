#pragma once

#include <stddef.h>
#include <stdint.h>

class MnistRecognizer {
 public:
  static constexpr size_t kImageWidth = 28;
  static constexpr size_t kImageHeight = 28;
  static constexpr size_t kImageSize = kImageWidth * kImageHeight;

  bool init();
  bool classify(const uint8_t *pixels, size_t len, int *digit_out,
                int *confidence_out);

 private:
  bool _initialized = false;
};
