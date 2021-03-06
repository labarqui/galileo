/*
main.cpp userspace main loop for Intel Galileo family boards
Copyright (C) 2014 Intel Corporation

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

 */
// Arduino hooks
#include <Arduino.h>
#include <trace.h>
#include <interrupt.h>
#include <sys/stat.h>

#include <Wire.h>
#define PLATFORM_NAME_PATH "/sys/devices/platform/"
#define MY_TRACE_PREFIX __FILE__

void setup(void);
void loop(void);

int main(int argc, char * argv[])
{
	char *platform_path = NULL;
	struct stat s;
	int err;

	// Install a signal handler

	// make ttyprintk at some point
	stdout = freopen("/tmp/log.txt", "w", stdout);
	if (stdout == NULL){
	    fprintf(stderr, "unable to remap stdout !\n");
	    exit(-1);
	}
	fflush(stdout);

	stderr = freopen("/tmp/log_er.txt", "w", stderr);
	if (stderr == NULL){
	    printf("Unable to remap stderr !\n");
	    exit(-1);
	}
	fflush(stderr);

	// Snapshot time counter
	if (timeInit() < 0)
		exit(-1);

	// debug for the user
	if (argc < 2){
		fprintf(stderr, "./sketch tty0\n");
		return -1;
	}
	printf("started with binary=%s Serial=%s\n", argv[0], argv[1]);
	fflush(stdout);

	// check if we're running on the correct platform
	// and refuse to run if no match

	platform_path = (char *)malloc(sizeof(PLATFORM_NAME_PATH) + sizeof(PLATFORM_NAME));
	sprintf(platform_path,"%s%s", PLATFORM_NAME_PATH, PLATFORM_NAME);

	printf("checking platform_path [%s]\n", platform_path);
	fflush(stdout);

	err = stat(platform_path, &s);

	if(err != 0) {
		fprintf(stderr, "stat failed checking for %s with error code %d\n", PLATFORM_NAME, err);
		free(platform_path);
		return -1;
	}
	if(!S_ISDIR(s.st_mode)) {
		/* exists but is no dir */
		fprintf(stderr, "Target board not a %s\n", PLATFORM_NAME);
		free(platform_path);
		return -1;
	}

	printf("Running on a %s platform (%s)\n", PLATFORM_NAME, platform_path);
	fflush(stdout);

	free(platform_path);

	// TODO: derive trace level and optional IP from command line
	trace_init(VARIANT_TRACE_LEVEL, 0);
	trace_target_enable(TRACE_TARGET_UART);

	// Call Arduino init
	init(argc, argv);

	// Init IRQ layer
	// Called after init() to ensure I/O permissions inherited by pthread
	interrupt_init();

#if defined(USBCON)
	USBDevice.attach();
#endif

	setup();
	for (;;) {
		loop();
		//if (serialEventRun) serialEventRun();
	}
	return 0;
}

// Begin custom application code
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
