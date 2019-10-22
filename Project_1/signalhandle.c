#include <signal.h>

void *handle_signal(){
	sigset_t set;
	int signal_type;
	sigemptyset(&set);
  	sigaddset(&set, SIGHUP);
	sigwait(&set,  signal_type);
	if (signal_type == SIGHUP) {
		printf("%s\n","Caught the signal");
	}
}


int main() {
	pthread_t handlerID;
	sigset_t set;
	pthread_create(&handlerID, NULL, handle_signal, NULL);
	// Ignore SIGHUP in main thread
	sigemptyset(&set);
	sigaddset(&set, SIGHUP);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	pthread_detach(handlerID);
	return 0;
}