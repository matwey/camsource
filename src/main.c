#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

#include "main.h"
#include "grab.h"
#include "configfile.h"
#include "mod_handle.h"

int
main(int argc, char **argv)
{
	main_init();
	
	/* nothing to do, so exit */
	pthread_exit(NULL);
	
	return 0;
}

void
main_init()
{
	int ret;
	pthread_t grab_tid;
	pthread_attr_t attr;
	
	signal(SIGPIPE, SIG_IGN);
	
	mod_init();
	
	ret = config_init();
	if (ret)
	{
		printf("No config file found, exit.\n");
		exit(1);
	}
	
	ret = config_load();
	if (ret)
	{
		printf("Failed to load config file %s, exit.\n", ourconfig);
		exit(1);
	}
	
	grab_thread_init();

	mod_load_all();
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&grab_tid, &attr, grab_thread, NULL);
	pthread_attr_destroy(&attr);

	mod_start_all();
}

