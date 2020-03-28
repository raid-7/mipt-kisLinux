#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/sysext.h>

#include "directory.h"

#define __preserve noinline

#define USER_BUF_CAP 1024

MODULE_LICENSE("GPL");

static LIST_HEAD(phone_directory);
static DEFINE_SEMAPHORE(directory_lock);

static const char* device_name = "directory_kmodule";
static int dev_major_num;


struct user_handle {
    struct list_head pending_output_head;
    spinlock_t lock;

    char input_buf[USER_BUF_CAP];
    loff_t input_offset;

    char output_buf[USER_BUF_CAP];
    loff_t output_offset;
};

static struct user_handle* handle_alloc(void) {
    struct user_handle* handle = kzalloc(sizeof(struct user_handle), GFP_KERNEL);
    handle->lock = __SPIN_LOCK_UNLOCKED(handle->lock);
    INIT_LIST_HEAD(&handle->pending_output_head);
    return handle;
}

static __preserve ssize_t process_operation(char* buf, size_t len, struct user_handle *handle) {
    size_t rec_size = sizeof(struct phonedir_record);

    char op = buf[0];
    size_t need_bytes = op == PHONEDIR_ADD ? rec_size : PHONEDIR_NAME_LEN;

    if (need_bytes + 1 < len) {
        return 0;
    }

    if (op == PHONEDIR_ADD) {
        struct phonedir_record *record = (struct phonedir_record*) (buf + 1);
        int retval = phonedir_add_record(&phone_directory, record);
        if (retval) {
            return retval;
        }
    } else if (op == PHONEDIR_DELETE || op == PHONEDIR_FIND) {
        char* surname = buf + 1;
        // ensure null terminated
        // tainting source buffer is ok
        surname[PHONEDIR_NAME_LEN - 1] = 0;

        if (op == PHONEDIR_DELETE) {
            phonedir_del(&phone_directory, surname);
        } else {
            struct list_head* found = phonedir_find(&phone_directory, surname);
            if (!found) {
                return -ENOMEM;
            }

            list_splice_tail(found, &handle->pending_output_head);
            kfree(found);
        }
    }

    return need_bytes + 1;
}

static __preserve int handle_process_input_buffers(struct user_handle* handle) {
    int retval = 0;
    size_t l_index = 0;
    size_t r_index = handle->input_offset;

    printk(KERN_INFO "directory_kmodule: process input buffers: %lu %lu\n", l_index, r_index);

    while (l_index < r_index) {
        ssize_t n_processed = process_operation(handle->input_buf + l_index, r_index - l_index, handle);
        if (n_processed < 0) {
            retval = n_processed;
            goto ret;
        }
        if (n_processed == 0) {
            break;
        }
        l_index += n_processed;
    }

    ret:
        if (l_index < r_index) {
            memmove(handle->input_buf, handle->input_buf + l_index, r_index - l_index);
        }
        handle->input_offset = r_index - l_index;
        return retval;
}

static __preserve int handle_process_output_buffers(struct user_handle* handle) {
    printk(KERN_INFO "directory_kmodule: process output buffers: %d\n", list_empty(&handle->pending_output_head) ? 0 : 1);

    if (!list_empty(&handle->pending_output_head)) {
        struct directory_entry *cursor, *n;
        size_t i = handle->output_offset;
        size_t rec_size = sizeof(struct phonedir_record);
        list_for_each_entry_safe(cursor, n, &handle->pending_output_head, list) {
            if (USER_BUF_CAP - i < rec_size)
                break;

            memcpy(handle->output_buf + i, &cursor->record, rec_size);
            i += rec_size;

            list_del(&cursor->list);
            kfree(cursor);
        }
        handle->output_offset = i;
    }
    return 0;
}

static __preserve int handle_process_buffers(struct user_handle* handle) {
    if (down_interruptible(&directory_lock)) {
        return -EINTR;
    }

    int ret_in = handle_process_input_buffers(handle);
    int ret_out = handle_process_output_buffers(handle);

    if (ret_in != 0) {
        printk(KERN_ALERT "directory_kmodule: handle_process_input_buffers: %d\n", ret_in);
    }
    if (ret_out != 0) {
        printk(KERN_ALERT "directory_kmodule: handle_process_output_buffers: %d\n", ret_out);
    }

    up(&directory_lock);
    return ret_in != 0 ? ret_in : ret_out;
}

static void handle_free(struct user_handle* handle) {
    handle_process_buffers(handle);
    phonedir_free(&handle->pending_output_head);
    kfree(handle);
}

static ssize_t dev_read(struct file *file, char __user *data, size_t len, loff_t *pos) {
    ssize_t retval;
    struct user_handle *handle = file->private_data;

    handle_process_buffers(handle);

    spin_lock(&handle->lock);

    size_t data_remaining = handle->output_offset;
    if (data_remaining == 0) {
        retval = 0;
        goto ret;
    }

    size_t copied = min(len, data_remaining);
    if (copy_to_user(data, handle->output_buf, copied)) {
        retval = -EFAULT;
        goto ret;
    }

    handle->output_offset -= copied;
    if (copied != data_remaining) {
        memmove(handle->output_buf, handle->output_buf + copied, data_remaining - copied);
    }
    retval = copied;

    ret:
        spin_unlock(&handle->lock);
        return retval;
}

static ssize_t dev_write(struct file *file, const char __user *data, size_t len, loff_t *pos) {
    ssize_t retval;
    struct user_handle *handle = file->private_data;
    spin_lock(&handle->lock);

    size_t space_remaining = USER_BUF_CAP - handle->input_offset;

    if (space_remaining == 0) {
        spin_unlock(&handle->lock);
        handle_process_buffers(handle);
        spin_lock(&handle->lock);

        space_remaining = USER_BUF_CAP - handle->input_offset;
        if (space_remaining == 0) {
            retval = -ENOSPC;
            goto ret;
        }
    }

    size_t copied = min(len, space_remaining);
    if (copy_from_user(handle->input_buf + handle->input_offset, data, copied)) {
        retval = -EFAULT;
        goto ret;
    }

    handle->input_offset += copied;
    retval = copied;

    ret:
        spin_unlock(&handle->lock);
        if (retval > 0) {
            handle_process_buffers(handle);
        }
        return retval;
}

static int dev_open(struct inode *inode, struct file *file) {
    struct user_handle *handle = handle_alloc();
    if (!handle) {
        return -ENOMEM;
    }

    file->private_data = handle;
    return 0;
}

static int dev_release(struct inode *inode, struct file *file) {
    handle_free((struct user_handle *) file->private_data);
    return 0;
}

static struct file_operations dev_ops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release
};

static char* surname_from_user(void* ptr, unsigned int len, long* retval) {
    if (len >= PHONEDIR_NAME_LEN || len == 0) {
        *retval = -EINVAL;
        return NULL;
    }

    char* surname = kmalloc(len + 1, GFP_KERNEL);
    if (!surname) {
        *retval = -ENOMEM;
        return NULL;
    }

    if (copy_from_user(surname, ptr, len)) {
        *retval = -EFAULT;
        kfree(surname);
        return NULL;
    }
    surname[len] = 0;

    *retval = 0;
    return surname;
}

static long sys_add(void *ptr) {
    printk(KERN_INFO "directory_kmodule: sys_add\n");

    size_t rec_size = sizeof(struct phonedir_record);
    struct phonedir_record *record = kmalloc(rec_size, GFP_KERNEL);
    if (!record) {
        return -ENOMEM;
    }

    long retval = 0;
    if (copy_from_user(record, ptr, rec_size)) {
        retval = -EFAULT;
        goto ret;
    }

    if (down_interruptible(&directory_lock)) {
        retval = -EINTR;
        goto ret;
    }
    retval = phonedir_add_record(&phone_directory, record);
    up(&directory_lock);

    ret:
        kfree(record);
        return retval;
}

static long sys_del(void *ptr, unsigned int len) {
    printk(KERN_INFO "directory_kmodule: sys_del\n");

    long retval = 0;
    char* surname = surname_from_user(ptr, len, &retval);
    if (!surname) {
        return retval;
    }

    if (down_interruptible(&directory_lock)) {
        retval = -EINTR;
        goto ret;
    }
    phonedir_del(&phone_directory, surname);
    up(&directory_lock);

    ret:
        kfree(surname);
        return retval;
}

static long sys_find(void *ptr0, unsigned int len, void *ptr1) {
    printk(KERN_INFO "directory_kmodule: sys_find\n");

    long retval = 0;
    struct list_head* found = NULL;
    char* surname = surname_from_user(ptr0, len, &retval);
    if (!surname) {
        return retval;
    }

    if (down_interruptible(&directory_lock)) {
        retval = -EINTR;
        goto ret;
    }

    found = phonedir_find(&phone_directory, surname);
    if (!found) {
        retval = -ENOMEM;
        goto ret_unlock;
    }

    if (list_empty(found)) {
        retval = 0;
        goto ret_unlock;
    }

    struct directory_entry *first_entry = list_first_entry(found, struct directory_entry, list);
    if (copy_to_user(ptr1, &first_entry->record, sizeof(struct phonedir_record))) {
        retval = -EFAULT;
        goto ret_unlock;
    }
    retval = 1;

    ret_unlock:
        up(&directory_lock);
    ret:
        if (found) {
            phonedir_free(found);
            kfree(found);
        }
        kfree(surname);
        return retval;
}

static struct module_sys_descritor sys_ops = {
    .sys_ptr = sys_add,
    .sys_ptr_uint = sys_del,
    .sys_ptr_uint_ptr = sys_find
};


static int __init first_module_init(void) {
    printk(KERN_INFO "directory_kmodule: init\n");
    dev_major_num = register_chrdev(0, device_name, &dev_ops);
    if (dev_major_num < 0) {
        printk(KERN_ALERT "directory_kmodule: cannot register device\n");
        return dev_major_num;
    }

    memcpy(sys_ops.mod_name, THIS_MODULE->name, MODULE_NAME_LEN);
    module_sys_publish(&sys_ops);

    return 0;
}

static void __exit first_module_exit(void) {
    printk(KERN_INFO "directory_kmodule: exit\n");
    unregister_chrdev(dev_major_num, device_name);
    module_sys_retract();
    phonedir_free(&phone_directory);
}


module_init(first_module_init);
module_exit(first_module_exit);
