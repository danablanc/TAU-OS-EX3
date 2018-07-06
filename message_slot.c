/*
 * message_slot.c
 *
 *  Created on: May 4, 2018
 *      Author: gasha
 */

// ref: the code is based on the code given on rec 06
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE


#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/slab.h> /*for kfree*/
#include <linux/types.h> /*for uniptr*/

MODULE_LICENSE("GPL");

//Our custom definitions of IOCTL operations
#include "message_slot.h"


typedef struct {
    int minor_num;
    char** channel_buffers;
    int* buffer_size;
} MessageSlot;

static MessageSlot* msg_slot = NULL;
static int num_of_drivers = 0;

void freeMemory(void) {
    int i = 0;
    int k = 0;
    for (i = 0; i < num_of_drivers; i++) {
        for (k = 0; k < NUM_OF_CHANNELS; k++) {
            kfree(msg_slot[i].channel_buffers[k]);
        }
        kfree(msg_slot[i].channel_buffers);
        kfree(msg_slot[i].buffer_size);
    }
    kfree(msg_slot);
    num_of_drivers = 0;
}

MessageSlot* CreateMessageSlot(int device_id)
{
    int i = 0;
    int j = 0;
    MessageSlot* tmp_msg_slot;
    tmp_msg_slot = NULL;
    if (num_of_drivers == 0) {
        tmp_msg_slot = kmalloc(sizeof(MessageSlot), GFP_KERNEL);
        memset(tmp_msg_slot, 0, sizeof(MessageSlot));
    }
    else {
        tmp_msg_slot = krealloc(msg_slot, sizeof(MessageSlot) * (num_of_drivers + 1), GFP_KERNEL);
    }
    if (!tmp_msg_slot) {
        return NULL;
    }
    tmp_msg_slot[num_of_drivers].minor_num = device_id;
    tmp_msg_slot[num_of_drivers].channel_buffers = kmalloc(sizeof(char*) * NUM_OF_CHANNELS, GFP_KERNEL);
    memset(tmp_msg_slot[num_of_drivers].channel_buffers, 0, sizeof(char*) * NUM_OF_CHANNELS);
    if (!tmp_msg_slot[num_of_drivers].channel_buffers) {
        return NULL;
    }
    for (i = 0; i < NUM_OF_CHANNELS; i++) {
        tmp_msg_slot[num_of_drivers].channel_buffers[i] = kmalloc((BUF_LEN) * sizeof(char), GFP_KERNEL);
        memset(tmp_msg_slot[num_of_drivers].channel_buffers[i], 0, (BUF_LEN) * sizeof(char));

        if (!tmp_msg_slot[num_of_drivers].channel_buffers[i]) {
            for (j = 0; j < i; j++) {
                kfree(tmp_msg_slot[num_of_drivers].channel_buffers[j]);
            }
            kfree(tmp_msg_slot[num_of_drivers].channel_buffers);
            return NULL;
        }
    }
    tmp_msg_slot[num_of_drivers ].buffer_size = kmalloc(NUM_OF_CHANNELS * sizeof(int), GFP_KERNEL);
    memset(tmp_msg_slot[num_of_drivers].buffer_size, 0, NUM_OF_CHANNELS * sizeof(int));
    if (!tmp_msg_slot[num_of_drivers].buffer_size) {
        for (i = 0; i < NUM_OF_CHANNELS; i++) {
            kfree(tmp_msg_slot[num_of_drivers].channel_buffers[i]);
        }
        kfree(tmp_msg_slot[num_of_drivers].channel_buffers);
        return NULL;
    }
    return tmp_msg_slot;
}

//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
    int i = 0;
    bool new_device = true;
    int device_id = iminor(file->f_inode);
    if (msg_slot == NULL) {
        msg_slot = CreateMessageSlot(device_id);
        num_of_drivers = num_of_drivers + 1;
        if (!msg_slot) {
            num_of_drivers = num_of_drivers - 1;
            return -ENOMEM;
        }
    }
    else {
        for (i = 0; i < num_of_drivers; i++) {
            if (msg_slot[i].minor_num == device_id) {
                new_device = false;
            }
        }
        if (new_device == true) {
            msg_slot = CreateMessageSlot(device_id);
            num_of_drivers = num_of_drivers + 1;
            if (!msg_slot) {
                num_of_drivers = num_of_drivers - 1;
                return -ENOMEM;
            }
        }
    }
    file->private_data = (void*)NUM_OF_CHANNELS;
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{
    int i = 0;
    int j = 0;
    int device_id = iminor(file->f_inode);
    int msg_slot_num = -1;
    int counter = 0;
    int channel_id = NUM_OF_CHANNELS;
    for (i = 0; i < num_of_drivers; i++) {
        if (msg_slot[i].minor_num == device_id)
            msg_slot_num = i;
    }
    if (msg_slot_num == -1)
        return -EINVAL;
    channel_id = (int) (uintptr_t)file->private_data;
    if (channel_id == NUM_OF_CHANNELS)
        return -EINVAL;
    if (msg_slot[msg_slot_num].buffer_size[channel_id] == 0)
        return -EWOULDBLOCK;
    if (length < msg_slot[msg_slot_num].buffer_size[channel_id])
        return -ENOSPC;
    for (j = 0; j < msg_slot[msg_slot_num].buffer_size[channel_id]; j++) {
        if (put_user(msg_slot[msg_slot_num].channel_buffers[channel_id][j], &buffer[j]) != 0) {
            return -EFAULT;
        }
        counter++;
    }
    return counter;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{
    int i = 0;
    int j = 0;
    int msg_slot_num = -1;
    int counter = 0;
    int channel_id = NUM_OF_CHANNELS;
    if (length > BUF_LEN || length <= 0)
        return -EINVAL;
    for (i = 0; i < num_of_drivers; i++) {
        if (msg_slot[i].minor_num == iminor(file->f_inode))
            msg_slot_num = i;
    }
    if (msg_slot_num == -1)
        return -EINVAL;
    channel_id = (int) (uintptr_t)file->private_data;
    for (j = 0; j < length; j++) {
        if (get_user(msg_slot[msg_slot_num].channel_buffers[channel_id][j], &buffer[j]) != 0)
            return -EINVAL;
        counter++;
    }
    msg_slot[msg_slot_num].buffer_size[channel_id] = counter;
    return counter;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
    // Switch according to the ioctl called
    if (( MSG_SLOT_CHANNEL == ioctl_command_id ) && (ioctl_param >= 0 ) && (ioctl_param <= NUM_OF_CHANNELS-1))
    {
        file->private_data = (void*)ioctl_param;
        return SUCCESS;
    }
    return -EINVAL;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
    .read           = device_read,
    .write          = device_write,
    .open           = device_open,
    .unlocked_ioctl = device_ioctl,
    .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
    int rc = -1;

    // Register driver capabilities. Obtain major num
    rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

    // Negative values signify an error
    if ( rc < 0 )
    {
        printk( KERN_ALERT "%s registraion failed for  %d\n",
                DEVICE_FILE_NAME, MAJOR_NUM );
        return rc;
    }
    else {
        printk(KERN_INFO "message_slot: registered major number %d\n", MAJOR_NUM);
    }

    return 0;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
    // Unregister the device
    // Should always succeed
    freeMemory();
    unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
