
#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <string>
#include "utilsconfig.h"

int main(int argc, char* argv[])
{
	std::string path(BELPHEGOR_SOCKET);
	if(getenv("BELPHEGOR_SOCKET")) {
		path = getenv("BELPHEGOR_SOCKET");
	}
	struct sockaddr_un name;
	int sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if(sock < 0) {
		std::cerr << argv[0] << ": opening socket: " << strerror(errno) << std::endl;
	}
	name.sun_family = AF_UNIX;
	strncpy(name.sun_path, path.c_str(), sizeof(name.sun_path));
	name.sun_path[sizeof(name.sun_path)-1] = 0;
	size_t size = (offsetof(struct sockaddr_un, sun_path) + strlen(name.sun_path));
	if(connect(sock, (struct sockaddr*)& name, sizeof(name)) != 0) {
		std::cerr << argv[0] << ": connecting to belphegor: " << strerror(errno) << std::endl;
	}
	std::string message("SACRIFICE");
	for(size_t i=0; i<argc; ++i) {
		message += std::string("\0", 1) + std::string(argv[i]);
	}
	send(sock, message.c_str(), message.size(), 0);
	const size_t bufsize = 1024;
	char tmpbuf[bufsize];
	std::string answer("");
	int recvsize = 0;
	while((recvsize = recv(sock, tmpbuf, bufsize, 0)) > 0) {
		answer += std::string(tmpbuf, recvsize);
	}
	close(sock);
	std::cout << answer << std::flush;
}
