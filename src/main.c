#include <pthread.h>
#include <unistd.h>

#include "config.h"

#include "grab.h"
#include "configfile.h"

int
main(int argc, char **argv)
{
	int ret;
	pthread_t grab_tid;
	
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

	config_load_modules();
	
	pthread_create(&grab_tid, NULL, grab_thread, NULL);

	config_start_threads();

	/* TODO: find something to do, or thread_exit */
	for (;;)
		pause();
	
	return 0;
}

