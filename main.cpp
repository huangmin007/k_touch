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


int running = 1;
TCPServer server;


void ctrl_c_handler(int signum)
{
	printf("signum:%d\n", signum);
	if(signum == SIGPIPE)	return;

	running = 0;
}
void setup_sigaction()
{
	struct sigaction sa;
	sa.sa_handler = ctrl_c_handler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);

	//signal(SIGPIPE, SIG_IGN);
}

static void *GetAbsPosition(void *args)
{
	Window root = 0;
	Window window = 0;
	Display *display = XOpenDisplay(0);

	int x = 0, y = 0;
	int dummy_int = 0;
	unsigned int dummy_uint = 0;

	char res[32];
	uint8_t buf[30];

	int fd = open("/dev/input/mouse0", O_RDONLY);
	if(fd < 0)
	{
		printf("failed to open \"/dev/input\"\n");
		exit(1);
	}

	while(1)
	{
		if(read(fd, buf, sizeof(buf)) > 0)
		{
			int ret = XQueryPointer(display, XDefaultRootWindow(display),
				&root, &window, &x, &y, &dummy_int, &dummy_int, &dummy_uint);
			if(ret <= 0)	continue;

			sprintf(res, "{\"type\":0x%02x, \"rx\":%d, \"ry\":%d, \"ax\":%d, \"ay\":%d}\r\n", buf[0], buf[1], buf[2], x, y); 
			printf("%s", res);

			server.Send((uint8_t*)res, strlen(res));
			usleep(30 * 1000);
		}
		//usleep(30 * 1000);
	}
}


int main(int argc, char *argv[])
{
	setup_sigaction();
	printf("start ... \n");

	server.Bind(5000);
	server.Start();

	pthread_t pt_id;
    int ret	= pthread_create(&pt_id, NULL, GetAbsPosition, NULL);
	if(ret != 0)
	{
		printf("get abs position pthread start failed ... \n");
		exit(1);
	}

	while(running)
	{
		sleep(1);
	}

	server.Stop();
	pthread_cancel(pt_id);
	
	return 0;
}

