/****************************************88
 *	2019/06/11
 *	gcc -o main test.c -lX11
 *
 *
 */ 


#include <stdio.h>
#include <X11/Xlib.h>
#include <linux/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include "TCPServer.h"

// sigaction handler
int runningi = 1;	//rinning
void ctrl_c_handler(int signum)
{
	runningi = 0;
}
void setup_sigaction()
{
	struct sigaction sa;
	sa.sa_handler = ctrl_c_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
}

TCPServer server;

int main(int argc, char *argv[])
{
	setup_sigaction();
	printf("start ... \n");
	int x = 0, y = 0;

	Display *display = XOpenDisplay(0);
	Window window = 0;
	Window root = 0;

	int dummy_int = 0;
	unsigned int dummy_uint = 0;
	
	uint8_t buf[3];
	int fd = open("/dev/input/mouse0", O_RDONLY);
	
	if(fd < 0)
	{
		printf("failed to open \"/dev/input/\"\n");
		exit(-1);
	}

	server.Bind(5000);
	server.Start();

	char res[32];

	while(runningi)
	{
		if(read(fd, buf, sizeof(buf)) > 0)
		{
			int ret = XQueryPointer(display, XDefaultRootWindow(display), 
				&root, &window, &x, &y, &dummy_int, &dummy_int, &dummy_uint);

			if(ret <= 0) continue;
			//printf("mouse type=0x%02x ax=%d ay=%d\r\n", buf[0], x, y);
			//printf("mouse type=0x%02x relX=%d relY=%d  absX=%d absY=%d\r\n", buf[0], buf[1], buf[2], x, y);
			
			sprintf(res, "{\"type\":0x%02x, \"rx\":%d, \"ry\":%d, \"ax\":%d, \"ay\":%d}\r\n", buf[0], buf[1], buf[2], x, y); 
			printf("%s", res);
			server.Send((uint8_t*)res, strlen(res));
			
		}
		usleep(50 * 1000);
	}

	server.Stop();
	return 0;
}
