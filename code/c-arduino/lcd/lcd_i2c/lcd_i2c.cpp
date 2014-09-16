#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define I2C_ADDR 0x27
#define BACKLIGHT_PIN 3
#define EN_PIN 2
#define RW_PIN 1
#define RS_PIN 0
#define D4_PIN 4
#define D5_PIN 5
#define D6_PIN 6
#define D7_PIN 7

LiquidCrystal_I2C lcd(I2C_ADDR, EN_PIN, RW_PIN, RS_PIN, D4_PIN, D5_PIN, D6_PIN, D7_PIN);

void setup(void) {
  lcd.begin(16, 2);
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE);
  lcd.setBacklight(HIGH);  
  lcd.home();
  lcd.print("Arqui LCD");  
}

static int n = 1;

void loop(void) {
  lcd.setCursor(0, 1);
  lcd.print(n++, DEC);
  lcd.setBacklight(LOW);
  delay(1000);
  lcd.setBacklight(HIGH);
  delay(1000);
}

int main(void) {
	setup();
	for (;;) {
		loop();
	}
	return 0;
}
