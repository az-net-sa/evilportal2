#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>

#define MAX_I2C_ADDR 0x7F

#define APP_NAME "I2C Tools"

#define SCAN_MENU_TEXT "Scan"
#define SCAN_MENU_X 75
#define SCAN_MENU_Y 6

#define SNIFF_MENU_TEXT "Sniff"
#define SNIFF_MENU_X 75
#define SNIFF_MENU_Y 20

#define SEND_MENU_TEXT "Send"
#define SEND_MENU_X 75
#define SEND_MENU_Y 34

#define PLAY_MENU_TEXT "Play"
#define PLAY_MENU_X 75
#define PLAY_MENU_Y 48

// Sniffer Pins
#define pinSCL &gpio_ext_pc0
#define pinSDA &gpio_ext_pc1

// I2C BUS
#define I2C_BUS &furi_hal_i2c_handle_external

typedef enum {
    MAIN_VIEW,
    SCAN_VIEW,
    SNIFF_VIEW,
    SEND_VIEW,
    PLAY_VIEW,

    /* Know menu Size*/
    MENU_SIZE
} i2cToolsMainMenu;

// Bus Sniffer
typedef enum { I2C_BUS_IDLE, I2C_BUS_STARTED } i2cBusStates;

#define MAX_FRAMES 32

typedef struct {
    uint8_t i2ctools[MAX_FRAMES];
    bool ack[MAX_FRAMES];
    uint8_t bit_index;
    uint8_t data_index;
} i2cFrame;

typedef struct {
    bool started;
    bool first;
    i2cBusStates state;
    i2cFrame frames[MAX_FRAMES];
    uint8_t frame_index;
    uint8_t menu_index;
} _sniffer;

// Bus scanner
typedef struct {
    uint8_t addresses[MAX_I2C_ADDR + 1];
    uint8_t found;
    uint8_t menu_index;
    bool scanned;
} _scanner;

// Sender
typedef struct {
    uint8_t address_idx;
    uint8_t value;
    uint8_t recv[2];
    bool must_send;
    bool sended;
    bool error;
} _sender;

typedef struct {
    ViewPort* view_port;
    i2cToolsMainMenu current_menu;
    uint8_t main_menu_index;

    _scanner scanner;
    _sniffer sniffer;
    _sender sender;
} i2cTools;

void scan_i2c_bus(i2cTools* i2ctools) {
    i2ctools->scanner.found = 0;
    i2ctools->scanner.scanned = true;
    furi_hal_i2c_acquire(I2C_BUS);
    // scan
    for(uint8_t addr = 0x01; addr < MAX_I2C_ADDR; addr++) {
        // Check for peripherals
        if(furi_hal_i2c_is_device_ready(I2C_BUS, addr, 2)) {
            // skip even 8-bit addr
            if(addr % 2 != 0) {
                continue;
            }
            // convert addr to 7-bits
            i2ctools->scanner.addresses[i2ctools->scanner.found] = addr >> 1;
            i2ctools->scanner.found++;
        }
    }
    furi_hal_i2c_release(I2C_BUS);
}

void i2ctools_draw_main_menu(Canvas* canvas, i2cTools* i2ctools) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_draw_icon(canvas, 2, 13, &I_passport_bad3_46x49);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 3, 3, AlignLeft, AlignTop, APP_NAME);

    switch(i2ctools->main_menu_index) {
    case 0:
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_str_aligned(
            canvas, SNIFF_MENU_X, SNIFF_MENU_Y, AlignLeft, AlignTop, SNIFF_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, SEND_MENU_X, SEND_MENU_Y, AlignLeft, AlignTop, SEND_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, PLAY_MENU_X, PLAY_MENU_Y, AlignLeft, AlignTop, PLAY_MENU_TEXT);

        canvas_draw_rbox(canvas, 60, SCAN_MENU_Y - 2, 60, 13, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(
            canvas, SCAN_MENU_X, SCAN_MENU_Y, AlignLeft, AlignTop, SCAN_MENU_TEXT);
        break;

    case 1:
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_str_aligned(
            canvas, SCAN_MENU_X, SCAN_MENU_Y, AlignLeft, AlignTop, SCAN_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, SEND_MENU_X, SEND_MENU_Y, AlignLeft, AlignTop, SEND_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, PLAY_MENU_X, PLAY_MENU_Y, AlignLeft, AlignTop, PLAY_MENU_TEXT);

        canvas_draw_rbox(canvas, 60, SNIFF_MENU_Y - 2, 60, 13, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(
            canvas, SNIFF_MENU_X, SNIFF_MENU_Y, AlignLeft, AlignTop, SNIFF_MENU_TEXT);
        break;

    case 2:
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_str_aligned(
            canvas, SCAN_MENU_X, SCAN_MENU_Y, AlignLeft, AlignTop, SCAN_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, SNIFF_MENU_X, SNIFF_MENU_Y, AlignLeft, AlignTop, SNIFF_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, PLAY_MENU_X, PLAY_MENU_Y, AlignLeft, AlignTop, PLAY_MENU_TEXT);

        canvas_draw_rbox(canvas, 60, SEND_MENU_Y - 2, 60, 13, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(
            canvas, SEND_MENU_X, SEND_MENU_Y, AlignLeft, AlignTop, SEND_MENU_TEXT);
        break;

    case 3:
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_str_aligned(
            canvas, SCAN_MENU_X, SCAN_MENU_Y, AlignLeft, AlignTop, SCAN_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, SNIFF_MENU_X, SNIFF_MENU_Y, AlignLeft, AlignTop, SNIFF_MENU_TEXT);
        canvas_draw_str_aligned(
            canvas, SEND_MENU_X, SEND_MENU_Y, AlignLeft, AlignTop, SEND_MENU_TEXT);

        canvas_draw_rbox(canvas, 60, PLAY_MENU_Y - 2, 60, 13, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(
            canvas, PLAY_MENU_X, PLAY_MENU_Y, AlignLeft, AlignTop, PLAY_MENU_TEXT);
        break;

    default:
        break;
    }
}

void clearSnifferBuffers(void* ctx) {
    i2cTools* i2ctools = ctx;
    for(uint8_t i = 0; i < MAX_FRAMES; i++) {
        for(uint8_t j = 0; j < MAX_FRAMES; j++) {
            i2ctools->sniffer.frames[i].ack[j] = false;
            i2ctools->sniffer.frames[i].i2ctools[j] = 0;
        }
        i2ctools->sniffer.frames[i].bit_index = 0;
        i2ctools->sniffer.frames[i].data_index = 0;
    }
    i2ctools->sniffer.frame_index = 0;
    i2ctools->sniffer.state = I2C_BUS_IDLE;
    i2ctools->sniffer.first = true;
}

// Called on Fallin/Rising SDA
// Used to monitor i2c bus state
static void SDAcallback(void* ctx) {
    i2cTools* i2ctools = ctx;
    // SCL is low maybe cclock strecching
    if(furi_hal_gpio_read(pinSCL) == false) {
        return;
    }
    // Check for stop condition: SDA rising while SCL is High
    if(i2ctools->sniffer.state == I2C_BUS_STARTED) {
        if(furi_hal_gpio_read(pinSDA) == true) {
            i2ctools->sniffer.state = I2C_BUS_IDLE;
            view_port_update(i2ctools->view_port);
        }
    }
    // Check for start condition: SDA falling while SCL is high
    else if(furi_hal_gpio_read(pinSDA) == false) {
        i2ctools->sniffer.state = I2C_BUS_STARTED;
        if(i2ctools->sniffer.first) {
            i2ctools->sniffer.first = false;
            return;
        }
        i2ctools->sniffer.frame_index++;
        if(i2ctools->sniffer.frame_index >= MAX_FRAMES) {
            clearSnifferBuffers(ctx);
        }
    }
    return;
}

// Called on Rising SCL
// Used to read bus datas
static void SCLcallback(void* ctx) {
    i2cTools* i2ctools = ctx;
    if(i2ctools->sniffer.state == I2C_BUS_IDLE) {
        return;
    }
    uint8_t frame = i2ctools->sniffer.frame_index;
    uint8_t bit = i2ctools->sniffer.frames[frame].bit_index;
    uint8_t data_idx = i2ctools->sniffer.frames[frame].data_index;
    if(bit < 8) {
        i2ctools->sniffer.frames[frame].i2ctools[data_idx] <<= 1;
        i2ctools->sniffer.frames[frame].i2ctools[data_idx] |= (int)furi_hal_gpio_read(pinSDA);
        i2ctools->sniffer.frames[frame].bit_index++;
    } else {
        i2ctools->sniffer.frames[frame].ack[data_idx] = !furi_hal_gpio_read(pinSDA);
        i2ctools->sniffer.frames[frame].data_index++;
        i2ctools->sniffer.frames[frame].bit_index = 0;
    }
}

void start_interrupts(i2cTools* i2ctools) {
    furi_hal_gpio_init(pinSCL, GpioModeInterruptRise, GpioPullNo, GpioSpeedHigh);
    furi_hal_gpio_add_int_callback(pinSCL, SCLcallback, i2ctools);

    // Add Rise and Fall Interrupt on SDA pin
    furi_hal_gpio_init(pinSDA, GpioModeInterruptRiseFall, GpioPullNo, GpioSpeedHigh);
    furi_hal_gpio_add_int_callback(pinSDA, SDAcallback, i2ctools);
}

void stop_interrupts() {
    furi_hal_gpio_remove_int_callback(pinSCL);
    furi_hal_gpio_remove_int_callback(pinSDA);
}

void i2ctools_draw_sniff_view(Canvas* canvas, i2cTools* i2ctools) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_draw_icon(canvas, 2, 13, &I_passport_happy2_46x49);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 3, 3, AlignLeft, AlignTop, SNIFF_MENU_TEXT);
    canvas_set_font(canvas, FontSecondary);

    // Button
    canvas_draw_rbox(canvas, 70, 48, 45, 13, 3);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_icon(canvas, 75, 50, &I_Ok_btn_9x9);
    if(!i2ctools->sniffer.started) {
        canvas_draw_str_aligned(canvas, 85, 51, AlignLeft, AlignTop, "Start");
    } else {
        canvas_draw_str_aligned(canvas, 85, 51, AlignLeft, AlignTop, "Stop");
    }
    canvas_set_color(canvas, ColorBlack);
    // Address text
    char addr_text[8];
    snprintf(
        addr_text,
        sizeof(addr_text),
        "0x%02x",
        (int)(i2ctools->sniffer.frames[i2ctools->sniffer.menu_index].i2ctools[0] >> 1));
    canvas_draw_str_aligned(canvas, 50, 3, AlignLeft, AlignTop, "Addr: ");
    canvas_draw_str_aligned(canvas, 75, 3, AlignLeft, AlignTop, addr_text);
    // R/W
    if((int)(i2ctools->sniffer.frames[i2ctools->sniffer.menu_index].i2ctools[0]) % 2 == 0) {
        canvas_draw_str_aligned(canvas, 105, 3, AlignLeft, AlignTop, "W");
    } else {
        canvas_draw_str_aligned(canvas, 105, 3, AlignLeft, AlignTop, "R");
    }
    // nbFrame text
    canvas_draw_str_aligned(canvas, 50, 13, AlignLeft, AlignTop, "Frames: ");
    snprintf(addr_text, sizeof(addr_text), "%d", (int)i2ctools->sniffer.menu_index + 1);
    canvas_draw_str_aligned(canvas, 90, 13, AlignLeft, AlignTop, addr_text);
    canvas_draw_str_aligned(canvas, 100, 13, AlignLeft, AlignTop, "/");
    snprintf(addr_text, sizeof(addr_text), "%d", (int)i2ctools->sniffer.frame_index + 1);
    canvas_draw_str_aligned(canvas, 110, 13, AlignLeft, AlignTop, addr_text);
    // Frames content
    uint8_t x_pos = 0;
    uint8_t y_pos = 23;
    for(uint8_t i = 1; i < i2ctools->sniffer.frames[i2ctools->sniffer.menu_index].data_index;
        i++) {
        snprintf(
            addr_text,
            sizeof(addr_text),
            "0x%02x",
            (int)i2ctools->sniffer.frames[i2ctools->sniffer.menu_index].i2ctools[i]);
        x_pos = 50 + (i - 1) * 35;
        canvas_draw_str_aligned(canvas, x_pos, y_pos, AlignLeft, AlignTop, addr_text);
        if(i2ctools->sniffer.frames[i2ctools->sniffer.menu_index].ack[i]) {
            canvas_draw_str_aligned(canvas, x_pos + 24, y_pos, AlignLeft, AlignTop, "A");
        } else {
            canvas_draw_str_aligned(canvas, x_pos + 24, y_pos, AlignLeft, AlignTop, "N");
        }
    }
}

void i2ctools_draw_record_view(Canvas* canvas, i2cTools* i2ctools) {
    UNUSED(i2ctools);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_draw_icon(canvas, 2, 13, &I_passport_happy2_46x49);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 3, 3, AlignLeft, AlignTop, PLAY_MENU_TEXT);
}

void i2ctools_draw_send_view(Canvas* canvas, i2cTools* i2ctools) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_draw_icon(canvas, 2, 13, &I_passport_happy2_46x49);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 3, 3, AlignLeft, AlignTop, SEND_MENU_TEXT);

    if(!i2ctools->scanner.scanned) {
        scan_i2c_bus(i2ctools);
    }

    canvas_set_font(canvas, FontSecondary);
    if(i2ctools->scanner.found <= 0) {
        canvas_draw_str_aligned(canvas, 60, 5, AlignLeft, AlignTop, "No peripherals");
        canvas_draw_str_aligned(canvas, 60, 15, AlignLeft, AlignTop, "Found");
        return;
    }
    canvas_draw_rbox(canvas, 70, 48, 45, 13, 3);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_icon(canvas, 75, 50, &I_Ok_btn_9x9);
    canvas_draw_str_aligned(canvas, 85, 51, AlignLeft, AlignTop, "Send");
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_str_aligned(canvas, 50, 5, AlignLeft, AlignTop, "Addr: ");
    canvas_draw_icon(canvas, 80, 5, &I_ButtonLeft_4x7);
    canvas_draw_icon(canvas, 115, 5, &I_ButtonRight_4x7);
    char addr_text[8];
    snprintf(
        addr_text,
        sizeof(addr_text),
        "0x%02x",
        (int)i2ctools->scanner.addresses[i2ctools->sender.address_idx]);
    canvas_draw_str_aligned(canvas, 90, 5, AlignLeft, AlignTop, addr_text);
    canvas_draw_str_aligned(canvas, 50, 15, AlignLeft, AlignTop, "Value: ");

    canvas_draw_icon(canvas, 80, 17, &I_ButtonUp_7x4);
    canvas_draw_icon(canvas, 115, 17, &I_ButtonDown_7x4);
    snprintf(addr_text, sizeof(addr_text), "0x%02x", (int)i2ctools->sender.value);
    canvas_draw_str_aligned(canvas, 90, 15, AlignLeft, AlignTop, addr_text);
    if(i2ctools->sender.must_send) {
        furi_hal_i2c_acquire(&furi_hal_i2c_handle_external);
        i2ctools->sender.error = furi_hal_i2c_trx(
            &furi_hal_i2c_handle_external,
            i2ctools->scanner.addresses[i2ctools->sender.address_idx] << 1,
            &i2ctools->sender.value,
            1,
            i2ctools->sender.recv,
            sizeof(i2ctools->sender.recv),
            3);
        furi_hal_i2c_release(&furi_hal_i2c_handle_external);
        i2ctools->sender.must_send = false;
        i2ctools->sender.sended = true;
    }
    canvas_draw_str_aligned(canvas, 50, 25, AlignLeft, AlignTop, "Result: ");
    if(i2ctools->sender.sended) {
        for(uint8_t i = 0; i < sizeof(i2ctools->sender.recv); i++) {
            snprintf(addr_text, sizeof(addr_text), "0x%02x", (int)i2ctools->sender.recv[i]);
            canvas_draw_str_aligned(canvas, 90, 25 + (i * 10), AlignLeft, AlignTop, addr_text);
        }
    }
}

void i2ctools_draw_scan_view(Canvas* canvas, i2cTools* i2ctools) {
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, 0, 0, 128, 64, 3);
    canvas_draw_icon(canvas, 2, 13, &I_passport_happy3_46x49);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 3, 3, AlignLeft, AlignTop, SCAN_MENU_TEXT);

    char count_text[46];
    char count_text_fmt[] = "Found: %d";
    canvas_set_font(canvas, FontSecondary);
    snprintf(count_text, sizeof(count_text), count_text_fmt, (int)i2ctools->scanner.found);
    canvas_draw_str_aligned(canvas, 50, 3, AlignLeft, AlignTop, count_text);
    uint8_t x_pos = 0;
    uint8_t y_pos = 0;
    uint8_t idx_to_print = 0;
    for(uint8_t i = 0; i < (int)i2ctools->scanner.found; i++) {
        idx_to_print = i + i2ctools->scanner.menu_index * 3;
        if(idx_to_print >= MAX_I2C_ADDR) {
            break;
        }
        snprintf(
            count_text,
            sizeof(count_text),
            "0x%02x ",
            (int)i2ctools->scanner.addresses[idx_to_print]);
        if(i < 3) {
            x_pos = 50 + (i * 26);
            y_pos = 15;
        } else if(i < 6) {
            x_pos = 50 + ((i - 3) * 26);
            y_pos = 25;
        } else if(i < 9) {
            x_pos = 50 + ((i - 6) * 26);
            y_pos = 35;
        } else if(i < 12) {
            x_pos = 50 + ((i - 9) * 26);
            y_pos = 45;
        } else if(i < 15) {
            x_pos = 50 + ((i - 12) * 26);
            y_pos = 55;
        } else {
            break;
        }
        canvas_draw_str_aligned(canvas, x_pos, y_pos, AlignLeft, AlignTop, count_text);
    }
    // Right cursor
    y_pos = 14 + i2ctools->scanner.menu_index;
    canvas_draw_rbox(canvas, 125, y_pos, 3, 10, 1);

    // Button
    canvas_draw_rbox(canvas, 70, 48, 45, 13, 3);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_icon(canvas, 75, 50, &I_Ok_btn_9x9);
    canvas_draw_str_aligned(canvas, 85, 51, AlignLeft, AlignTop, "Scan");
}

void i2ctools_draw_callback(Canvas* canvas, void* ctx) {
    i2cTools* i2c_addr = acquire_mutex((ValueMutex*)ctx, 25);

    switch(i2c_addr->current_menu) {
    case MAIN_VIEW:
        i2ctools_draw_main_menu(canvas, i2c_addr);
        break;

    case SCAN_VIEW:
        i2ctools_draw_scan_view(canvas, i2c_addr);
        break;

    case SNIFF_VIEW:
        i2ctools_draw_sniff_view(canvas, i2c_addr);
        break;
    case SEND_VIEW:
        i2ctools_draw_send_view(canvas, i2c_addr);
        break;
    case PLAY_VIEW:
        i2ctools_draw_record_view(canvas, i2c_addr);
        break;
    default:
        break;
    }
    release_mutex((ValueMutex*)ctx, i2c_addr);
}

void i2ctools_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

int32_t i2ctools_app(void* p) {
    UNUSED(p);
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    i2cTools* i2ctools = malloc(sizeof(i2cTools));
    ValueMutex i2ctools_mutex;
    if(!init_mutex(&i2ctools_mutex, i2ctools, sizeof(i2cTools))) {
        FURI_LOG_E(APP_NAME, "cannot create mutex\r\n");
        free(i2ctools);
        return -1;
    }
    printf(APP_NAME);
    printf("\r\n");

    i2ctools->view_port = view_port_alloc();
    view_port_draw_callback_set(i2ctools->view_port, i2ctools_draw_callback, &i2ctools_mutex);
    view_port_input_callback_set(i2ctools->view_port, i2ctools_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, i2ctools->view_port, GuiLayerFullscreen);

    InputEvent event;

    clearSnifferBuffers(i2ctools);
    i2ctools->sniffer.started = false;
    i2ctools->sniffer.menu_index = 0;

    i2ctools->scanner.menu_index = 0;
    i2ctools->scanner.scanned = false;

    i2ctools->sender.must_send = false;
    i2ctools->sender.sended = false;
    while(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk) {
        if(event.key == InputKeyBack && event.type == InputTypeRelease) {
            if(i2ctools->current_menu == MAIN_VIEW) {
                break;
            } else {
                if(i2ctools->current_menu == SNIFF_VIEW) {
                    stop_interrupts();
                    i2ctools->sniffer.started = false;
                    i2ctools->sniffer.state = I2C_BUS_IDLE;
                }
                i2ctools->current_menu = MAIN_VIEW;
            }
        } else if(event.key == InputKeyUp && event.type == InputTypeRelease) {
            if(i2ctools->current_menu == MAIN_VIEW) {
                if(i2ctools->main_menu_index > 0) {
                    i2ctools->main_menu_index--;
                }
            } else if(i2ctools->current_menu == SCAN_VIEW) {
                if(i2ctools->scanner.menu_index > 0) {
                    i2ctools->scanner.menu_index--;
                }
            } else if(i2ctools->current_menu == SEND_VIEW) {
                if(i2ctools->sender.value < 0xFF) {
                    i2ctools->sender.value++;
                    i2ctools->sender.sended = false;
                }
            }
        } else if(
            event.key == InputKeyUp &&
            (event.type == InputTypeLong || event.type == InputTypeRepeat)) {
            if(i2ctools->current_menu == SEND_VIEW) {
                if(i2ctools->sender.value < 0xF9) {
                    i2ctools->sender.value += 5;
                    i2ctools->sender.sended = false;
                }
            }
        } else if(event.key == InputKeyDown && event.type == InputTypeRelease) {
            if(i2ctools->current_menu == MAIN_VIEW) {
                if(i2ctools->main_menu_index < 3) {
                    i2ctools->main_menu_index++;
                }
            } else if(i2ctools->current_menu == SCAN_VIEW) {
                if(i2ctools->scanner.menu_index < ((int)i2ctools->scanner.found / 3)) {
                    i2ctools->scanner.menu_index++;
                }
            } else if(i2ctools->current_menu == SEND_VIEW) {
                if(i2ctools->sender.value > 0x00) {
                    i2ctools->sender.value--;
                    i2ctools->sender.sended = false;
                }
            }
        } else if(event.key == InputKeyDown && event.type == InputTypeLong) {
            if(i2ctools->current_menu == SEND_VIEW) {
                if(i2ctools->sender.value > 0x05) {
                    i2ctools->sender.value -= 5;
                    i2ctools->sender.sended = false;
                }
            }

        } else if(event.key == InputKeyOk && event.type == InputTypeRelease) {
            if(i2ctools->current_menu == MAIN_VIEW) {
                if(i2ctools->main_menu_index == 0) {
                    scan_i2c_bus(i2ctools);
                    i2ctools->current_menu = SCAN_VIEW;
                } else if(i2ctools->main_menu_index == 1) {
                    i2ctools->current_menu = SNIFF_VIEW;
                } else if(i2ctools->main_menu_index == 2) {
                    i2ctools->current_menu = SEND_VIEW;
                } else if(i2ctools->main_menu_index == 3) {
                    i2ctools->current_menu = PLAY_VIEW;
                }
            } else if(i2ctools->current_menu == SCAN_VIEW) {
                scan_i2c_bus(i2ctools);
            } else if(i2ctools->current_menu == SEND_VIEW) {
                i2ctools->sender.must_send = true;
            } else if(i2ctools->current_menu == SNIFF_VIEW) {
                if(i2ctools->sniffer.started) {
                    stop_interrupts();
                    i2ctools->sniffer.started = false;
                    i2ctools->sniffer.state = I2C_BUS_IDLE;
                } else {
                    start_interrupts(i2ctools);
                    i2ctools->sniffer.started = true;
                    i2ctools->sniffer.state = I2C_BUS_IDLE;
                }
            }
        } else if(event.key == InputKeyRight && event.type == InputTypeRelease) {
            if(i2ctools->current_menu == SEND_VIEW) {
                if(i2ctools->sender.address_idx < (i2ctools->scanner.found - 1)) {
                    i2ctools->sender.address_idx++;
                    i2ctools->sender.sended = false;
                }
            } else if(i2ctools->current_menu == SNIFF_VIEW) {
                if(i2ctools->sniffer.menu_index < i2ctools->sniffer.frame_index) {
                    i2ctools->sniffer.menu_index++;
                }
            }
        } else if(event.key == InputKeyLeft && event.type == InputTypeRelease) {
            if(i2ctools->current_menu == SEND_VIEW) {
                if(i2ctools->sender.address_idx > 0) {
                    i2ctools->sender.address_idx--;
                    i2ctools->sender.sended = false;
                }
            } else if(i2ctools->current_menu == SNIFF_VIEW) {
                if(i2ctools->sniffer.menu_index > 0) {
                    i2ctools->sniffer.menu_index--;
                }
            }
        }
        view_port_update(i2ctools->view_port);
    }
    gui_remove_view_port(gui, i2ctools->view_port);
    view_port_free(i2ctools->view_port);
    furi_message_queue_free(event_queue);
    free(i2ctools);
    furi_record_close(RECORD_GUI);
    return 0;
}
