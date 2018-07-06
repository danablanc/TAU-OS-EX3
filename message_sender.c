/*
 * message_sender.c
 *
 *  Created on: May 11, 2018
 *      Author: gasha
 */

#include "message_slot.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int main(int argc, char** argv) {
	if (argc != 4) { //do this need to be 3 or 4?
		printf("Error in number of given args!\n");
		return ERROR;
	}
	int target_channel_id = atoi(argv[2]);
	int fd=open(argv[1],O_RDWR);
    if (fd < 0){
        printf("Error Could not open message slot path. %s\n", strerror(errno));
        return ERROR;
    }
    int ioctl_val = ioctl(fd, MSG_SLOT_CHANNEL, target_channel_id);
    if (ioctl_val < 0){
        printf("Error ioctl have failed. %s\n", strerror(errno));
        close(fd);
        return ERROR;
    }
    int written_chars_num = write(fd, argv[3], strlen(argv[3]));
    if (written_chars_num <0){
        printf("Error in write. %s\n", strerror(errno));
        close(fd);
        return ERROR;
    }
    close(fd);
    printf("%d bytes written to device\n", written_chars_num);
    return SUCCESS;
}

