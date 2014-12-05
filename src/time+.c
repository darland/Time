#include <pebble.h>
#include <pebble_app_info.h>
#include <pebble_fonts.h>


#define DEVICE_WIDTH        144
#define DEVICE_HEIGHT       168
#define BLUETOOTH_BUZZER 1


int STYLE_KEY = 1;


GColor background_color = GColorWhite;
GColor foreground_color = GColorBlack;
GCompOp compositing_mode = GCompOpAssign;


static Window *window;

//Layers
static TextLayer *text_date_layer;
static TextLayer *text_time_layer;
static TextLayer *text_day_layer;
static Layer *battery_layer;

static Layer *statusbar;
Layer *window_layer;

static BitmapLayer *bluetoothLayer;
static GBitmap *bluetooth;

static GRect bluetooth_rect;
bool bluetooth_state = false;
static char days[] = "MOTUWETHFRSASU";

//Struct for application state management
typedef struct State {
	int battery_count;
	int wday;
	int month;
	int day;
	bool init;
} State;
static State state = {0, 0, 0, 0, false};


static void handle_bluetooth(bool connected) {
	layer_remove_from_parent(bitmap_layer_get_layer(bluetoothLayer));
    bitmap_layer_destroy(bluetoothLayer);
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
    
    
    if (connected != bluetooth_state) {
        bluetooth_state = connected;
#ifdef BLUETOOTH_BUZZER
        if (!bluetooth_state) {
            vibes_long_pulse();
        }
#endif
    }
    
    
	if (connected) {
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
	//if (state.init) vibes_short_pulse();
	
	}
	layer_add_child(statusbar, bitmap_layer_get_layer(bluetoothLayer));
    bitmap_layer_set_compositing_mode(bluetoothLayer, compositing_mode);
}

static void handle_appfocus(bool in_focus){
	if (in_focus){
        //state.init = false;
        handle_bluetooth(bluetooth_connection_service_peek());
    }
}




static void battery_layer_update_callback(Layer *me, GContext* ctx) {
	graphics_context_set_stroke_color(ctx, foreground_color);
	graphics_context_set_fill_color(ctx, foreground_color);

	graphics_draw_rect(ctx, GRect(8, 1, 28, 13));
	graphics_draw_rect(ctx, GRect(7, 0, 30, 15));
	graphics_fill_rect(ctx, GRect(37, 3, 3, 9), 0, GCornerNone);

	if (state.battery_count == -1) {
		graphics_draw_line(ctx, GPoint(15, 8), GPoint(22, 6));
		graphics_draw_line(ctx, GPoint(22, 6), GPoint(22, 8));
		graphics_draw_line(ctx, GPoint(22, 8), GPoint(29, 6));
		return;
	}

	for (int i = 0; i < state.battery_count; i++) {
		graphics_fill_rect(ctx, GRect(10+5*i, 3, 4, 9), 0, GCornerNone);
	}
}





void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	static char time_text[] = "00:00";
	static char date_text[] = "00 Xxxxxxxxx";
	static char day_text[] = "XX";

	char *time_format;
	int wday = tick_time->tm_wday - 1;
	if (wday == -1) {
		wday = 6;
	}

	if (tick_time->tm_mon != state.month || tick_time->tm_mday != state.day || !state.init) {
		strftime(date_text, sizeof(date_text), "%e %B", tick_time);
		text_layer_set_text(text_date_layer, date_text);
		state.day = tick_time->tm_mday;
		state.month = tick_time->tm_mon;
	}

	if (wday != state.wday || !state.init) {
		memmove(day_text, days + 2*wday, 2);
		layer_set_frame(text_layer_get_layer(text_day_layer), GRect(118, 140, 30, 30));
		text_layer_set_text(text_day_layer, day_text);
		state.wday = wday;
	}

	if (clock_is_24h_style()) {
		time_format = "%R";
	} else {
		time_format = "%I:%M";
	}

	strftime(time_text, sizeof(time_text), time_format, tick_time);

	if (!clock_is_24h_style() && (time_text[0] == '0')) {
		memmove(time_text, &time_text[1], sizeof(time_text) - 1);
	}

	text_layer_set_text(text_time_layer, time_text);
}

static void handle_battery(BatteryChargeState charge_state) {
	layer_mark_dirty(battery_layer);
	if (charge_state.is_charging) {
		state.battery_count = -1;
		return;
	}

	state.battery_count = (charge_state.charge_percent / 10 + 1) / 2;
}

void set_style(void) {
    bool inverse = persist_read_bool(STYLE_KEY);
    
    background_color  = inverse ? GColorWhite : GColorBlack;
    foreground_color  = inverse ? GColorBlack : GColorWhite;
    compositing_mode  = inverse ? GCompOpAssign : GCompOpAssignInverted;
    
    window_set_background_color(window, background_color);
    
    layer_set_update_proc(battery_layer, &battery_layer_update_callback);
    
    text_layer_set_text_color(text_time_layer, foreground_color);
    text_layer_set_text_color(text_day_layer, foreground_color);
    text_layer_set_text_color(text_date_layer, foreground_color);

    
    
    
    bitmap_layer_set_compositing_mode(bluetoothLayer, compositing_mode);
}


void handle_tap(AccelAxisType axis, int32_t direction) {
    persist_write_bool(STYLE_KEY, !persist_read_bool(STYLE_KEY));
    set_style();
    //force_update();
    vibes_long_pulse();
    accel_tap_service_unsubscribe();
}

static void window_load(Window *window) {
    
    
	
	window_set_background_color(window, background_color);
	
	Layer *root_layer = window_get_root_layer(window);
    
	GRect bounds = layer_get_bounds(root_layer);
    
    window_layer = layer_create(bounds);
    layer_add_child(root_layer, window_layer);
    
    
	
	statusbar = layer_create(GRect(0,0,DEVICE_WIDTH,DEVICE_HEIGHT));
    
    layer_add_child(window_layer, statusbar);
    
	//Layer for current date "December 8"
	text_date_layer = text_layer_create(GRect(7, 140, bounds.size.w-7, bounds.size.h-140));
	text_layer_set_text_color(text_date_layer, foreground_color);
	text_layer_set_background_color(text_date_layer, GColorClear);
	text_layer_set_font(text_date_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURAB_21)));
	layer_add_child(window_layer, text_layer_get_layer(text_date_layer));
    
	//Layer for current time "17:35"
	text_time_layer = text_layer_create(GRect(7, 40, bounds.size.w-7, bounds.size.h-40));
	text_layer_set_text_color(text_time_layer, foreground_color);
	text_layer_set_background_color(text_time_layer, GColorClear);
	text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURAB_64)));
	layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
    
	//Layer for current day of the week "SU"
	text_day_layer = text_layer_create(GRect(7, 140, bounds.size.w-7, bounds.size.h-92));
	text_layer_set_text_color(text_day_layer, foreground_color);
	text_layer_set_background_color(text_day_layer, GColorClear);
	text_layer_set_font(text_day_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_FUTURAB_21)));
	layer_add_child(window_layer, text_layer_get_layer(text_day_layer));
	text_layer_set_text(text_day_layer, "YO");
    
	
    
	//Battery status icon
	battery_layer = layer_create(GRect(99, 4, 40, 40));
	layer_set_update_proc(battery_layer, &battery_layer_update_callback);
	layer_add_child(window_layer, battery_layer);
	
	
	
    
	
	bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
    
	bluetooth_rect = GRect(1,4,18,18);
	
    
	
	bluetoothLayer = bitmap_layer_create(bluetooth_rect);
    bitmap_layer_set_bitmap(bluetoothLayer, bluetooth);
	layer_add_child(statusbar, bitmap_layer_get_layer(bluetoothLayer));
    bitmap_layer_set_compositing_mode(bluetoothLayer, compositing_mode);
	
	
    
    handle_bluetooth(bluetooth_connection_service_peek());
    
}

static void window_unload(Window *window) {
    
    
    bitmap_layer_destroy(bluetoothLayer);
    text_layer_destroy(text_date_layer);
    text_layer_destroy(text_time_layer);
    text_layer_destroy(text_day_layer);
    
    
    layer_destroy(battery_layer);
    layer_destroy(statusbar);
    
    
    
    gbitmap_destroy(bluetooth);
    
}


static void init() {
	
	
    time_t curTime = time(NULL);
    window = window_create();
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    
    const bool animated = true;
    window_stack_push(window, animated);
    tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
	
    
    //set_style();
    
    
	battery_state_service_subscribe(&handle_battery);
	
	bluetooth_connection_service_subscribe(&handle_bluetooth);
	app_focus_service_subscribe(&handle_appfocus);
	
	handle_battery(battery_state_service_peek());
	
    //accel_tap_service_subscribe(handle_tap);
	
	
	handle_minute_tick(localtime(&curTime), MINUTE_UNIT);
	
	state.init = true;
    
	
}

static void deinit() {
    tick_timer_service_unsubscribe();
    battery_state_service_unsubscribe();
    bluetooth_connection_service_unsubscribe();
	app_focus_service_unsubscribe();
    window_stack_remove(window, false);
    window_destroy(window);
}

int main(void) {

	init();
	
	app_event_loop();
	
	deinit();
}