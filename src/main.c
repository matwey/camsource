#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

#include "main.h"
#include "grab.h"
#include "configfile.h"
#include "mod_handle.h"
#include "camdev.h"
#include "log.h"

static void main_init(char *);

int
main(int argc, char **argv)
{
	if (argc >= 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") || !strcmp(argv[1], "-?")))
	{
		printf("Usage:\n");
		printf("  %s [configfile]\n", argv[0]);
		printf("       - Starts camsource, optionally with a certain config file\n");
		printf("  %s -c [device]\n", argv[0]);
		printf("       - Dumps the video capabilites info for given device\n");
		printf("  %s -h\n", argv[0]);
		printf("       - Shows this text\n");
		exit(0);
	}
	
	if (argc >= 2 && !strcmp(argv[1], "-c"))
	{
		camdev_capdump(argv[2]);
		exit(0);
	}
	
	main_init(argv[1]);
	
	/* nothing to do, so exit */
	pthread_exit(NULL);
	
	return 0;
}

static
void
main_init(char *config)
{
	int ret;
	pthread_t grab_tid;
	pthread_attr_t attr;
	int logfd;
	struct camdev *camdev;
	
	signal(SIGPIPE, SIG_IGN);
	
	printf("Camsource " VERSION " starting up...\n");
	fflush(stdout);
	
	mod_init();
	
	ret = config_init(config);
	if (ret)
	{
		printf("No config file found, exit.\n");
		printf("If you've just installed or compiled, check out \"camsource.conf.example\",\n");
		printf("either located in " SYSCONFDIR " or in the source tree.\n");
		exit(1);
	}
	
	ret = config_load();
	if (ret)
	{
		printf("Failed to load config file %s, exit.\n", ourconfig);
		exit(1);
	}
	
	logfd = log_open();
	
	grab_thread_init();

	mod_load_all();
	
	camdev = grab_open();
	if (!camdev)
		exit(1);
	
	if (logfd >= 0)
		log_replace_bg(logfd);
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&grab_tid, &attr, grab_thread, camdev);
	pthread_attr_destroy(&attr);

	mod_start_all();
}

