#include <pebble.h>


static Window *window;
static TextLayer *text_layer;
static BitmapLayer *bitmap_layer;

static int clockmode = 0;

static char message[20];
static char going_string[46];

static void updater();

static int going_since = -1;
static const char* current_msg = NULL;

static void draw_clock(struct Layer *layer, GContext *ctx);
static GBitmap *watchface;

static unsigned int inverted = false;
static const char fmsg[] = "Dags!";
static const char smsg[] = "Gå in!";


static const unsigned int time_to_go[] = { 9*60+30,
					   12*60+8,
					   14*60+00,
					   15*60+00,
					   17*60+00,
					   18*60+02,
					   19*60+30,
                                         };

static unsigned char wdays[] = {
  255,
  255,
  255,
  255,
  255,
  255,
  255,
};

static const char* msgs[] = { fmsg,
		       fmsg,
		       fmsg,
		       smsg,
		       fmsg,
		       smsg,
		       fmsg,
};

		

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  inverted = !inverted;
  
  gbitmap_destroy(watchface);

  watchface = gbitmap_create_with_resource(inverted ?  RESOURCE_ID_WATCHFACEINVERTED :
					    RESOURCE_ID_WATCHFACE
					   );
      
  updater();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  vibes_cancel();
  going_since = -1;
  current_msg = NULL;
  
  layer_set_hidden((Layer*) bitmap_layer, false);
  layer_set_hidden((Layer*) text_layer, true);  
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  clockmode = (clockmode +1) % 3;
  updater();
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
  
  text_layer = text_layer_create((GRect) { .origin = { 0, bounds.size.h/2-25 },
	                                   .size = { bounds.size.w, 50 } });
  bitmap_layer = bitmap_layer_create((GRect) { .origin = {0,0},
                                   .size = {bounds.size.w, bounds.size.h}});
 
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

  bitmap_layer_destroy(bitmap_layer);
}

static void go_now(unsigned int now) {

  layer_set_hidden((Layer*) bitmap_layer, true);
  layer_set_hidden((Layer*) text_layer, false);

  going_since = now;
  vibes_double_pulse();
  text_layer_set_text(text_layer,current_msg);
}

static void draw_go_overlay(struct Layer *layer, GContext *ctx, GRect *bounds) {

  if ( -1 == going_since || !current_msg)
    return;

  snprintf(going_string,  sizeof(going_string), "%d:%02d   %s",
	   going_since/60,
	   going_since % 60,
	   current_msg);
  
  graphics_draw_text(ctx, going_string,
		     fonts_get_system_font( FONT_KEY_GOTHIC_18_BOLD  ),
		     (GRect) { .origin = {0,bounds->size.h-19},
			 .size = {bounds->size.w,18} },
		     GTextOverflowModeFill,
		     GTextAlignmentCenter, NULL);
  
}





static void draw_digital(struct Layer *layer, GContext *ctx, GRect *bounds) {

  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);

  strftime(message, sizeof(message)-1, "%H:%M", t);
        
  graphics_draw_text(ctx, message,
		     fonts_get_system_font( FONT_KEY_BITHAM_42_BOLD  ),
		     (GRect) { .origin = {0,bounds->size.h/2-22},
			 .size = {bounds->size.w,42} },
		     GTextOverflowModeFill,
		     GTextAlignmentCenter, NULL);

   
}


static void draw_analog_hands(struct Layer *layer, GContext *ctx, GRect* bounds, int xtrans, int ytrans) {

  GPoint middle =  { bounds->size.w/2 + xtrans,
		     bounds->size.h/2 + ytrans};
  int clocksize = bounds->size.w/2-8;
  
  time_t temp = time(NULL); 
  struct tm *t = localtime(&temp);

  int32_t hour_angle = TRIG_MAX_ANGLE*( ( (t->tm_hour % 12)*60+
					  t->tm_min))/(12*60);
  GPoint hour = { bounds->size.w/2 +
		  (sin_lookup(hour_angle) * clocksize*7/16
		   / TRIG_MAX_RATIO) + xtrans,
		  bounds->size.h/2 -
		  (cos_lookup(hour_angle) * clocksize*7/16
		   / TRIG_MAX_RATIO) + ytrans};
  
  graphics_draw_line(ctx, middle, hour);

  int32_t minute_angle = TRIG_MAX_ANGLE*t->tm_min/60;
  GPoint minute = { bounds->size.w/2 + (sin_lookup(minute_angle) * clocksize*12/16 / TRIG_MAX_RATIO) + xtrans,
		  bounds->size.h/2 - (cos_lookup(minute_angle) * clocksize*12/16 / TRIG_MAX_RATIO) + ytrans};
  
  graphics_draw_line(ctx, middle, minute);

}

static void draw_analog_nice(struct Layer *layer, GContext *ctx, GRect* bounds) {

  graphics_draw_bitmap_in_rect(ctx, watchface, *bounds);
  draw_analog_hands(layer, ctx, bounds, 0, 3);
}
  
static void draw_analog_simple(struct Layer *layer, GContext *ctx, GRect* bounds) {

  int clocksize = bounds->size.w/2-8;
    
  GPoint middle =  { bounds->size.w/2,
		     bounds->size.h/2};
  
  graphics_draw_circle(ctx, middle, clocksize);

  graphics_draw_text(ctx, "12",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {bounds->size.w/2-14, 0},
			 .size = {28,14} }, GTextOverflowModeFill,GTextAlignmentCenter, NULL);

  graphics_draw_text(ctx, "3",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {bounds->size.w-15, bounds->size.h/2-7},
			 .size = {14,14} }, GTextOverflowModeFill,GTextAlignmentRight, NULL);

  graphics_draw_text(ctx, "9",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {1, bounds->size.h/2-7},
			 .size = {14,14} }, GTextOverflowModeFill,GTextAlignmentLeft, NULL);

  graphics_draw_text(ctx, "6",  fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
		     (GRect) { .origin = {bounds->size.w/2-7, bounds->size.h-14},
			 .size = {14,14} }, GTextOverflowModeFill,GTextAlignmentCenter, NULL);

  draw_analog_hands(layer, ctx, bounds, 0, 0);
  
  for (int i=0; i<12; i++) {
    int32_t mark_angle = TRIG_MAX_ANGLE*i/12;
    GPoint mark_outer =  { bounds->size.w/2 + (sin_lookup(mark_angle) * clocksize / TRIG_MAX_RATIO) ,
			   bounds->size.h/2 - (cos_lookup(mark_angle) * clocksize / TRIG_MAX_RATIO)};

    GPoint mark_inner =  { bounds->size.w/2 + (sin_lookup(mark_angle) * clocksize * 15 / 16 / TRIG_MAX_RATIO) ,
			   bounds->size.h/2 - (cos_lookup(mark_angle) * clocksize * 15 / 16 / TRIG_MAX_RATIO)};

    graphics_draw_line(ctx, mark_inner, mark_outer);
   }
}

static void draw_clock (struct Layer *layer, GContext *ctx) {

  GRect bounds = layer_get_bounds(layer);
  
  if (!inverted) {
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_context_set_stroke_color(ctx, GColorBlack);
  } else {
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_context_set_stroke_color(ctx, GColorWhite);
  }

 graphics_fill_rect(ctx, bounds, 0, 0);
  
  switch  (clockmode) {
    
  case 0: 
    draw_analog_simple(layer, ctx, &bounds);
    break;
  case 1:
    draw_analog_nice(layer, ctx, &bounds);
    break;
  case 2:
    draw_digital(layer, ctx, &bounds);
    break;
  }

  draw_go_overlay(layer, ctx, &bounds);

}

static void updater() {
  
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  unsigned int time_now = tick_time->tm_hour*60+tick_time->tm_min;

  layer_mark_dirty((Layer*) bitmap_layer);
  
  if ((going_since != -1) && (((time_now - going_since) % 3) == 0))  {
    go_now(going_since);
    return;
  }
  
  for (unsigned int i=0; i < sizeof(time_to_go); i++) {
    if (time_now == time_to_go[i]) {
      current_msg = msgs[i];
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

  watchface = gbitmap_create_with_resource(RESOURCE_ID_WATCHFACE);
  
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  window_destroy(window);
  gbitmap_destroy(watchface);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
