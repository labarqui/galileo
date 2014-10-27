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

#if defined(USBCON)
	USBDevice.attach();
#endif

	setup();
	for (;;) {
		loop();
	}
	return 0;
}

void setup(void) {
}

void loop(void) {
	while (1) {
		system("ifconfig eth0 | grep 'inet addr' | sed -e 's/$/\r/' > /dev/ttyGS0"); 
		sleep(1);
	}
}
