#include <stdio.h>
#include <furi.h>
#include <../../applications/main/gpio/gpio_item.h>
#include <gui/gui.h>
#include <input/input.h>
#include <notification/notification_messages.h>

//#include <notification/notification_messages_notes.h>

int Time = 10;
int Count = 10;
int WorkTime = 0;
int WorkCount = 0;
bool InfiniteShot = false;
bool Bulb = false;
bool Backlight = true;

const NotificationSequence sequence_click = {
    &message_note_c7,
    &message_delay_50,
    &message_sound_off,
    NULL,
};

typedef enum {
    EventTypeTick,
    EventTypeInput,
} EventType;

typedef struct {
    EventType type;
    InputEvent input;
} ZeitrafferEvent;

static void draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
	char temp_str[36];
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    if (Count == -1)
		snprintf(temp_str,sizeof(temp_str),"Set: BULB %i sec",Time);		
	else if (Count == 0)
		snprintf(temp_str,sizeof(temp_str),"Set: infinite, %i sec",Time);
	else
		snprintf(temp_str,sizeof(temp_str),"Set: %i frames, %i sec",Count,Time);
	canvas_draw_str(canvas, 3, 15, temp_str);
	snprintf(temp_str,sizeof(temp_str),"Left: %i frames, %i sec",WorkCount,WorkTime);
	canvas_draw_str(canvas, 3, 35, temp_str);
	snprintf(temp_str,sizeof(temp_str),"Backlight: %i",Backlight);
	canvas_draw_str(canvas, 3, 55, temp_str);	
}

static void input_callback(InputEvent* input_event, void* ctx) {
    // Проверяем, что контекст не нулевой
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;

    ZeitrafferEvent event = {.type = EventTypeInput, .input = *input_event};
    furi_message_queue_put(event_queue, &event, FuriWaitForever);
}

static void timer_callback(FuriMessageQueue* event_queue) {
    // Проверяем, что контекст не нулевой
    furi_assert(event_queue);

    ZeitrafferEvent event = {.type = EventTypeTick};
    furi_message_queue_put(event_queue, &event, 0);
}

int32_t zeitraffer_app(void* p) {
    UNUSED(p);
	
    // Текущее событие типа кастомного типа ZeitrafferEvent
    ZeitrafferEvent event;
    // Очередь событий на 8 элементов размера ZeitrafferEvent
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(ZeitrafferEvent));

    // Создаем новый view port
    ViewPort* view_port = view_port_alloc();
    // Создаем callback отрисовки, без контекста
    view_port_draw_callback_set(view_port, draw_callback, NULL);
    // Создаем callback нажатий на клавиши, в качестве контекста передаем
    // нашу очередь сообщений, чтоб запихивать в неё эти события
    view_port_input_callback_set(view_port, input_callback, event_queue);

    // Создаем GUI приложения
    Gui* gui = furi_record_open(RECORD_GUI);
    // Подключаем view port к GUI в полноэкранном режиме
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

	gpio_item_configure_all_pins(GpioModeOutputPushPull);

    // Создаем периодический таймер с коллбэком, куда в качестве
    // контекста будет передаваться наша очередь событий
    FuriTimer* timer = furi_timer_alloc(timer_callback, FuriTimerTypePeriodic, event_queue);
    // Запускаем таймер
    //furi_timer_start(timer, 1500);

    // Включаем нотификации
    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);

    // Бесконечный цикл обработки очереди событий
    while(1) {
        // Выбираем событие из очереди в переменную event (ждем бесконечно долго, если очередь пуста)
        // и проверяем, что у нас получилось это сделать
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        // Наше событие — это нажатие кнопки
       if(event.type == EventTypeInput) {
		if(event.input.type == InputTypeShort) {
            // Если нажата кнопка "назад", то выходим из цикла, а следовательно и из приложения
            if(event.input.key == InputKeyBack) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else {
				WorkCount = Count;
				WorkTime = 3;
				if (Count == 0) {InfiniteShot = true; WorkCount = 1;} else InfiniteShot = false;
				notification_message(notifications, &sequence_success); 
			}
               // break;
            }
			if(event.input.key == InputKeyRight) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 			
			Count++;
			notification_message(notifications, &sequence_click);
			//view_port_update(view_port);
			}
            }
			if(event.input.key == InputKeyLeft) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 
			Count--;
			notification_message(notifications, &sequence_click);
			//view_port_update(view_port);
			}
            }
			if(event.input.key == InputKeyUp) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 
			Time++;
			notification_message(notifications, &sequence_click);
			//view_port_update(view_port);
            }
			}
			if(event.input.key == InputKeyDown) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 
			Time--;
			notification_message(notifications, &sequence_click);
			//view_port_update(view_port);
            }
			}
			if(event.input.key == InputKeyOk) {
			
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_click);
				furi_timer_stop(timer); 
			}
			else { 
				furi_timer_start(timer, 1000);
				if (WorkCount == 0) WorkCount = Count;
				if (WorkTime == 0) WorkTime = 3;
				if (Count == 0) {InfiniteShot = true; WorkCount = 1;} else InfiniteShot = false;
				if (Count == -1) {gpio_item_set_pin(4, true); gpio_item_set_pin(5, true); Bulb = true; WorkCount = 1; WorkTime = Time;} else Bulb = false;
				notification_message(notifications, &sequence_success);
				notification_message(notifications, &sequence_display_backlight_off_delay_1000);
				}
            }
			}
		if(event.input.type == InputTypeLong) {
            // Если нажата кнопка "назад", то выходим из цикла, а следовательно и из приложения
            if(event.input.key == InputKeyBack) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else {
			notification_message(notifications, &sequence_click);
			gpio_item_set_all_pins(false);
			furi_timer_stop(timer);
                break;
            }
			}
			if(event.input.key == InputKeyOk) {
				//notification_message(notifications, &sequence_display_backlight_on);
				Backlight = !Backlight;
            }
			}
		if(event.input.type == InputTypeRepeat) {
			if(event.input.key == InputKeyRight) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 
			Count = Count+10;
			//view_port_update(view_port);
            }
			}
			if(event.input.key == InputKeyLeft) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 
			Count = Count-10;
			//view_port_update(view_port);
            }
			}
			if(event.input.key == InputKeyUp) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 
			Time = Time+10;
			//view_port_update(view_port);
            }
			}
			if(event.input.key == InputKeyDown) {
			if(furi_timer_is_running(timer)) {
				notification_message(notifications, &sequence_error);
			}
			else { 
			Time = Time-10;
			//view_port_update(view_port);
            }
			}
			}
            // Наше событие — это сработавший таймер
        } else if(event.type == EventTypeTick) {
            // Отправляем нотификацию мигания синим светодиодом
			WorkTime--;
			notification_message(notifications, &sequence_blink_blue_100);
			if (Backlight) {
				notification_message(notifications, &sequence_display_backlight_on);
			}
			else {
				notification_message(notifications, &sequence_display_backlight_off);
			}
            if( WorkTime < 1 ) {
				
				if (Bulb) {
					gpio_item_set_all_pins(false); WorkCount = 0;
					}
				else {
				WorkCount--;
				view_port_update(view_port);
				notification_message(notifications, &sequence_click);
				//gpio_item_set_all_pins(true);
				gpio_item_set_pin(4, true);
				gpio_item_set_pin(5, true);
				furi_delay_ms(400);
				gpio_item_set_pin(4, false);
				gpio_item_set_pin(5, false);
				//gpio_item_set_all_pins(false);

				if (InfiniteShot) WorkCount++;
				
				WorkTime = Time;
				view_port_update(view_port);
				}
				}
			if( WorkCount < 1 ) { 
				gpio_item_set_all_pins(false);
				furi_timer_stop(timer); 
				notification_message(notifications, &sequence_audiovisual_alert);
				WorkTime = 3;
				WorkCount = 0;
				}
        }
    if (Time < 1) Time = 1;
	if (Count < -1) Count = 0;
	}

    // Очищаем таймер
    furi_timer_free(timer);

    // Специальная очистка памяти, занимаемой очередью
    furi_message_queue_free(event_queue);

    // Чистим созданные объекты, связанные с интерфейсом
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    // Очищаем нотификации
    furi_record_close(RECORD_NOTIFICATION);

    return 0;
}
