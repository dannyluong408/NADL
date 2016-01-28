#include <signal.h>
#include <stdio.h>
#include "server.hpp"

Server server;

void signal_handler(const int signum) {
	puts("\nGot interrupt signal."); // line break
	server.force_shutdown();
}

int main(int argc, char **argv) {
	if (signal(SIGINT, signal_handler) == SIG_ERR) {
		puts("Failed to set signal handled.");
		return -1;
	}
	if (server.init(argc, argv)) {
		puts("Server failed to start");
		return -2;
	}
	server.run();
	return 0;
}