// ESP32 16x6 keyboard matrix + SK6812mini-e backlight
// Columns: MCP23017 @ 0x21 (A2=0, A1=0, A0=1) — all 16 pins used
// Rows:    MCP23017 @ 0x20 (A2=0, A1=0, A0=0) — GPA0..GPA5 used
// LEDs:    SK6812mini-e chain on GPIO 3 (~96 LEDs)

#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>

constexpr uint8_t SDA_PIN = 0;    // IO0 (strapping; keep pulled up at boot)
constexpr uint8_t SCL_PIN = 1;    // IO1
constexpr uint32_t I2C_FREQ = 400000;

constexpr uint8_t COL_ADDR = 0x21;  // columns expander address
constexpr uint8_t ROW_ADDR = 0x20;  // rows expander address

constexpr uint8_t ROW_COUNT = 6;
constexpr uint8_t COL_COUNT = 16;

constexpr uint8_t LED_PIN = 3;       // GPIO3
constexpr uint16_t LED_COUNT = 96;
constexpr uint8_t LED_BRIGHTNESS = 32;

Adafruit_MCP23X17 mcpCols;
Adafruit_MCP23X17 mcpRows;
Adafruit_NeoPixel pixels(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

bool keyState[ROW_COUNT][COL_COUNT];

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN, I2C_FREQ);

  if (!mcpCols.begin_I2C(COL_ADDR)) {
    Serial.println("Cols MCP init failed");
    while (true) {}
  }
  if (!mcpRows.begin_I2C(ROW_ADDR)) {
    Serial.println("Rows MCP init failed");
    while (true) {}
  }

  // Columns as outputs, idle HIGH
  for (uint8_t c = 0; c < COL_COUNT; c++) {
    mcpCols.pinMode(c, OUTPUT);
    mcpCols.digitalWrite(c, HIGH);
  }

  // Rows as inputs with pull-ups (GPA0..GPA5)
  for (uint8_t r = 0; r < ROW_COUNT; r++) {
    mcpRows.pinMode(r, INPUT_PULLUP);
  }

  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);
  pixels.clear();
  pixels.show();
}

void scanMatrix() {
  // Clear current state
  for (uint8_t r = 0; r < ROW_COUNT; r++) {
    for (uint8_t c = 0; c < COL_COUNT; c++) {
      keyState[r][c] = false;
    }
  }

  for (uint8_t c = 0; c < COL_COUNT; c++) {
    // Drive one column low, others high
    for (uint8_t cc = 0; cc < COL_COUNT; cc++) {
      mcpCols.digitalWrite(cc, cc == c ? LOW : HIGH);
    }

    delayMicroseconds(5); // allow signals to settle

    for (uint8_t r = 0; r < ROW_COUNT; r++) {
      bool pressed = (mcpRows.digitalRead(r) == LOW);
      keyState[r][c] = pressed;
    }
  }

  // Return columns to idle high
  for (uint8_t c = 0; c < COL_COUNT; c++) {
    mcpCols.digitalWrite(c, HIGH);
  }
}

void showDemoLeds() {
  // Simple moving white pixel to verify LED chain
  static uint16_t pos = 0;
  pixels.clear();
  pixels.setPixelColor(pos % LED_COUNT, pixels.Color(0, 0, 0, 255));
  pixels.show();
  pos++;
}

void loop() {
  scanMatrix();

  // Example: print pressed keys
  for (uint8_t r = 0; r < ROW_COUNT; r++) {
    for (uint8_t c = 0; c < COL_COUNT; c++) {
      if (keyState[r][c]) {
        Serial.print("Key r");
        Serial.print(r);
        Serial.print(" c");
        Serial.print(c);
        Serial.println(" pressed");
      }
    }
  }

  showDemoLeds();
  delay(5); // tune scan cadence
}
