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

#include "TCPClient.h"

#define PORT 10088 

//int id = -1;
int running = 1;
TCPClient client;
pthread_t pt_id;

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
	int id = *(int*)args;

	Window root = 0;
	Window window = 0;
	Display *display = XOpenDisplay(0);

	int x = 0, y = 0;
	int dummy_int = 0;
	unsigned int dummy_uint = 0;

	char res[64];
	uint8_t buf[3];

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

			sprintf(res, "{\"id\":%d, \"type\":0x%02x, \"rx\":%d, \"ry\":%d, \"ax\":%d, \"ay\":%d}\r\n", id, buf[0], buf[1], buf[2], x, y); 
			printf("%s", res);

			client.Send((uint8_t*)res, strlen(res));
			usleep(30 * 1000);
		}
		//usleep(30 * 1000);
	}
}


int main(int argc, char *argv[])
{
	setup_sigaction();
	if(argc != 3)
	{
		perror("args error ...\n");
		printf("0:cleint index\n1:server ip address\n");

		client.Close();
		exit(1);
	}
	
	int id = atoi(argv[1]);
	char *addr = (char*)argv[2];

	for(int i = 0; i < argc; i ++)
	{
		printf("argv:%d %s\n", i, argv[i]);
	}
	
	uint32_t wait_count = 0;
	client.Connect(addr, PORT);

    int ret	= pthread_create(&pt_id, NULL, GetAbsPosition, &id);
	if(ret != 0)
	{
		printf("get abs position pthread start failed ... \n");
		exit(1);
	}

	while(running)
	{
		if(wait_count == 3)
		{
			wait_count = 0;
			client.Connect(addr, PORT);
		}

		sleep(1);
		if(!client.Connected())
			wait_count ++;
		else
			wait_count = 0;
	}

	client.Close();
	pthread_cancel(pt_id);
	
	return 0;
}

