#include "mnist_recognizer.h"

#include <cmath>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace {

constexpr char kTag[] = "MNIST";
constexpr size_t kTensorArenaSize = 24 * 1024;

extern const uint8_t mnist_model_start[] asm("_binary_mnist_int8_tflite_start");
extern const uint8_t mnist_model_end[] asm("_binary_mnist_int8_tflite_end");

const tflite::Model *g_model = nullptr;
tflite::MicroInterpreter *g_interpreter = nullptr;
TfLiteTensor *g_input = nullptr;
TfLiteTensor *g_output = nullptr;
uint8_t *g_tensor_arena = nullptr;

template <typename T>
T clamp_value(T value, T low, T high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

}  // namespace

bool MnistRecognizer::init() {
  if (_initialized) {
    return true;
  }

  g_model = tflite::GetModel(mnist_model_start);
  if (g_model == nullptr) {
    ESP_LOGE(kTag, "Model pointer is null");
    return false;
  }
  if (g_model->version() != TFLITE_SCHEMA_VERSION) {
    ESP_LOGE(kTag, "Model schema mismatch: %d != %d", g_model->version(),
             TFLITE_SCHEMA_VERSION);
    return false;
  }

  if (g_tensor_arena == nullptr) {
    g_tensor_arena = static_cast<uint8_t *>(
        heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  }
  if (g_tensor_arena == nullptr) {
    ESP_LOGE(kTag, "Failed to allocate tensor arena");
    return false;
  }

  static tflite::MicroMutableOpResolver<3> resolver;
  static bool resolver_ready = false;
  if (!resolver_ready) {
    resolver.AddReshape();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();
    resolver_ready = true;
  }

  static tflite::MicroInterpreter static_interpreter(g_model, resolver,
                                                     g_tensor_arena,
                                                     kTensorArenaSize);
  g_interpreter = &static_interpreter;

  if (g_interpreter->AllocateTensors() != kTfLiteOk) {
    ESP_LOGE(kTag, "AllocateTensors failed");
    return false;
  }

  g_input = g_interpreter->input(0);
  g_output = g_interpreter->output(0);
  if (g_input == nullptr || g_output == nullptr) {
    ESP_LOGE(kTag, "Model tensors unavailable");
    return false;
  }

  ESP_LOGI(kTag, "Model loaded: %u bytes",
           (unsigned)(mnist_model_end - mnist_model_start));
  _initialized = true;
  return true;
}

bool MnistRecognizer::classify(const uint8_t *pixels, size_t len, int *digit_out,
                               int *confidence_out) {
  if (!pixels || len != kImageSize || !digit_out || !confidence_out) {
    return false;
  }
  if (!init()) {
    return false;
  }

  if (g_input->type != kTfLiteInt8 || g_output->type != kTfLiteInt8) {
    ESP_LOGE(kTag, "Expected int8 input/output tensors");
    return false;
  }

  const float input_scale = g_input->params.scale;
  const int input_zero_point = g_input->params.zero_point;
  for (size_t i = 0; i < kImageSize; ++i) {
    const float normalized = static_cast<float>(pixels[i]) / 255.0f;
    const int32_t quantized =
        static_cast<int32_t>(std::lround(normalized / input_scale)) + input_zero_point;
    g_input->data.int8[i] =
        static_cast<int8_t>(clamp_value<int32_t>(quantized, -128, 127));
  }

  if (g_interpreter->Invoke() != kTfLiteOk) {
    ESP_LOGE(kTag, "Invoke failed");
    return false;
  }

  int best_digit = 0;
  float best_score = -1.0f;
  const float output_scale = g_output->params.scale;
  const int output_zero_point = g_output->params.zero_point;
  for (int i = 0; i < 10; ++i) {
    const float score =
        (static_cast<int>(g_output->data.int8[i]) - output_zero_point) * output_scale;
    if (score > best_score) {
      best_score = score;
      best_digit = i;
    }
  }

  *digit_out = best_digit;
  *confidence_out =
      static_cast<int>(clamp_value(best_score * 100.0f, 0.0f, 100.0f));
  ESP_LOGI(kTag, "Prediction=%d confidence=%d%%", *digit_out, *confidence_out);
  return true;
}
