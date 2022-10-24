#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include <base64.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>

#define deBuG false

#define CAMERA_MODEL_WROVER_KIT
#include "camera_pins.h"

#include "config.h"
const char* _SSID       = SSID;
const char* _PWD        = PWD;

// const char* _MQBroker = ""; // replace with your broker url
// const char* _MQUID    = "";
// const char* _MQPWD    = "";
// const int   _MQPort   = 0;
// const char* _MQTopic  = "";
// const bool  _MQTLS    = true;

#if defined(HIVEMQ)
  WiFiClientSecure HTTPClient;
  PubSubClient MQTTClient(HTTPClient);

  const char* _MQBroker = HIVEMQ_BROKER; // replace with your broker url
  const char* _MQUID    = HIVEMQ_UID;
  const char* _MQPWD    = HIVEMQ_PWD;
  const int   _MQPort   = 8883;
  const char* _MQTopic  = "bluebird";
#else
  WiFiClient HTTPClient;
  PubSubClient MQTTClient(HTTPClient);

  const char* _MQBroker = MAC_BROKER;
  const char* _MQUID    = MAC_UID;
  const char* _MQPWD    = MAC_PWD;
  const int   _MQPort   = 1883;
  const char* _MQTopic  = "test";
#endif

camera_config_t config;

void startCameraServer();
void config_init();

unsigned long tic = millis();
unsigned long toc = tic;
unsigned long delta = 5000;

uint8_t*  _jpg_buf = NULL;
size_t    _jpg_len = 0;

void onmessage(char*, byte*, unsigned int);
void publish_me(String);
void reconnect();
String capturePhoto(uint8_t*, size_t*);
static const char *root_ca PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  config_init();
  config.frame_size = FRAMESIZE_QVGA; // FRAMESIZE_240X240; 
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

  // WiFi
  WiFi.begin(SSID, PWD);
  Serial.print("Making WiFi connection..");
  while (WiFi.isConnected() != true) {
    Serial.print(" .");
    delay(500);
  }
  Serial.println("Done!");

  // MQTT
  // HTTPClient.setCACert(root_ca);   // Use with WiFiClientSecure (HIveMQ), 
                                      // takes too long to publish image!
  MQTTClient.setServer(_MQBroker, _MQPort); 
  MQTTClient.setCallback(onmessage);    

  if (deBuG) {
    startCameraServer();
    Serial.print("Use http://");
    Serial.print(WiFi.localIP());
    Serial.println(" to view!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  toc = millis();
  if (toc - tic >= delta) {
    tic = toc;

    if (!deBuG) {
      // Grab a snap
      String b64enc = String();
      b64enc = capturePhoto(_jpg_buf, &_jpg_len);

      // TODO [Alt.]: Ship to MQTT broker - MacBook

      // Ship to MQTT broker - HiveMQ
      Serial.print("Connect to MQTT broker..");
      if (!MQTTClient.connected()) {
        Serial.println("Failed! Made NO MQTT connection.");
        reconnect();
      } else {
        Serial.println("Done!");
        publish_me(b64enc); // Publisher action
      }
      MQTTClient.loop(); // Callbacks handled in event loop

      // TODO: Send Telegram notification
    } // end IF deBuG
    // TODO: Check motion-sensor signal
  }   // end TIC-TOC
}     // end LOOP

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
    Serial.println("Failed! Got NO snap!");
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
        Serial.println("Failed! Produced NO JPEG!");
      }
    } else {
      _jpg    = frame->buf;
      *_len   = frame->len;
      Serial.println("READY TO PUBLISH IMAGE!!");
    } // end IF JPEG `.. format is JPEG
    
    b64enc = base64::encode(_jpg, *_len);
    esp_camera_fb_return(frame);
    frame = NULL;
  }   // end IF frame .. picture taken
  return b64enc;
}

void onmessage(char* topic, byte* payload, unsigned int length) {
  /*
  Handle a new message published to the subscribed topic on the 
  MQTT broker and show in the OLED display.
  This is the heart of the subscriber, making it so the nodemcu
  can act upon information, say, to operate a solenoid valve when
  the mositure sensor indicates dry soil.
  */
  Serial.print("Got a message in topic ");
  Serial.println(topic);
  Serial.print("Received data: ");
  char message2display[length];
  for (unsigned int i = 0; i < length; i++) {
    Serial.print(char(payload[i]));
    message2display[i] = payload[i];
  }
  Serial.println();
}

void publish_me(String message) {
  Serial.print("Publishing image..");
  boolean pstatus;
  pstatus = MQTTClient.publish_P(_MQTopic, (const uint8_t*)message.c_str(), message.length(), true);
  //pstatus = MQTTClient.publish(_HiveTopic, message.c_str());
  Serial.println(String(pstatus ? "Published!" : " Failed! Sent no image."));
}

void reconnect() {
  while (!MQTTClient.connected()) {   // Until connected..
    Serial.print("Connecting to MQTT broker.. ");
    String clientID = "BIRDWORD";
    if (MQTTClient.connect(clientID.c_str(), _MQUID, _MQPWD)) { // Present credentials
      Serial.println("Connected!");
      MQTTClient.publish(_MQTopic, "BIRD IS THE WORD");           // Announce status
      MQTTClient.subscribe(_MQTopic);                             // Resubscribe
    } else {
      Serial.print("Failed! Made no connection with rc = ");
      Serial.print(MQTTClient.state());
      Serial.println("! Try again in 5 seconds!");                   // Retry after delay
      delay(5000);
    }
  }
}