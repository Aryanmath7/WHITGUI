#include <lvgl.h>
#include <TFT_eSPI.h>
#include "lv_conf.h"
#include "CST816S.h"

#define EXAMPLE_LVGL_TICK_PERIOD_MS 2

static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */
CST816S touch(6, 7, 13, 5);                         // sda, scl, rst, irq

#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char *buf) {
  Serial.printf(buf);
  Serial.flush();
}
#endif

//Objects
lv_obj_t *battery;

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp_drv);
}

void example_increase_lvgl_tick(void *arg) {
  /* Tell LVGL how many milliseconds has elapsed */
  lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static uint8_t count = 0;
void example_increase_reboot(void *arg) {
  count++;
  if (count == 30) {
    // esp_restart();
  }
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {

  bool touched = touch.available();
  if (!touched)
  // if( 0!=touch.data.points )
  {
    data->state = LV_INDEV_STATE_REL;
  } else {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touch.data.x;
    data->point.y = touch.data.y;
  }
}

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */

  lv_init();
#if LV_USE_LOG != 0
  lv_log_register_print_cb(my_print); /* register print function for debugging */
#endif

  tft.begin();        /* TFT init */
  tft.setRotation(0); /* Landscape orientation, flipped */

  touch.begin();

  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
  //lv_disp_set_bg_color(&disp_drv, lv_color_hex(0x212529))
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x212529), LV_PART_MAIN);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };

  const esp_timer_create_args_t reboot_timer_args = {
    .callback = &example_increase_reboot,
    .name = "reboot"
  };

  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);

  esp_timer_handle_t reboot_timer = NULL;
  esp_timer_create(&reboot_timer_args, &reboot_timer);
  esp_timer_start_periodic(reboot_timer, 2000 * 1000);

  lv_example_scroll_2();
  initBatteryMeter();
  changeBattery(battery, 0, 63);


  Serial.println("Setup done");
}

void loop() {
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}


void button(void) {

  lv_obj_t *btn = lv_btn_create(lv_scr_act());                    /*Add a button to the current screen*/
  lv_obj_set_pos(btn, 10, 10);                                    /*Set its position*/
  lv_obj_set_size(btn, 100, 50);                                  /*Set its size*/
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, NULL); /*Assign a callback to the button*/

  lv_obj_t *label = lv_label_create(btn); /*Add a label to the button*/
  lv_label_set_text(label, "Button");     /*Set the labels text*/
  lv_obj_center(label);
}


void btn_event_cb(lv_event_t *e) {
  printf("Clicked\n");
}


static void set_angle(void *obj, int32_t v) {
  lv_obj_t *arc = (lv_obj_t *)obj;  // Cast the object pointer to lv_obj_t *
  lv_arc_set_value(arc, v);
}

void initBatteryMeter(void) {
  battery = lv_arc_create(lv_scr_act());
  lv_obj_set_size(battery, 240, 240);
  lv_arc_set_rotation(battery, 270);
  lv_arc_set_bg_angles(battery, 0, 360);
  lv_obj_remove_style(battery, NULL, LV_PART_KNOB);  /*Be sure the knob is not displayed*/
  lv_obj_clear_flag(battery, LV_OBJ_FLAG_CLICKABLE); /*To not allow adjusting by click*/
  lv_obj_center(battery);
  lv_obj_set_style_arc_width(battery, 10, LV_PART_MAIN);       // Changes background arc width
  lv_obj_set_style_arc_width(battery, 10, LV_PART_INDICATOR);  // Changes set part width
  lv_obj_clear_flag(battery, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_arc_color(battery, lv_color_hex(0x373d43), LV_PART_MAIN);
}

void changeBattery(lv_obj_t *battery, int32_t startVal, int32_t stopVal) {
  lv_anim_t anim;
  lv_anim_init(&anim);
  lv_anim_set_var(&anim, battery);
  lv_anim_set_exec_cb(&anim, set_angle);
  lv_anim_set_time(&anim, 1000);
  lv_anim_set_values(&anim, startVal, stopVal);
  lv_anim_start(&anim);

  if (stopVal <= 33) {
    lv_obj_set_style_arc_color(battery, lv_color_hex(0xf94144), LV_PART_INDICATOR);
  } else if (stopVal <= 66) {
    lv_obj_set_style_arc_color(battery, lv_color_hex(0xf3722c), LV_PART_INDICATOR);
  } else if (stopVal <= 100) {
    lv_obj_set_style_arc_color(battery, lv_color_hex(0x43aa8b), LV_PART_INDICATOR);
  }
}

/**
 * Show an example to scroll snap
 */
void lv_example_scroll_2(void) {
  lv_obj_t *panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(panel, 240, 240);
  lv_obj_set_scroll_snap_y(panel, LV_SCROLL_SNAP_CENTER);
  lv_obj_set_scroll_snap_x(panel, LV_SCROLL_SNAP_CENTER);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_center(panel);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x181a1c), LV_PART_MAIN);

  uint32_t i;
  for (i = 0; i < 5; i++) {
    lv_obj_t *subPanel = lv_obj_create(panel);
    lv_obj_set_size(subPanel, 240, 240);
    lv_obj_set_style_bg_color(subPanel, lv_color_hex(0x212529), LV_PART_MAIN);
    lv_obj_set_style_radius(subPanel, 120, LV_PART_MAIN);
    lv_obj_set_style_border_width(subPanel, 0, LV_PART_MAIN);
    lv_obj_center(subPanel);

    if (i == 0) {
      lv_obj_t *label = lv_label_create(subPanel);
      lv_label_set_text(label, "0436");
      lv_obj_align(label, LV_ALIGN_CENTER, -40, 0);
      lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(label, 60);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);

      static lv_style_t style;
      lv_style_init(&style);
      lv_style_set_text_font(&style, &lv_font_montserrat_40);
      lv_obj_add_style(label, &style, 0);



    } else {
      lv_obj_t *label = lv_label_create(subPanel);
      lv_label_set_text_fmt(label, "Panel %" LV_PRIu32, i);
      lv_obj_center(label);
    }
  }
  lv_obj_update_snap(panel, LV_ANIM_ON);
}
