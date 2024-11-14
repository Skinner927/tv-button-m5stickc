#include <HTTPClient.h>
#include <M5StickC.h>
#include <Ticker.h>
#include <WString.h>
#include <WiFi.h>
#include <stdbool.h>
#include <stdint.h>

#include "secrets.h"

// Toggle on/off for printing
#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define debug(msg) Serial.print(msg)
#define debugf(fmt, ...) Serial.printf(fmt, __VA_ARGS__)
#else
#define debug(msg) ((void)msg)
#define debugf(...)
#endif  // DEBUG_SERIAL

// Utils
// Use empty string if str is NULL
#define SAFE_STR(str) ((str) ? (str) : "")

// Pins
#define PIN_RED_LED 10

// LCD Screen
#define SCREEN_BG BLACK                  // Background color of the LCD
#define SCREEN_FG WHITE                  // Text color of the LCD
#define NO_COLOR ((SCREEN_BG) + 0x0001)  // Sentinel
#define SCREEN_MARGIN_TOP 3
#define SCREEN_MARGIN_LEFT 3
#define TEXT_MARGIN_BTM 3
#define STATUS_MARGIN_RIGHT 2

// StatusItem stores messages to print to the screen
typedef struct {
  const char* label;
  const char* msg;
  uint32_t color;
  uint32_t status;        // status icon color
  bool blink;             // True if the status icon should blink
} StatusItem;
#define STATUS_GOOD GREEN
#define STATUS_INFO BLUE
#define STATUS_WARN YELLOW
#define STATUS_ERROR RED

StatusItem g_wifi_status = {.label = "WiFi: ",
                            .msg = "Connecting...",
                            .color = SCREEN_FG,
                            .status = STATUS_INFO,
                            .blink = true};
StatusItem g_btn_status = {
    .label = "Btn: ", .msg = NULL, .color = SCREEN_FG, .status = STATUS_ERROR};

StatusItem* g_all_status_items[] = {&g_wifi_status, &g_btn_status, NULL};

// Pixel height of 1 line of text including bottom margin.
// Sane default, but updated in setup() with real value.
int32_t g_lcd_line_height = 10;
int32_t g_lcd_line_height_no_padding = 8;

Ticker g_status_blink_ticker;
bool g_status_blink_state = true;

void statusBlinkTickerCallback(void) {
  g_status_blink_state = !g_status_blink_state;
}

void setup(void) {
  M5.begin();
  M5.Lcd.fillScreen(YELLOW);  // Indicator that we're in setup
#ifdef DEBUG_SERIAL
  Serial.begin(115200);
  Serial.flush();
  delay(50);
  Serial.print("OK\n");
#endif

  // 3 = screen is sideways with charger on left
  // 1 = screen is sideways with charger on right
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextWrap(false);
  // Figure out line height
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("\n");
  g_lcd_line_height_no_padding = M5.Lcd.getCursorY();
  g_lcd_line_height = g_lcd_line_height_no_padding + TEXT_MARGIN_BTM;
  M5.Lcd.setCursor(0, 0);
  debugf("g_lcd_line_height = %d\n", g_lcd_line_height);

  // Start tickers
  g_status_blink_ticker.attach_ms(350, statusBlinkTickerCallback);

  // Setup LED
  pinMode(PIN_RED_LED, OUTPUT);
  digitalWrite(PIN_RED_LED, 1);  // We have to sink LED to turn it on

  // Start WiFi
  M5.Lcd.fillScreen(SCREEN_BG);
  M5.Lcd.setTextColor(SCREEN_FG);
  // M5.Lcd.setCursor(MARGIN_X, SCREEN_Y / 2);
  // M5.Lcd.printf("Connecting to WiFi...");
  //  WiFi.begin(CONFIG_SSID, CONFIG_PASS, 0, NULL, true);
  //  WiFi.setAutoConnect(true);
}

void works_drawStatusItems() {
  char buffer[255] = {0};
  StatusItem* current = g_all_status_items[0];
  for (int i = 0; NULL != current; current = g_all_status_items[++i]) {
    debug("!_! ");
    if (NULL == current) {
      debugf("i %d NULL\n", i);
      break;
    }
    debugf("i %d  current: %p \n", i, current);
    if (NULL == current->label) {
      current->label = "";
    }
    if (NULL == current->msg) {
      current->msg = "";
    }
    debugf("lbl %s\n", current->label);
    debugf("msg %s\n", current->msg);
    snprintf(buffer, 255, "%s%s", current->label ? current->label : "",
             current->msg ? current->msg : "");
    debug("snprintf\n");
    M5.Lcd.drawString(buffer, 0, i * g_lcd_line_height);
    debug("drawString\n");
  }
}

void drawStatusItems() {
  char buffer[255] = {0};
  int32_t status_width  = g_lcd_line_height_no_padding;
  int32_t status_height = g_lcd_line_height_no_padding;

  // Loop over every StatusItem
  StatusItem** current = g_all_status_items;
  uint32_t line_y = SCREEN_MARGIN_TOP;
  uint32_t line_x = SCREEN_MARGIN_LEFT;
  for (; NULL != *current; line_y += g_lcd_line_height, current++) {
    // Clear this line
    M5.Lcd.fillRect(line_x, line_y, M5.Lcd.width(), g_lcd_line_height_no_padding, SCREEN_BG);

    int32_t text_start_x = line_x;
    if (NO_COLOR != (*current)->status) {
      text_start_x += status_width + STATUS_MARGIN_RIGHT;
      // Draw status icon if blink disabled or last blink was false
      uint32_t color = (!(*current)->blink || g_status_blink_state) ? (*current)->status : SCREEN_BG;
      M5.Lcd.fillRect(line_x, line_y, status_width, status_height, color);
    }

    // Write text
    snprintf(buffer, 255, "%s%s", SAFE_STR((*current)->label),
             SAFE_STR((*current)->msg));
    M5.Lcd.setTextColor((*current)->color);
    M5.Lcd.drawString(buffer, text_start_x, line_y, 1);
    M5.Lcd.setTextColor(SCREEN_FG);
  }
  // StatusItem** current = g_all_status_items;
  // uint32_t y_offset = 0;
  // for (int i = 0; NULL != *current; current++, i++) {
  //   char buffer[255] = {0};
  //   snprintf(buffer, 255, "%s%s", (*current)->label || "", (*current)->msg);
  //   M5.Lcd.drawString(buffer, 0, y_offset, 1);
  //   // if (NO_COLOR != (*current)->status) {

  //   // }
  // }
}

void loop() {
  drawStatusItems();
  delay(50);
}
