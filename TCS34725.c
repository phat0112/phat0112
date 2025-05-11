#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include "tcs34725.h"

#define DRIVER_NAME "tcs34725"
#define DEVICE_NAME "tcs34725"
#define CLASS_NAME "tcs34725"

static struct i2c_client *tcs34725_client;
static struct class *tcs34725_class;
static struct device *tcs34725_device;
static dev_t dev_num;
static struct cdev tcs34725_cdev;

/* Hàm ghi giá trị vào register của cảm biến */
static int tcs34725_write_register(struct i2c_client *client, u8 reg, u8 value)
{
    int ret;
    u8 buf[2];
    
    buf[0] = TCS34725_COMMAND_BIT | reg;
    buf[1] = value;
    
    ret = i2c_master_send(client, buf, 2);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to write register 0x%02x\n", reg);
        return ret;
    }
    
    return 0;
}

/* Hàm đọc giá trị từ register của cảm biến */
static int tcs34725_read_register(struct i2c_client *client, u8 reg, u8 *value)
{
    int ret;
    u8 buf[1];
    
    buf[0] = TCS34725_COMMAND_BIT | reg;
    
    ret = i2c_master_send(client, buf, 1);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to send read command for register 0x%02x\n", reg);
        return ret;
    }
    
    ret = i2c_master_recv(client, value, 1);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read register 0x%02x\n", reg);
        return ret;
    }
    
    return 0;
}

/* Hàm đọc giá trị màu từ cảm biến */
static int tcs34725_read_colors(struct i2c_client *client, struct tcs34725_data *data)
{
    u8 buf[8];
    int ret;
    
    /* Đọc 8 byte dữ liệu màu bắt đầu từ CDATAL */
    buf[0] = TCS34725_COMMAND_BIT | TCS34725_CDATAL;
    
    ret = i2c_master_send(client, buf, 1);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to send color read command\n");
        return ret;
    }
    
    ret = i2c_master_recv(client, buf, 8);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read color data\n");
        return ret;
    }
    
    data->clear = (buf[1] << 8) | buf[0];
    data->red = (buf[3] << 8) | buf[2];
    data->green = (buf[5] << 8) | buf[4];
    data->blue = (buf[7] << 8) | buf[6];
    
    return 0;
}

/* Hàm khởi tạo cảm biến */
static int tcs34725_init(struct i2c_client *client)
{
    int ret;
    u8 id;
    
    /* Kiểm tra ID của cảm biến */
    ret = tcs34725_read_register(client, TCS34725_ID, &id);
    if (ret < 0) {
        return ret;
    }
    
    if ((id != 0x44) && (id != 0x4D)) {
        dev_err(&client->dev, "Invalid TCS34725 ID: 0x%02x\n", id);
        return -ENODEV;
    }
    
    /* Bật cảm biến */
    ret = tcs34725_write_register(client, TCS34725_ENABLE, TCS34725_ENABLE_PON);
    if (ret < 0) {
        return ret;
    }
    
    msleep(3);  /* Chờ 3ms sau khi bật nguồn */
    
    /* Bật RGBC và chờ */
    ret = tcs34725_write_register(client, TCS34725_ENABLE, 
                                 TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN | TCS34725_ENABLE_WEN);
    if (ret < 0) {
        return ret;
    }
    
    /* Thiết lập thời gian tích hợp (ATIME) */
    ret = tcs34725_write_register(client, TCS34725_ATIME, TCS34725_INTEGRATIONTIME_101MS);
    if (ret < 0) {
        return ret;
    }
    
    /* Thiết lập hệ số khuếch đại (GAIN) */
    ret = tcs34725_write_register(client, TCS34725_CONTROL, TCS34725_GAIN_1X);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

/* Hàm xử lý file operations */
static int tcs34725_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int tcs34725_release(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t tcs34725_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    struct tcs34725_data data;
    int ret;
    
    if (count < sizeof(data)) {
        return -EINVAL;
    }
    
    ret = tcs34725_read_colors(tcs34725_client, &data);
    if (ret < 0) {
        return ret;
    }
    
    if (copy_to_user(buf, &data, sizeof(data))) {
        return -EFAULT;
    }
    
    return sizeof(data);
}

static const struct file_operations tcs34725_fops = {
    .owner = THIS_MODULE,
    .open = tcs34725_open,
    .release = tcs34725_release,
    .read = tcs34725_read,
};

/* Hàm probe và remove cho driver I2C */
static int tcs34725_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
    
    /* Kiểm tra kết nối I2C */
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        dev_err(&client->dev, "I2C functionality not supported\n");
        return -ENODEV;
    }
    
    tcs34725_client = client;
    
    /* Khởi tạo cảm biến */
    ret = tcs34725_init(client);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to initialize TCS34725\n");
        return ret;
    }
    
    /* Đăng ký device */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to allocate device number\n");
        return ret;
    }
    
    /* Tạo class */
    tcs34725_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(tcs34725_class)) {
        unregister_chrdev_region(dev_num, 1);
        dev_err(&client->dev, "Failed to create device class\n");
        return PTR_ERR(tcs34725_class);
    }
    
    /* Tạo device */
    tcs34725_device = device_create(tcs34725_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(tcs34725_device)) {
        class_destroy(tcs34725_class);
        unregister_chrdev_region(dev_num, 1);
        dev_err(&client->dev, "Failed to create device\n");
        return PTR_ERR(tcs34725_device);
    }
    
    /* Khởi tạo cdev */
    cdev_init(&tcs34725_cdev, &tcs34725_fops);
    tcs34725_cdev.owner = THIS_MODULE;
    
    /* Thêm cdev vào hệ thống */
    ret = cdev_add(&tcs34725_cdev, dev_num, 1);
    if (ret < 0) {
        device_destroy(tcs34725_class, dev_num);
        class_destroy(tcs34725_class);
        unregister_chrdev_region(dev_num, 1);
        dev_err(&client->dev, "Failed to add character device\n");
        return ret;
    }
    
    dev_info(&client->dev, "TCS34725 color sensor initialized\n");
    return 0;
}

static int tcs34725_remove(struct i2c_client *client)
{
    /* Tắt cảm biến */
    tcs34725_write_register(client, TCS34725_ENABLE, 0x00);
    
    /* Dọn dẹp thiết bị */
    cdev_del(&tcs34725_cdev);
    device_destroy(tcs34725_class, dev_num);
    class_destroy(tcs34725_class);
    unregister_chrdev_region(dev_num, 1);
    
    dev_info(&client->dev, "TCS34725 color sensor removed\n");
    return 0;
}

static const struct i2c_device_id tcs34725_id[] = {
    { "tcs34725", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, tcs34725_id);

static const struct of_device_id tcs34725_of_match[] = {
    { .compatible = "ams,tcs34725" },
    { }
};
MODULE_DEVICE_TABLE(of, tcs34725_of_match);

static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name = DRIVER_NAME,
        .of_match_table = of_match_ptr(tcs34725_of_match),
    },
    .probe = tcs34725_probe,
    .remove = tcs34725_remove,
    .id_table = tcs34725_id,
};

module_i2c_driver(tcs34725_driver);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Driver for TCS34725 Color Sensor");
MODULE_LICENSE("GPL");
