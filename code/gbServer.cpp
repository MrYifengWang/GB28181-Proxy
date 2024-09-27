#include "App.h"
#include <sys/time.h>
#include <sys/resource.h>


void print_usage() {

}

bool command_line(int argc, char* argv[]) {
	if ((argc % 2) == 0)
		return false;

	for (int i = 1; i < argc; i += 2) {
		if (strlen(argv[i]) != 2)
			return false;

		switch (argv[i][1]) {

		case 't':
			break;

		default:
			return false;
		}
	}

	return true;
}



int main(int argc, char* argv[]) {

	{
		struct sigaction act;
		memset(&act, 0, sizeof(act));
		sigemptyset(&act.sa_mask);
		act.sa_handler = SIG_IGN;
		if (sigaction(SIGPIPE, &act, NULL) < 0) {

		}
	}
	{
		struct rlimit rlim;
		getrlimit(RLIMIT_CORE, &rlim);
		const int core_limit = rlim.rlim_max;
		if (rlim.rlim_cur < core_limit) {
			rlim.rlim_cur = core_limit;
			setrlimit(RLIMIT_CORE, &rlim);
		}
	}



	if (!command_line(argc, argv)) {
		print_usage();
		exit(0);
	}

	theApp.start();

	return 0;
}
