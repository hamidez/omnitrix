#include <lvgl.h>
#include <demos/lv_demos.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_CST8XX.h>
#include "ui.h"
#include "PCF8574.h" 
#include "soc/lcd_cam_struct.h"

#define BUZZER_PIN  44
TaskHandle_t soundTaskHandle = NULL;
#define BUZZER_CHANNEL 2
#define SOUND_OPEN  1
#define SOUND_CLOSE 2
#define SOUND_CHANGE  3

#define I2C_SDA_PIN 38
#define I2C_SCL_PIN 39
PCF8574 pcf8574(0x21);

#define ENCODER_A_PIN 42    
#define ENCODER_B_PIN 4     

volatile uint8_t currentA = 0; 
volatile uint8_t lastA = 0;   
volatile uint8_t currentSW = 0; 
volatile uint8_t swPin = 0;      
bool pressedFlag = false;      
volatile int pressCount = 0;            
const unsigned long debounceTime = 50;    
const unsigned long doubleClickTime = 300;  
volatile unsigned long singleClickTimeout = 0; 
volatile int8_t position_tmp = 2;   

#define SCREEN_BACKLIGHT_PIN 6
const int pwmFreq = 5000;
const int pwmChannel = 0;
const int pwmResolution = 8;

bool isQT = false;
TaskHandle_t encTaskHandle = NULL;
TaskHandle_t swTaskHandle = NULL;

#define I2C_TOUCH_ADDR 0x15  // often but not always 0x15!
Adafruit_CST8XX tsPanel = Adafruit_CST8XX();
enum Events lastevent = NONE;
bool isTestingTouch = false;

static const uint16_t screenWidth = 480;
static const uint16_t screenHeight = 480;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf1 = NULL;
static lv_color_t *buf2 = NULL;

lv_obj_t *current_screen = NULL;
int screen1_index = 1;

int launchAnimState = 0;
int launchAnimFrame = 1;
int launchAnimTotalFrames = 14;
const void * launchAnimFrameArray[] = { 
    &ui_img_launchanim0001_png,
    &ui_img_launchanim0002_png, 
    &ui_img_launchanim0003_png,
    &ui_img_launchanim0004_png,
    &ui_img_launchanim0005_png,
    &ui_img_launchanim0006_png,
    &ui_img_launchanim0007_png,
    &ui_img_launchanim0008_png,
    &ui_img_launchanim0009_png,
    &ui_img_launchanim0010_png,
    &ui_img_launchanim0011_png,
    &ui_img_launchanim0012_png, 
    &ui_img_launchanim0013_png,
    &ui_img_launchanim0014_png
};

int currentAlienId = -1;
int prevAlienId = -1;
int alienArrayTotal = 11;
int holoState = 0;
const void * alienArray[] = { 
    &ui_img_wildmutt_png,
    &ui_img_fourarms_png,
    &ui_img_greymatter_png,
    &ui_img_xlr8_png,
    &ui_img_upgrade_png,
    &ui_img_diamondhead_png,
    &ui_img_ripjaws_png,
    &ui_img_stinkfly_png,
    &ui_img_ghostfreak_png,
    &ui_img_heatblast_png,
    &ui_img_tuna_png
};
const void * alienArrayHolo[] = { 
    &ui_img_wildmutt_holo_png,
    &ui_img_fourarms_holo_png, 
    &ui_img_greymatter_holo_png,
    &ui_img_xlr8_holo_png,
    &ui_img_upgrade_holo_png,
    &ui_img_diamondhead_holo_png,
    &ui_img_ripjaws_holo_png,
    &ui_img_stinkfly_holo_png,
    &ui_img_ghostfreak_holo_png,
    &ui_img_heatblast_holo_png,
    &ui_img_tuna_holo_png
};

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
  16 /* CS */, 2 /* SCK */, 1 /* SDA */,
  40 /* DE */, 7 /* VSYNC */, 15 /* HSYNC */, 41 /* PCLK */,
  46 /* R0 */, 3 /* R1 */, 8 /* R2 */, 18 /* R3 */, 17 /* R4 */,
  14 /* G0/P22 */, 13 /* G1/P23 */, 12 /* G2/P24 */, 11 /* G3/P25 */, 10 /* G4/P26 */, 9 /* G5 */,
  5 /* B0 */, 45 /* B1 */, 48 /* B2 */, 47 /* B3 */, 21 /* B4 */
);

Arduino_ST7701_RGBPanel *gfx = new Arduino_ST7701_RGBPanel(
  bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
  false /* IPS */, 480 /* width */, 480 /* height */,
  st7701_type5_init_operations, sizeof(st7701_type5_init_operations),
  true /* BGR */,
  5 /* hsync_front_porch(10) */, 4 /* hsync_pulse_width(8) */, 5 /* hsync_back_porch(50) */,
  10 /* vsync_front_porch(10) */, 4 /* vsync_pulse_width(8) */, 10 /* vsync_back_porch(20) */);

/* Display flushing */

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {    
  //vsync
  if (area->y1 == 0) {        
      unsigned long start = micros();        
      // If VSYNC is currently LOW (Active), we are in the middle of a sync pulse.
      while (gpio_get_level((gpio_num_t)7) == 0) {
      //while(digitalRead(7) == 0) {
            if (micros() - start > 20000) break; // Safety Timeout (20ms)
      }
      // We wait for the signal to fall from HIGH to LOW.
      // This exact moment marks the END of the frame
      while (gpio_get_level((gpio_num_t)7) == 1) {
      //while(digitalRead(7) == 1) {
            if (micros() - start > 20000) break; // Safety Timeout (20ms)
      }        
  }
  //draw next frame
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  //Cache_WriteBack_Addr((uint32_t)color_p, w * h * 2);
  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);  
  lv_disp_flush_ready(disp);
}

/*Read CST826 the touchpad*/
const int SWIPE_THRESHOLD = 100;  
const int TIME_THRESHOLD = 300;   
const int VERTICAL_LIMIT = 100;    
int startX = 0;
int startY = 0;
unsigned long startTime = 0;
bool trackingSwipe = false;
bool first_click = false;

void initBacklight() {
  ledcSetup(pwmChannel, pwmFreq, pwmResolution);
  ledcAttachPin(SCREEN_BACKLIGHT_PIN, pwmChannel);
  ledcWrite(pwmChannel, 255);
}

void setup() {
  Serial.begin(115200); /* prepare for possible serial debug */
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  pcf8574.pinMode(P0, OUTPUT);        //tp RST
  pcf8574.pinMode(P2, OUTPUT);        //tp INT
  pcf8574.pinMode(P3, OUTPUT);        //lcd power
  pcf8574.pinMode(P4, OUTPUT);        //lcd reset
  pcf8574.pinMode(P5, INPUT_PULLUP);  //encoder SW

  Serial.print("Init pcf8574...\n");
  if (pcf8574.begin()) {
    Serial.println("pcf8574 OK");
  } else {
    Serial.println("pcf8574 KO");
  }

  pcf8574.digitalWrite(P3, HIGH);
  delay(100);

  /*lcd reset*/
  pcf8574.digitalWrite(P4, HIGH);
  delay(100);
  pcf8574.digitalWrite(P4, LOW);
  delay(120);
  pcf8574.digitalWrite(P4, HIGH);
  delay(120);
  /*end*/

  /*tp RST*/
  pcf8574.digitalWrite(P0, HIGH);
  delay(100);
  pcf8574.digitalWrite(P0, LOW);
  delay(120);
  pcf8574.digitalWrite(P0, HIGH);
  delay(120);
  /*tp INT*/
  pcf8574.digitalWrite(P2, HIGH);
  delay(120);

  gfx->begin(14000000);
  gfx->fillScreen(BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(80, 100);

  if (!tsPanel.begin(&Wire, I2C_TOUCH_ADDR)) {
    Serial.println("No touchscreen found");
  } else {
    Serial.println("Touchscreen found");
  }

  pinMode(ENCODER_A_PIN, INPUT);
  pinMode(ENCODER_B_PIN, INPUT);
  lastA = digitalRead(ENCODER_A_PIN);

  pinMode(BUZZER_PIN, OUTPUT);
  PIN_INPUT_ENABLE(IO_MUX_GPIO7_REG);

  lv_init();
  
  uint32_t buffer_pixels = 360 * 480; // Full screen is better for S3
  size_t buffer_size_bytes = buffer_pixels * sizeof(lv_color_t);

  buf1 = (lv_color_t *)heap_caps_aligned_alloc(64, buffer_size_bytes, MALLOC_CAP_SPIRAM);
  buf2 = (lv_color_t *)heap_caps_aligned_alloc(64, buffer_size_bytes, MALLOC_CAP_SPIRAM);

  static lv_disp_draw_buf_t draw_buf;
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, buffer_pixels);

  /*Initialize the display*/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  /*Change the following line to your display resolution*/
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.direct_mode = 0;
  disp_drv.full_refresh = 0; 
  disp_drv.sw_rotate = 0;
  disp_drv.screen_transp = 0;
  //disp_drv.monitor_cb = my_monitor_cb; //a function that will be called once a frame draw is complete
  lv_disp_drv_register(&disp_drv);

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  /*Or try out a demo. Don't forget to enable the demos in lv_conf.h. E.g. LV_USE_DEMOS_WIDGETS*/
  // lv_demo_widgets();
  ui_init();
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x6E6E6E), LV_PART_MAIN);

  delay(200);
  initBacklight();
  pcf8574.digitalWrite(P3, LOW);
  ledcSetup(BUZZER_CHANNEL, 2000, 8);

  xTaskCreatePinnedToCore(encTask, "ENC", 2048, NULL, 1, &encTaskHandle, 0);
  xTaskCreatePinnedToCore(playOmnitrixSoundTask, "SndTask", 2048, NULL, 1, &soundTaskHandle, 0);

  Serial.println("Setup done");
}

void loop() {
  lv_timer_handler(); 
  handleLaunchAnim();
  handleAlienUpdate();
}

void handleLaunchAnim(){  
    if(launchAnimState == 0 && launchAnimFrame > 1){
      //reverse playing launch anim
      launchAnimFrame--;
      lv_img_set_src(ui_launchAnim, launchAnimFrameArray[launchAnimFrame-1]);
    }
    if(launchAnimState == 1 && launchAnimFrame < launchAnimTotalFrames){
      //playing launch anim
      launchAnimFrame++; 
      lv_img_set_src(ui_launchAnim, launchAnimFrameArray[launchAnimFrame-1]); 
      if(launchAnimFrame == launchAnimTotalFrames){
        //show first alien
        changeAlien(0, false);
        showAlien();        
      }
    }
}

void handleAlienUpdate(){
  if(prevAlienId != currentAlienId){
    if(holoState == 0){
      lv_img_set_src(ui_alien, alienArray[currentAlienId]);
    }else{
      lv_img_set_src(ui_alien, alienArrayHolo[currentAlienId]);
    }
    prevAlienId = currentAlienId;
  }
}

void showAlien(){
  lv_obj_clear_flag(ui_alien, LV_OBJ_FLAG_HIDDEN);
}

void hideAlien(){
  lv_obj_add_flag(ui_alien, LV_OBJ_FLAG_HIDDEN);
}

void changeAlien(int alienArrayId, bool playChangeSound){
  if(alienArrayId > -1 && alienArrayId < alienArrayTotal){
    currentAlienId = alienArrayId;
    if(playChangeSound){
      triggerOmnitrixSound(SOUND_CHANGE);
    }
  }
}

void switchHoloState(){
  if(launchAnimState == 1 && launchAnimFrame == launchAnimTotalFrames){ //if we're not in he launch anim state
    if(holoState == 0){
      //enable hologram
      holoState = 1;
      //force refresh alien visual
      prevAlienId = -1;
      //make bg black
      lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
      lv_obj_add_flag(ui_launchAnim, LV_OBJ_FLAG_HIDDEN);
    }else{
      //disable hologram
      holoState = 0;
      //force refresh alien visual
      prevAlienId = -1;
      //make bg gray
      lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x6E6E6E), LV_PART_MAIN);
      lv_obj_clear_flag(ui_launchAnim, LV_OBJ_FLAG_HIDDEN);   
    }
  }
}

void playOmnitrixSoundTask(void *pvParameters) {
    ledcSetup(BUZZER_CHANNEL, 2000, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
    ledcWriteTone(BUZZER_CHANNEL, 0);

    uint32_t receivedSound = 0;

    while (1) {
      // WAIT HERE until a notification arrives. 
      // The sound id is stored in 'receivedSound'
      receivedSound = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

      if (receivedSound == SOUND_OPEN) { 
        // charge up
        float freq = 2500;
        float target = 6500;
        int steps = 60; 
        float stepSize = (target - freq) / steps;
        for (int i = 0; i < steps; i++) {
            int jitter = (i % 2 == 0) ? 40 : -40;
            ledcWriteTone(BUZZER_CHANNEL, (int)freq + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
            freq += stepSize;
        }
        // silence gap
        ledcWriteTone(BUZZER_CHANNEL, 0);
        vTaskDelay(pdMS_TO_TICKS(30));
        // final electronic tune
        for (int i = 0; i < 12; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 700 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        for (int i = 0; i < 12; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 1500 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        for (int i = 0; i < 10; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 700 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        for (int i = 0; i < 10; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 500 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }        
      }else if (receivedSound == SOUND_CLOSE) { 
        // power down
        float freq = 4500;
        float target = 2000;
        int steps = 40; 
        float stepSize = (freq - target) / steps;
        for (int i = 0; i < steps; i++) {
            int jitter = (i % 2 == 0) ? 40 : -40;
            ledcWriteTone(BUZZER_CHANNEL, (int)freq + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
            freq -= stepSize;
        }
        // silence gap
        ledcWriteTone(BUZZER_CHANNEL, 0);
        vTaskDelay(pdMS_TO_TICKS(100));
        // final tune
        for (int i = 0; i < 10; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 700 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ledcWriteTone(BUZZER_CHANNEL, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
        for (int i = 0; i < 10; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 700 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ledcWriteTone(BUZZER_CHANNEL, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
        for (int i = 0; i < 10; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 700 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
      }else if (receivedSound == SOUND_CHANGE) { 
        // alien change tune
        for (int i = 0; i < 10; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 700 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ledcWriteTone(BUZZER_CHANNEL, 0);
        vTaskDelay(pdMS_TO_TICKS(50));
        for (int i = 0; i < 14; i++) { 
            int jitter = (i % 2 == 0) ? 10 : -10;
            ledcWriteTone(BUZZER_CHANNEL, 1200 + jitter);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
      }

      // DONE
      ledcWriteTone(BUZZER_CHANNEL, 0);
    }
}

void triggerOmnitrixSound(int soundType) {
    if (soundTaskHandle != NULL) {
        xTaskNotify(soundTaskHandle, soundType, eSetValueWithOverwrite);
    }
}

int last_counter = 0;
int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir = "";
bool one_test = false;

void performClickAction() {
  current_screen = lv_scr_act();
}

void performDoubleClickAction() {
  current_screen = lv_scr_act();
}

void processEncoder() {
  //handle rotary encoder
  current_screen = lv_scr_act();
  //Serial.println(position_tmp);
  if(position_tmp == 0){
    //rotated counter-clockwise
    if(launchAnimState == 1 && launchAnimFrame == launchAnimTotalFrames){
      if(currentAlienId <= 0 && holoState == 0){
        //reverse launch animation
        if(millis() > 3000){//prevent false trigger at startup
          launchAnimState = 0;
          hideAlien();
          triggerOmnitrixSound(SOUND_CLOSE);
        }
      }else{
        //previous alien
        changeAlien(currentAlienId - 1, true);
      }
    }
  }
  if(position_tmp == 1){
    //rotated clockwise
    //check for launch anim start
    if(launchAnimState == 0 && launchAnimFrame == 1){
      if(millis() > 3000){//prevent false trigger at startup
        launchAnimState = 1;
        triggerOmnitrixSound(SOUND_OPEN);
      }
    }
    //check for alien change
    if(launchAnimState == 1 && launchAnimFrame == launchAnimTotalFrames){
      //next alien
      changeAlien(currentAlienId + 1, true);
    }
  }
}


void encTask(void *pvParameters) {
  while (1) {
    // Read the current state of CLK
    currentStateCLK = digitalRead(ENCODER_A_PIN);
    // If last and current state of CLK are different, then pulse occurred
    // React to only 1 state change to avoid double count
    if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
      current_screen = lv_scr_act();
      // If the DT state is different than the CLK state then
      // the encoder is rotating CCW so decrement
      if (digitalRead(ENCODER_B_PIN) != currentStateCLK) {
        if (abs(last_counter - counter) > 10) {
          continue;
        }
        position_tmp = 0;

        counter++;
        currentDir = "CCW";
      } else {
        if (one_test == false)  
        {
          one_test = true;
          continue;
        }
        position_tmp = 1;
        counter--;
        currentDir = "CW";
      }

      //Serial.print("Direction: ");
      //Serial.print(currentDir);
      //Serial.print(" | Counter: ");
      //Serial.println(counter);
      last_counter = counter;
      processEncoder();
    }

    if (pressedFlag)
    {
      if (pressCount == 1 && millis() >= singleClickTimeout) {
        //Serial.println("Single Click Detected");
        performClickAction();
        pressCount = 0;
        pressedFlag = false;

      }
      else if (pressCount >= 2) {
        //Serial.println("Double Click Detected");
        performDoubleClickAction();
        pressCount = 0;
        pressedFlag = false;
      }
    }
    // Remember last CLK state
    lastStateCLK = currentStateCLK;
    vTaskDelay(pdMS_TO_TICKS(25));
  }
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
  static unsigned long touchStartTime = 0;
  static bool longPressTriggered = false;

  if (tsPanel.touched()) {
    if (touchStartTime == 0) {
        // New touch just started
        touchStartTime = millis();
        longPressTriggered = false; 
    }else {
        if (!longPressTriggered && (millis() - touchStartTime > 4000)) {
            //4 second touch detected
            longPressTriggered = true; 
            switchHoloState();
        }
    }
  } 
  else {
    // Touch released - Reset everything
    data->state = LV_INDEV_STATE_REL;
    touchStartTime = 0;
    longPressTriggered = false;
  }
}



