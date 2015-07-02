#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static BitmapLayer *bitmap_layer;

static char message[20];
static char timebuf[10];
static void updater();
static unsigned int going_since;
static void draw_clock(struct Layer *layer, GContext *ctx);
  
static const unsigned int time_to_go[] = { 7*60+25, 10*60, 12*60+20, 15*60, 17*60, 19*60+30, 22*60+24, 23*60+35 };

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Hej!");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  vibes_cancel();
  going_since = 0;

  layer_set_hidden((Layer*) bitmap_layer, false);
  layer_set_hidden((Layer*) text_layer, true);  
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Du!");
}

static void back_click_handler(ClickRecognizerRef recognizer, void *context) {
  //  text_layer_set_text(text_layer, "");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  text_layer = text_layer_create((GRect) { .origin = { 0, bounds.size.h/2-25 }, .size = { bounds.size.w, 50 } });
  bitmap_layer = bitmap_layer_create((GRect) { .origin = {0,0}, .size = {bounds.size.w, bounds.size.h}});
  
  // Improve the layout to be more like a watchface
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);

  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));

  layer_set_update_proc((Layer*) bitmap_layer, draw_clock);
  updater();
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

static void go_now(unsigned int now) {

  layer_set_hidden((Layer*) bitmap_layer, true);
  layer_set_hidden((Layer*) text_layer, false);
  
  going_since = now;
  vibes_double_pulse();
  text_layer_set_text(text_layer,"Dags!");
}


static void draw_clock (struct Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint middle =  { bounds.size.w/2,
		     bounds.size.h/2};
  int clocksize = bounds.size.w/2-8;

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_circle(ctx, middle, clocksize);

  graphics_draw_text(ctx, "12",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {bounds.size.w/2-14, 0},
			 .size = {28,14} }, GTextOverflowModeFill,GTextAlignmentCenter, NULL);

  graphics_draw_text(ctx, "3",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {bounds.size.w-15, bounds.size.h/2-7},
			 .size = {14,14} }, GTextOverflowModeFill,GTextAlignmentRight, NULL);

  graphics_draw_text(ctx, "9",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {1, bounds.size.h/2-7},
			 .size = {14,14} }, GTextOverflowModeFill,GTextAlignmentLeft, NULL);

  graphics_draw_text(ctx, "6",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {bounds.size.w/2-7, bounds.size.h-14},
			 .size = {14,14} }, GTextOverflowModeFill,GTextAlignmentCenter, NULL);
  
  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);

  int32_t hour_angle = TRIG_MAX_ANGLE*(t->tm_hour % 12)/12;
  GPoint hour = { bounds.size.w/2 + (sin_lookup(hour_angle) * clocksize*7/16 / TRIG_MAX_RATIO) ,
		  bounds.size.h/2 - (cos_lookup(hour_angle) * clocksize*7/16 / TRIG_MAX_RATIO)};
  
  graphics_draw_line(ctx, middle, hour);

  int32_t minute_angle = TRIG_MAX_ANGLE*t->tm_min/60;
  GPoint minute = { bounds.size.w/2 + (sin_lookup(minute_angle) * clocksize*12/16 / TRIG_MAX_RATIO) ,
		  bounds.size.h/2 - (cos_lookup(minute_angle) * clocksize*12/16 / TRIG_MAX_RATIO)};
  
  graphics_draw_line(ctx, middle, minute);


  for (int i=0; i<12; i++) {
    int32_t mark_angle = TRIG_MAX_ANGLE*i/12;
    GPoint mark_outer =  { bounds.size.w/2 + (sin_lookup(mark_angle) * clocksize / TRIG_MAX_RATIO) ,
			   bounds.size.h/2 - (cos_lookup(mark_angle) * clocksize / TRIG_MAX_RATIO)};

    GPoint mark_inner =  { bounds.size.w/2 + (sin_lookup(mark_angle) * clocksize * 15 / 16 / TRIG_MAX_RATIO) ,
			   bounds.size.h/2 - (cos_lookup(mark_angle) * clocksize * 15 / 16 / TRIG_MAX_RATIO)};

    graphics_draw_line(ctx, mark_inner, mark_outer);
  }
}

static void updater() {
  
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  unsigned int time_now = tick_time->tm_hour*60+tick_time->tm_min;

  if (going_since && (((time_now - going_since) % 3) == 0))  {
    go_now(going_since);
    return;
  }
  
  for (unsigned int i=0; i < sizeof(time_to_go); i++) {
    if (time_now == time_to_go[i]) {
      go_now(time_now);
      return;
    }
  }
  
  layer_set_hidden((Layer*) bitmap_layer, false);
  layer_set_hidden((Layer*) text_layer, true); 
      
}


static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  updater();
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}