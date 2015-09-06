#include <pebble.h>
#define KEY_DIRECTION 0
#define KEY_DISTANCE 1


// GUI

Window *window;
TextLayer *text_layer;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

// LED

static const SmartstrapServiceId SERVICE_ID = 0x1001;
static const SmartstrapAttributeId LED_ATTRIBUTE_ID = 0x0001;
static const size_t LED_ATTRIBUTE_LENGTH = 1;
static const SmartstrapAttributeId UPTIME_ATTRIBUTE_ID = 0x0002;
static const size_t UPTIME_ATTRIBUTE_LENGTH = 4;

static SmartstrapAttribute *led_attribute;
static SmartstrapAttribute *uptime_attribute;

static void prv_availability_changed(SmartstrapServiceId service_id, bool available) {
  if (service_id != SERVICE_ID) {
    return;
  }

  if (available) {
    window_set_background_color(window, GColorGreen);
    printf("TurnAKit");
  } else {
    window_set_background_color(window, GColorRed);
    printf("Disconnected!");
  }
}

static void prv_did_read(SmartstrapAttribute *attr, SmartstrapResult result,
                         const uint8_t *data, size_t length) {
  if (attr != uptime_attribute) {
    return;
  }
  if (result != SmartstrapResultOk) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Read failed with result %d", result);
    return;
  }
  if (length != UPTIME_ATTRIBUTE_LENGTH) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Got response of unexpected length (%d)", length);
    return;
  }
}

static void prv_notified(SmartstrapAttribute *attribute) {
  if (attribute != uptime_attribute) {
    return;
  }
  smartstrap_attribute_read(uptime_attribute);
}


static void prv_set_led_attribute(int pin) {
  SmartstrapResult result;
  uint8_t *buffer;
  size_t length;
  result = smartstrap_attribute_begin_write(led_attribute, &buffer, &length);
  if (result != SmartstrapResultOk) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Begin write failed with error %d", result);
    return;
  }

  buffer[0] = pin;

  result = smartstrap_attribute_end_write(led_attribute, 1, false);
  if (result != SmartstrapResultOk) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "End write failed with error %d", result);
    return;
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  prv_set_led_attribute(23);
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  prv_set_led_attribute(21);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  prv_set_led_attribute(22);
}

static void click_config_provider(void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Hello %d", 1);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char direction_buffer[32];
  static char distance_buffer[8];

  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_DIRECTION:
      if (strcmp(t->value->cstring, "center") == 0) {
        prv_set_led_attribute(21);
        gbitmap_destroy(s_background_bitmap);
        bitmap_layer_destroy(s_background_layer);
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UP);
        s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 144));
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
        layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
        snprintf(direction_buffer, sizeof(direction_buffer), "%s", "center hit");
        break;
      } else if (strcmp(t->value->cstring, "left") == 0) {
        prv_set_led_attribute(23);
        gbitmap_destroy(s_background_bitmap);
        bitmap_layer_destroy(s_background_layer);
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LEFT);
        s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 144));
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
        layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
        snprintf(direction_buffer, sizeof(direction_buffer), "%s", "left hit");
        break;
      } else {
        prv_set_led_attribute(22);
        gbitmap_destroy(s_background_bitmap);
        bitmap_layer_destroy(s_background_layer);
        s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_RIGHT);
        s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 144));
        bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
        layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
        snprintf(direction_buffer, sizeof(direction_buffer), "%s", "right hit");
        break;
      }
    case KEY_DISTANCE:
      snprintf(distance_buffer, sizeof(distance_buffer), "%d Ft",  (int)t->value->int32);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  text_layer_destroy(text_layer);

  text_layer = text_layer_create(GRect(0, 120, 144, 25));
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_text(text_layer, distance_buffer);

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void init() {
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  window = window_create();
  window_stack_push(window, true);

  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_UP);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 144));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  text_layer = text_layer_create(GRect(0, 120, 144, 25));
  text_layer_set_background_color(text_layer, GColorClear);
  text_layer_set_text_color(text_layer, GColorWhite);
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_set_text(text_layer, "Loading...");

  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));

    // setup smartstrap
  SmartstrapHandlers handlers = (SmartstrapHandlers) {
    .availability_did_change = prv_availability_changed,
    .did_read = prv_did_read,
    .notified = prv_notified
  };
  smartstrap_subscribe(handlers);
  led_attribute = smartstrap_attribute_create(SERVICE_ID, LED_ATTRIBUTE_ID, LED_ATTRIBUTE_LENGTH);
  uptime_attribute = smartstrap_attribute_create(SERVICE_ID, UPTIME_ATTRIBUTE_ID,
                                                 UPTIME_ATTRIBUTE_LENGTH);

  window_set_click_config_provider(window, click_config_provider);
}

void deinit() {
  gbitmap_destroy(s_background_bitmap);
  bitmap_layer_destroy(s_background_layer);
  text_layer_destroy(text_layer);
  window_destroy(window);

  smartstrap_attribute_destroy(led_attribute);
  smartstrap_attribute_destroy(uptime_attribute);
}

int main() {
  init();
  app_event_loop();
  deinit();
  return 0;
}
