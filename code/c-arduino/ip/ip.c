#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

void setup(void);
void loop(void);

int main(int argc, char * argv[])
{
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

