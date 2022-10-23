#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include <base64.h>

#define deBuG false

#define CAMERA_MODEL_WROVER_KIT

#include "camera_pins.h"

#include "config.h"
const char* _SSID   = SSID;
const char* _PWD    = PWD;

camera_config_t config;

void startCameraServer();
void config_init();

unsigned long tic = millis();
unsigned long toc = tic;
unsigned long delta = 1000;

uint8_t*  _jpg_buf = NULL;
size_t    _jpg_len = 0;

String capturePhoto(uint8_t*, size_t*);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  config_init();
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Initialized NO camera with error: 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  s->set_vflip(s, 1);       // 1: Upside down, 0: No operation
  s->set_hmirror(s, 0);     // 1: Reverse left and right, 0: No operation
  s->set_brightness(s, 1);  // Increment brightness level by a tick
  s->set_saturation(s, -1); // Decrement saturation level by a tick

  WiFi.begin(SSID, PWD);
  Serial.print("Making WiFi connection.");
  while (WiFi.isConnected() != true) {
    Serial.print(" .");
    delay(500);
  }
  Serial.println("Done!");

  if (deBuG) {
    startCameraServer();
    Serial.print("Use http://");
    Serial.print(WiFi.localIP());
    Serial.println(" to view!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  ;

  toc = millis();
  if (toc - tic >= delta) {
    tic = toc;

    if (!deBuG) {
      // Grab a snap
      String b64enc = String();
      b64enc = capturePhoto(_jpg_buf, &_jpg_len);
      // Encode in base64
      Serial.println(b64enc);

      // Ship to MQTT broker - MacBook

      // Ship to MQTT broker - HiveMQ

      // Send Telegram notification
    }

    // Check motion-sensor signal
    /*
    * TODO: Integrate motion-sensor
    * 
    */

  }
}

void config_init() {
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = Y2_GPIO_NUM;
  config.pin_d1       = Y3_GPIO_NUM;
  config.pin_d2       = Y4_GPIO_NUM;
  config.pin_d3       = Y5_GPIO_NUM;
  config.pin_d4       = Y6_GPIO_NUM;
  config.pin_d5       = Y7_GPIO_NUM;
  config.pin_d6       = Y8_GPIO_NUM;
  config.pin_d7       = Y9_GPIO_NUM;
  config.pin_xclk     = XCLK_GPIO_NUM;
  config.pin_pclk     = PCLK_GPIO_NUM;
  config.pin_vsync    = VSYNC_GPIO_NUM;
  config.pin_href     = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn     = PWDN_GPIO_NUM;
  config.pin_reset    = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.fb_count     = 1;
}

String capturePhoto(uint8_t* _jpg, size_t* _len) {
  camera_fb_t*  frame = NULL;
  String b64enc = String();

  Serial.print("Taking a picture..");
  frame = esp_camera_fb_get();
  if (!frame)
  {
    Serial.println("Got NO snap!");
  } else {
    Serial.println("Done! Got image of size ");
    Serial.print(frame->len);
    Serial.println(" bytes.");
    
    if (frame->format != PIXFORMAT_JPEG) {
      Serial.print("Compressing..");
      bool success = frame2jpg(frame, 80, &_jpg, _len);
      if (success) {
        Serial.print("Done! Compressed to JPEG of size ");
        Serial.print(*_len);
        Serial.print(" bytes.");
      } else {
        Serial.println("Produced NO JPEG!");
      }
    } else {
      _jpg    = frame->buf;
      *_len   = frame->len;
      Serial.println("READY TO PUBLISH IMAGE!");
    } // end IF JPEG `.. format is JPEG
    
    b64enc = base64::encode(_jpg, *_len);
    esp_camera_fb_return(frame);
    frame = NULL;
  }   // end IF frame .. picture taken
  return b64enc;
}