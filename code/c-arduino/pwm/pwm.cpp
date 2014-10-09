// Arduino hooks
#include <Arduino.h>
#include <stdio.h>
#include <trace.h>
#include <interrupt.h>
#include <sys/stat.h>

#define PLATFORM_NAME_PATH "/sys/devices/platform/"
#define MY_TRACE_PREFIX __FILE__

void setup(void);
void loop(void);

int main(int argc, char * argv[])
{
	char *platform_path = NULL;
	struct stat s;
	int err;

	// make ttyprintk at some point
	stderr = freopen("/tmp/log_er.txt", "w", stderr);
	if (stderr == NULL){
	    printf("Unable to remap stderr !\r\n");
	    exit(-1);
	}
	fflush(stderr);

	// Snapshot time counter
	if (timeInit() < 0)
		exit(-1);

	// debug for the user
	if (argc < 2){
		fprintf(stderr, "./sketch tty0\r\n");
		return -1;
	}
	printf("started with binary=%s Serial=%s\r\n", argv[0], argv[1]);
	fflush(stdout);

	// check if we're running on the correct platform
	// and refuse to run if no match

	platform_path = (char *)malloc(sizeof(PLATFORM_NAME_PATH) + sizeof(PLATFORM_NAME));
	sprintf(platform_path,"%s%s", PLATFORM_NAME_PATH, PLATFORM_NAME);

	printf("checking platform_path [%s]\r\n", platform_path);
	fflush(stdout);

	err = stat(platform_path, &s);

	if(err != 0) {
		fprintf(stderr, "stat failed checking for %s with error code %d\r\n", PLATFORM_NAME, err);
		free(platform_path);
		return -1;
	}
	if(!S_ISDIR(s.st_mode)) {
		/* exists but is no dir */
		fprintf(stderr, "Target board not a %s\r\n", PLATFORM_NAME);
		free(platform_path);
		return -1;
	}

	printf("Running on a %s platform (%s)\r\n", PLATFORM_NAME, platform_path);
	fflush(stdout);

	free(platform_path);

	// TODO: derive trace level and optional IP from command line
	trace_init(VARIANT_TRACE_LEVEL, 0);
	trace_target_enable(TRACE_TARGET_UART);

	// Do not call Arduino init
	// init(argc, argv);

	// Init IRQ layer
	// Called after init() to ensure I/O permissions inherited by pthread
	interrupt_init();

#if defined(USBCON)
	USBDevice.attach();
#endif

	setup();
	for (;;) {
		loop();
	}
	return 0;
}

// A dirty-work buffer for writing to files
char buffer[16] = "";

// The file descriptor for the duty cycle sysfs hook
static FILE * fd_duty_cycle = NULL;

void setup(void) {
	printf("Setup\r\n");
	fflush(stdout);

	FILE * fd_export = fopen("/sys/class/pwm/pwmchip0/export", "ab");
	if (fd_export == NULL) {
		printf("Null export!\r\n"); return;
	}
	rewind(fd_export);
	snprintf(buffer, sizeof(buffer), "%u", 3);
	fwrite(buffer, sizeof(char), sizeof(buffer), fd_export);
	fflush(fd_export);
	fclose(fd_export);

	FILE * fd_enable = fopen("/sys/class/pwm/pwmchip0/pwm3/enable", "ab");
	if (fd_enable == NULL) {
		printf("Null enable!\r\n"); return;
	}
	rewind(fd_enable);
	snprintf(buffer, sizeof(buffer), "%u", 3);
	fwrite(buffer, sizeof(char), sizeof(buffer), fd_enable);
	fflush(fd_enable);
	fclose(fd_enable);

	FILE * fd_period = fopen("/sys/class/pwm/pwmchip0/pwm3/period", "ab");
	if (fd_period == NULL) {
		printf("Null period!\r\n"); return;
	}
	rewind(fd_period);
	snprintf(buffer, sizeof(buffer), "%u", 2000000);
	fwrite(buffer, sizeof(char), sizeof(buffer), fd_period);
	fflush(fd_period);
	fclose(fd_period);

	fd_duty_cycle = fopen("/sys/class/pwm/pwmchip0/pwm3/duty_cycle", "ab");
	if (fd_duty_cycle == NULL) {
		printf("Null duty_cycle!\r\n"); return;
	}
	rewind(fd_duty_cycle);
	snprintf(buffer, sizeof(buffer), "%u", 500000);
	fwrite(buffer, sizeof(char), sizeof(buffer), fd_duty_cycle);
	fflush(fd_duty_cycle);
	fclose(fd_duty_cycle);

	printf("Waiting\r\n");
	fflush(stdout);
	sleep(5);
	printf("Done\r\n");
	fflush(stdout);
}

void loop(void) {

}
