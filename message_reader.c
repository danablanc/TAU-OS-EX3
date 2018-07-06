/*
 * message_reader.c
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
	if (argc != 3) {
		printf("Error in number of given args!\n");
		return ERROR;
	}
	int target_channel_id = atoi(argv[2]);
	int fd=open(argv[1],O_RDONLY);
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
    char buffer[BUF_LEN+1];
    int read_chars_num = read(fd, buffer, BUF_LEN);
    if (read_chars_num <0){
        printf("Error in read. %s\n", strerror(errno));
        close(fd);
        return ERROR;
    }
    buffer[read_chars_num]='\0';
    printf("Read %d\n", read_chars_num);
    printf("Buffer is: %s\n", buffer);
      //  exit(1);
    printf("%d bytes read to device\n", read_chars_num);
        close(fd);
    return SUCCESS;
}
