#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdint.h>

// Define Pin numbers used by the camera.
#define FLASH_GPIO_NUM 4
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define SIOC_GPIO_NUM 27
#define SIOD_GPIO_NUM 26
#define VSYNC_GPIO_NUM 25
#define XCLK_GPIO_NUM 0
#define Y2_GPIO_NUM 5
#define Y3_GPIO_NUM 18
#define Y4_GPIO_NUM 19
#define Y5_GPIO_NUM 21
#define Y6_GPIO_NUM 36
#define Y7_GPIO_NUM 39
#define Y8_GPIO_NUM 34
#define Y9_GPIO_NUM 35

/**
 * The dithering algorithms available.
 */
enum DitheringAlgorithm : uint8_t
{
    FLOYD_STEINBERG,
    JARVIS_JUDICE_NINKE,
    STUCKI
};

typedef struct CameraModel
{
    /**
     * Flag to enable or disable dithering.
     */
    bool isDitheringDisabled;
    /**
     * Flag to represent the flash state when saving pictures to the Flipper.
     */
    bool isFlashEnabled;
    /**
     * Flag to invert pixel colors.
     */
    bool isInverted;
    /**
     * Flag to stop or start the stream.
     */
    bool isStreamEnabled;
    /**
     * Holds the currently selected dithering algorithm.
     */
    DitheringAlgorithm ditherAlgorithm;
} CameraModel;

#endif
