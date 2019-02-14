/**
**
** Copyright (C) Intel 2015
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
*/
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>



int sent_at(int fd, const char *at_cmd, int timeout){
    char buffer[2048];

    struct pollfd pfd[] = {
        { .fd = fd, .events = POLLIN },
    };


    printf("[AT]  sending: %s\n", at_cmd);
    int lenb = snprintf(buffer, sizeof(buffer), "%s\r\n", at_cmd);

    ssize_t len = write(fd, buffer, lenb);
    if (len != lenb) {
        printf("Write failure. %zd/%d written", len, lenb);
        return -1;
    }

    size_t idx = 0;

    int ret = 1;
    while (ret > 0) {
        int err = poll(pfd, sizeof(pfd)/sizeof(pfd[0]), timeout);

        if ((err == 0) || (pfd[0].revents & (POLLERR | POLLHUP | POLLNVAL))) {
            printf("[AT] no answer from modem. timeout: %dms", timeout);
            ret = -1;
            break;
        } else if (pfd[1].revents) {
            printf("[AT] aborted");
            ret = -2;
            break;
        }

        if (pfd[0].revents & POLLIN) {
            int len = read(fd, &buffer[idx], sizeof(buffer) - idx);
            if (len <= 0) {
                printf("failed to read answer. errno: %d/%s", errno, strerror(errno));
                ret = -1;
                break;
            }

            idx += len;
            buffer[idx] = '\0';

            char *cr;
            while ((cr = strstr(buffer, "\r\n")) != NULL) {
                *cr = '\0';
                if (buffer[0] != '\0')
                    printf("[AT] received: %s\n", buffer);

                if (strcmp(buffer, "OK") == 0) {
                    ret = 0;
                    break;
                } else if (strcmp(buffer, "ERROR") == 0) {
                    ret = -1;
                    break;
                }

                cr += 2; // skipping \r\n
                idx = &buffer[idx] - cr;
                memmove(buffer, cr, idx);
            }
        }
    }

    return ret;
}

int sendAT(char *device, char *cmd){
	int fd = open(device, O_RDWR | O_NONBLOCK);
	if (fd >= 0) {
		printf("[AT] device %s open success\n", device);
		int err = sent_at(fd, cmd, 500);
		if (err < 0){
			printf("%s at cmd send failed\n", __FUNCTION__);
			close(fd);
			return -1;
		}
		close(fd);		
	}
	else{
		printf("[AT] device %s open failed\n", device);
	}
	return 0;	
}

/* sample client app */
int main(void){
	
	printf("%s sample CRM client", __FUNCTION__);
	sendAT("/dev/iat", "at");
	sendAT("/dev/iat", "ati");
	return 0;
}
