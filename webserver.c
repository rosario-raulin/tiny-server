#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/sendfile.h>
#include <cnaiapi.h>

void handle_client(int c) {
	int read_ret, bytes_read = 0, allocated = 8192;
	char *end, *buf = malloc(allocated);
	while (buf && (read_ret = read(c, buf + bytes_read, allocated - bytes_read)) >= 0) {
		bytes_read += read_ret;
		if ((end = strstr(buf, "\r\n\r\n")) != NULL) { break; }
		if (bytes_read >= allocated) {
			allocated *= 2;
			buf = realloc(buf, allocated);
		}
	}
	if (read_ret >= 0 && buf) {
		end = end ? end : buf + bytes_read;
		end[0] = '\0'; 
		char* line = strtok(buf, "\r");
		if (line) {
			int fd;
			char* path = strtok(line, " ") ? strtok(NULL, " ") : NULL;
			if (path && (fd = open(++path, O_RDONLY)) > 0) {
				struct stat s;
				if (fstat(fd, &s) == 0) {
					sendfile(c, fd, NULL, s.st_size);
					close(fd);
				}
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
				break;
			case -1:
			default:
				end_contact(c);
				break;
		}
	}
	return EXIT_SUCCESS;
}
