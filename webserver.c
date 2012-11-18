#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sendfile.h>
#include <cnaiapi.h>

#define OK_MSG "HTTP/1.0 200 OK\r\n\r\n"
#define NOT_FOUND_MSG "HTTP/1.0 404 Not Found\r\n\r\nNot Found"

void handle_client(int c) {
	int read_ret, bytes_read = 0, allocated = 8192;
	char *end, *buf = malloc(allocated);
	while (buf && (read_ret = read(c, buf + bytes_read, allocated - bytes_read)) > 0) {
		bytes_read += read_ret;
		if ((end = strstr(buf, "\r\n\r\n")) != NULL) { break; }
		if (bytes_read >= allocated) {
			allocated *= 2;
			buf = realloc(buf, allocated);
		}
	}
	if (read_ret > 0 && buf) {
		end = end ? end : buf + bytes_read;
		end[0] = '\0'; 
		char* line = strtok(buf, "\r");
		if (line) {
			char* path = strtok(line, " ") ? strtok(NULL, " ") : NULL;
			int fd;
			struct stat s;
			if (path && stat(++path, &s) == 0 && S_ISREG(s.st_mode) && (fd = open(path, O_RDONLY)) != -1) {
				if (write(c, OK_MSG, sizeof(OK_MSG)-1) == sizeof(OK_MSG)-1) {
						sendfile(c, fd, NULL, s.st_size);
				}
				close(fd);
			} else {
				write(c, NOT_FOUND_MSG, sizeof(NOT_FOUND_MSG)-1);
			}
		}
		free(buf);
	}
	end_contact(c);
}

int main(int argc, char* argv[]) {
	if (argc < 2) { return EXIT_FAILURE; }
	while (42) {
		int c = await_contact(atoi(argv[1]));
		if (c == -1) continue;
		int child = fork();
		switch(child) {
			case 0:
				handle_client(c);
				/* fallthrough */
			case -1:
			default:
				end_contact(c);
				break;
		}
	}
	return EXIT_SUCCESS;
}

