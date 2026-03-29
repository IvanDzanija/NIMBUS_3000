#pragma once

#include <string.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "mnist_model.h"
#include "mnist_preprocess.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

static const char *TAG = "MNIST";

#define ARENA_SIZE (48 * 1024)

namespace mnist {

static uint8_t *s_arena = nullptr;
static tflite::MicroInterpreter *s_interpreter = nullptr;
static TfLiteTensor *s_input = nullptr;
static bool s_ready = false;

using MyResolver = tflite::MicroMutableOpResolver<5>;
static MyResolver s_resolver;

inline bool init() {
  if (s_ready) {
    return true;
  }

  tflite::InitializeTarget();

#if CONFIG_SPIRAM_USE_MALLOC
  s_arena = (uint8_t *)heap_caps_malloc(ARENA_SIZE, MALLOC_CAP_SPIRAM);
  if (!s_arena) {
    ESP_LOGW(TAG, "PSRAM alloc failed, falling back to DRAM");
    s_arena = (uint8_t *)heap_caps_malloc(ARENA_SIZE, MALLOC_CAP_8BIT);
  }
#else
  s_arena = (uint8_t *)heap_caps_malloc(ARENA_SIZE, MALLOC_CAP_8BIT);
#endif

  if (!s_arena) {
    ESP_LOGE(TAG, "Failed to allocate tensor arena (%d bytes)", ARENA_SIZE);
    return false;
  }

  const tflite::Model *model = tflite::GetModel(g_mnist_model_data);
  if (!model) {
    ESP_LOGE(TAG, "Model pointer is null");
    return false;
  }
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    ESP_LOGE(TAG, "Model schema version mismatch: got %lu, want %d",
             model->version(), TFLITE_SCHEMA_VERSION);
    return false;
  }

  s_resolver.AddConv2D();
  s_resolver.AddMaxPool2D();
  s_resolver.AddFullyConnected();
  s_resolver.AddSoftmax();
  s_resolver.AddReshape();

  static tflite::MicroInterpreter static_interp(model, s_resolver, s_arena,
                                                ARENA_SIZE);
  s_interpreter = &static_interp;

  if (s_interpreter->AllocateTensors() != kTfLiteOk) {
    ESP_LOGE(TAG, "AllocateTensors() failed");
    return false;
  }

  s_input = s_interpreter->input(0);
  if (!s_input || s_input->type != kTfLiteFloat32) {
    ESP_LOGE(TAG, "Unexpected input tensor type");
    return false;
  }

  ESP_LOGI(TAG, "Ready. Arena used: %u / %d bytes, model=%u bytes",
           (unsigned)s_interpreter->arena_used_bytes(), ARENA_SIZE,
           g_mnist_model_data_len);
  s_ready = true;
  return true;
}

inline int predict(const float pixels[MNIST_DIM * MNIST_DIM],
                   float *confidence = nullptr) {
  if (!s_ready) {
    ESP_LOGE(TAG, "Call init() first");
    return -1;
  }

  memcpy(s_input->data.f, pixels, MNIST_DIM * MNIST_DIM * sizeof(float));

  if (s_interpreter->Invoke() != kTfLiteOk) {
    ESP_LOGE(TAG, "Invoke() failed");
    return -1;
  }

  TfLiteTensor *output = s_interpreter->output(0);
  if (!output || output->type != kTfLiteFloat32) {
    ESP_LOGE(TAG, "Unexpected output tensor type");
    return -1;
  }

  int best = 0;
  float best_score = output->data.f[0];
  for (int i = 1; i < 10; i++) {
    if (output->data.f[i] > best_score) {
      best_score = output->data.f[i];
      best = i;
    }
  }

  if (confidence) {
    *confidence = best_score;
  }
  return best;
}

inline int predict_u8(const uint8_t pixels[MNIST_DIM * MNIST_DIM],
                      float *confidence = nullptr) {
  float float_pixels[MNIST_DIM * MNIST_DIM];

  for (int i = 0; i < (MNIST_DIM * MNIST_DIM); ++i) {
    float_pixels[i] = (float)pixels[i] / 255.0f;
  }

  return predict(float_pixels, confidence);
}

}  // namespace mnist
