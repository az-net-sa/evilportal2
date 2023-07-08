#include "esp_camera.h"

// Pin definitions
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// Camera configuration
camera_config_t config;

// Function prototypes
void handleSerialInput();
void initializeCamera();
void processImage(camera_fb_t* fb);
void ditherImage(camera_fb_t* fb, int dt);
bool isDarkBit(uint8_t bit);

// Serial input flags
bool disableDithering = false;
bool invert = false;
bool rotated = false;
bool stopStream = false;
// Dithering type:
//    0 = Floyd Steinberg (default)
//    1 = Atkinson
int dtType = 0;


void setup() {
  Serial.begin(230400);
  initializeCamera();
}

void loop() {
  handleSerialInput();

  if (stopStream) {
    return;
  }

  // Frame buffer.
  camera_fb_t* fb = esp_camera_fb_get();

  if (!fb) {
    return;
  }

  processImage(fb);

  esp_camera_fb_return(fb);
  fb = NULL;
  delay(50);
}

void handleSerialInput() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    sensor_t* cameraSensor = esp_camera_sensor_get();

    switch (input) {
      case '>': // Toggle dithering.
        disableDithering = !disableDithering;
        break;
      case '<': // Toggle invert.
        invert = !invert;
        break;
      case 'B': // Add brightness.
        cameraSensor->set_contrast(cameraSensor, cameraSensor->status.brightness + 1);
        break;
      case 'b': // Remove brightness.
        cameraSensor->set_contrast(cameraSensor, cameraSensor->status.brightness - 1);
        break;
      case 'C': // Add contrast.
        cameraSensor->set_contrast(cameraSensor, cameraSensor->status.contrast + 1);
        break;
      case 'c': // Remove contrast.
        cameraSensor->set_contrast(cameraSensor, cameraSensor->status.contrast - 1);
        break;
      case 'D': // Use Floyd Steinberg dithering.
        dtType = 0;
        break;
      case 'd': // Use Atkinson dithering.
        dtType = 1;
        break;
      case 'M': // Toggle Mirror
        cameraSensor->set_hmirror(cameraSensor, !cameraSensor->status.hmirror);
        break;
      case 'S': // Start stream.
        stopStream = false;
        break;
      case 's': // Stop stream.
        stopStream = true;
        break;
      default:
        break;
    }
  }
}

void initializeCamera() {
  // Set camera configuration
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_GRAYSCALE;
  config.frame_size = FRAMESIZE_QQVGA;
  config.fb_count = 1;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  // Set high contrast to make dithering easier
  sensor_t* s = esp_camera_sensor_get();
  s->set_contrast(s, 2);

  // Set rotation (added lines)
  s->set_vflip(s, true);  // Vertical flip
  s->set_hmirror(s, true);  // Horizontal mirror
}

void processImage(camera_fb_t* frameBuffer) {
  if (!disableDithering) {
    ditherImage(frameBuffer, dtType);
  }

  uint8_t flipper_y = 0;
  for (uint8_t y = 28; y < 92; ++y) {
    // Print the Y coordinate.
    Serial.print("Y:");
    Serial.print((char)flipper_y);

    // Print the character.
    // The y value to use in the frame buffer array.
    size_t true_y = y * frameBuffer->width;

    // For each column of 8 pixels in the current row.
    for (uint8_t x = 16; x < 144; x += 8) {
      // The current character being constructed.
      char c = 0;

      // For each pixel in the current column of 8.
      for (uint8_t j = 0; j < 8; ++j) {
        if (isDarkBit(frameBuffer->buf[true_y + x + (7 - j)])) {
          // Shift the bit into the right position
          c |= (uint8_t)1 << (uint8_t)j;
        }
      }

      // Output the character.
      Serial.print(c);
    }

    // Move to the next line.
    ++flipper_y;
    Serial.flush();
  }
}

// Dither image.
// @param fb Frame buffer
// @param dt Dithering type:
//    0 = Floyd Steinberg (default)
//    1 = Atkinson
void ditherImage(camera_fb_t* fb, int dt) {
  switch (dt) {
    default:
    case 0: // Floyd Steinberg dithering
      for (int y = 0; y < fb->height - 1; ++y) {
        for (int x = 1; x < fb->width - 1; ++x) {
          int current = y * fb->width + x;
          // Convert to black or white
          uint8_t oldpixel = fb->buf[current];
          uint8_t newpixel = oldpixel >= 128 ? 255 : 0;
          fb->buf[current] = newpixel;
          // Compute quantization error
          int quant_error = oldpixel - newpixel;
          // Propagate the error
          fb->buf[current + 1] += quant_error * 7 / 16;
          fb->buf[(y + 1) * fb->width + x - 1] += quant_error * 3 / 16;
          fb->buf[(y + 1) * fb->width + x] += quant_error * 5 / 16;
          fb->buf[(y + 1) * fb->width + x + 1] += quant_error / 16;
        }
      }
      break;
    case 1: // Atkinson dithering
      for (int y = 0; y < fb->height; ++y) {
        for (int x = 0; x < fb->width; ++x) {
          int current = y * fb->width + x;
          uint8_t oldpixel = fb->buf[current];
          uint8_t newpixel = oldpixel >= 128 ? 255 : 0;
          fb->buf[current] = newpixel;
          int quant_error = oldpixel - newpixel;

          if (x + 1 < fb->width)
            fb->buf[current + 1] += quant_error * 1 / 8;
          if (x + 2 < fb->width)
            fb->buf[current + 2] += quant_error * 1 / 8;
          if (x > 0 && y + 1 < fb->height)
            fb->buf[(y + 1) * fb->width + x - 1] += quant_error * 1 / 8;
          if (y + 1 < fb->height)
            fb->buf[(y + 1) * fb->width + x] += quant_error * 1 / 8;
          if (y + 1 < fb->height && x + 1 < fb->width)
            fb->buf[(y + 1) * fb->width + x + 1] += quant_error * 1 / 8;
          if (y + 2 < fb->height)
            fb->buf[(y + 2) * fb->width + x] += quant_error * 1 / 8;
        }
      }
      break;
  }
}

// Returns true if the bit is "dark".
bool isDarkBit(uint8_t bit) {
  if (invert) {
    return bit >= 128;
  } else {
    return bit < 128;
  }
}