#include <linux/version.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/fs.h>

#include <asm/uaccess.h>

#include <rtdm/driver.h>

#define GPIO_LED 18
#define GPIO_bouton 19
rtdm_task_t task_bouton;
static int ini_task_bouton;
static int periode_us = 1000*1000; //en us

#define DATA_SIZE 128
static char *msg = "-1";
static char my_data [DATA_SIZE];

static int etat_appuyer = -1;

void bouton (void *arg){
    while(!rtdm_task_should_stop()) {
        
//        gpio_direction_output (GPIO_LED, 0);
//        rtdm_task_sleep (1000000000LL); //attendre pour 1s
//        gpio_direction_output (GPIO_LED, 1);
//        rtdm_task_sleep (1000000000LL); //attendre pour 1s
        
        etat_appuyer = gpio_get_value (GPIO_bouton); //low en appuyant
        if (etat_appuyer == 0) {
            gpio_direction_output (GPIO_LED, 1);
            //rtdm_printk (KERN_INFO "bouton appuye detecte dans la tache");
        }
        else if (etat_appuyer == 1) {
            gpio_direction_output (GPIO_LED, 0);
            //rtdm_printk (KERN_INFO "bouton libre");
        }
        //else
            //rtdm_printk(KERN_INFO "bouton etat = %d", etat_appuyer);


        rtdm_task_wait_period(NULL);
    }
}



static int my_open_function(struct rtdm_fd *fd, int flags)
{
    //rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
    return 0;
}



static void my_close_function(struct rtdm_fd *fd)
{
    //rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
}



static int my_read_nrt_function  (struct rtdm_fd *fd, void __user *buffer, size_t lg)
{
    //rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
    //rtdm_printk (KERN_INFO "etat obtenu dans la fct de read %d \n", etat_appuyer);
    if (etat_appuyer == 0) {
        msg = "bouton_appuye";
        //rtdm_printk (KERN_INFO "bouton appuye obtenu dans la fct de read \n");
    }
    else if (etat_appuyer == 1) {
        msg = "bouton_libre";
        //rtdm_printk (KERN_INFO "bouton libre obtenu dans la fct de read \n");
    }
    if (lg > 0) {
        if (rtdm_safe_copy_to_user(fd, buffer, msg, lg) != 0) {
            return -EFAULT;
        }
    }

    //rtdm_printk("%s: sent %d bytes, \"%.*s\"\n",  __FUNCTION__, strlen(msg), strlen(msg), msg);

    return lg;
}



static int my_write_nrt_function(struct rtdm_fd *fd, const void __user *buffer, size_t lg)
{
    //rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

    if (lg > 0) {
        if (rtdm_safe_copy_from_user(fd,
                          &my_data,
                          buffer, lg) != 0) {
            return -EFAULT;
        }
    }

    rtdm_printk("==> Received from Linux %d bytes : %.*s\n", lg, lg, my_data);

    return lg;
}



static struct rtdm_driver rt_driver_bouton = {

    .profile_info = RTDM_PROFILE_INFO(my_example, RTDM_CLASS_TESTING, 1, 1),

    .device_flags   = RTDM_NAMED_DEVICE,
    .device_count   = 1,
    .context_size   = 0,

    .ops = {
        .open      = my_open_function,
        .close     = my_close_function,
        .read_nrt  = my_read_nrt_function,
        .write_nrt = my_write_nrt_function,
    },
};


static struct rtdm_device rt_device_bouton = {

    .driver = &rt_driver_bouton,
    .label  = "rtdm_driver_%d",
};



static int __init bouton_init(void) {

    int err;
    //******initialiser GPIO du bouton
    if ((err = gpio_request(GPIO_bouton, THIS_MODULE->name)) != 0){
        return err;
        rtdm_printk("gpio_request bouton echoue %d", err);
    } //else rtdm_printk ("gpio_request bouton reussi");
    
    if ((err = gpio_request(GPIO_LED, THIS_MODULE->name)) != 0){
        return err;
        rtdm_printk("gpio_request LED echoue %d", err);
    } //else rtdm_printk ("gpio_request LED reussi");
    
    //******initialiser le task du bouton
    ini_task_bouton = rtdm_task_init(&task_bouton, "task_bouton", bouton, NULL, 30, periode_us*10);// periode = 10us
    if (ini_task_bouton) {
        rtdm_printk(KERN_INFO "initialisation %s bouton echue %d", THIS_MODULE->name, ini_task_bouton);
    } //else  rtdm_printk(KERN_INFO "%s bien initialise", THIS_MODULE->name);
    
    //******initialiser le device pour communiquer
    rtdm_dev_register(&rt_device_bouton);
    
    rtdm_printk (KERN_INFO "%s bien initialise", THIS_MODULE->name);
    
    return 0;
}

static void __exit bouton_exit(void) {
    
    rtdm_task_destroy(&task_bouton);
    //rtdm_printk(KERN_INFO "task desroied");
    
    gpio_free(GPIO_bouton);
    //rtdm_printk(KERN_INFO "bouton free");
    gpio_free(GPIO_LED);
    //rtdm_printk(KERN_INFO "LED free");

    rtdm_dev_unregister(&rt_device_bouton);
    
    rtdm_printk(KERN_INFO "%s exit", THIS_MODULE->name);
}

module_init(bouton_init);
module_exit(bouton_exit);
MODULE_LICENSE("GPL");
