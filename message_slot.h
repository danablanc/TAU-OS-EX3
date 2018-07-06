/*
 * message_slot.h
 *
 *  Created on: May 11, 2018
 *      Author: gasha
 */

#ifndef MESSAGE_SLOT_H_
#define MESSAGE_SLOT_H_

#include <linux/ioctl.h>

#define MAJOR_NUM 244

// Set the message of the device driver
#define MSG_SLOT_CHANNEL _IOW(MAJOR_NUM, 0, unsigned long)

#define DEVICE_RANGE_NAME "message_slot"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "message_slot"
#define SUCCESS 0
#define ERROR -1
#define NUM_OF_CHANNELS 4



#endif /* MESSAGE_SLOT_H_ */
