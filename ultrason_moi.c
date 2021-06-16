#include <linux/version.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/fs.h>

#include <asm/uaccess.h>
#include <asm/div64.h>

#include <rtdm/rtdm.h>
#include <rtdm/driver.h>

static int periode_us = 1000; //en us
static int periode_ultrason = 1000*1000; //en ms

static void timer_ultrason (rtdm_timer_t *);
static rtdm_timer_t rtimer;

static int ini_task_ultrason;
rtdm_task_t task_ultrason;

#define MY_DATA_SIZE  128
static char my_data [MY_DATA_SIZE];

static char *msg = "msg initiale";
//static double msg = 1.66;

#define GPIO_Ultrason 5

static nanosecs_abs_t t0, t1, t2;
static int dis = 0;
static int dt = 0;
static int count = 0;
static int distance = 1;

int ultrason_obtenir_distance (void) {
    gpio_direction_output(GPIO_Ultrason, 0);
    rtdm_task_sleep (2000000LL);//2ms
    //rtdm_timer_start(&rtimer, periode_us*1000*2, periode_us*1000*2, RTDM_TIMERMODE_ABSELUTE);
    gpio_direction_output(GPIO_Ultrason, 1);
    rtdm_task_sleep (1000000LL);//1ms
    //rtdm_timer_start(&rtimer, periode_us*1000*10, periode_us*1000*10, RTDM_TIMERMODE_RELATIVE);
    gpio_direction_output(GPIO_Ultrason, 0);
    
    if ((err = gpio_direction_input(GPIO_OSCILLATEUR)) != 0)
    {
        gpio_free(GPIO_Ultrason);
        return err;
    }
    //count = gpio_get_value(GPIO_Ultrason);
    //rtdm_printk (KERN_INFO "gpio = %d", count);
    
    t0 = rtdm_clock_read();//en ns
    //rtdm_printk(KERN_INFO "t0 = %d", t0);
    count = 0;
    while (count < 1000) {
        if (gpio_get_value(GPIO_Ultrason) == 1) {
            break;
        }
        count += 1;
    }
    if (count >= 1000) {
        rtdm_printk (KERN_INFO "time out1");
        return 0;
    }

    t1 = rtdm_clock_read();
    count = 0;
    while (count < 10000) {
        if (gpio_get_value(GPIO_Ultrason) != 1) {
            break;
        }
        count += 1;
    }
    if (count >= 10000) {
        rtdm_printk (KERN_INFO "time out2");
        return 0;
    }
    
    t2 = rtdm_clock_read();
    dt = t1 - t0;
    dt = (int)(dt / 1000); //en us
    rtdm_printk (KERN_INFO "dt = %d", dt);
    if (dt > 530) {
        rtdm_printk (KERN_INFO "out of range");
        return 0;
        //rtdm_printk (KERN_INFO "%d", dt);
    }

    dis = t2 - t1; // en ns
    dis = (int)(dis / 1000); // en us
    rtdm_printk (KERN_INFO "dis = %d", dis);
    dis = (int)(dis / 29); // en cm
    dis = (int)(dis / 2); // en cm
    rtdm_printk (KERN_INFO "distance = %d", dis);
    
    return dis;
}

void task_ultrason_obtenir_distance (void *arg){
    while(!rtdm_task_should_stop()) {
        
        distance = ultrason_obtenir_distance();
        if (distance != 0) {
            rtdm_printk("bonne distance = %d", distance);
        }
        rtdm_printk("distance = %d", distance);
        //distance = 140.01;
        //char double_to_str[128] = distance;
        //char double_to_str = "test";
        //rtdm_printk("1_change : %s", double_to_str);
        //sprintf(double_to_str, "%f", distance); // ne marche pas
        //rtdm_printk("2_change : %s", double_to_str);
        //char* itoa(distance, char* msg, int 10);
        //msg = distance; // la machine va etre en penne !!
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
    rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

    if (lg > 0) {
        if (rtdm_safe_copy_to_user(fd, buffer, msg, lg) != 0) {
            return -EFAULT;
        }
    }

    rtdm_printk("%s: sent %d bytes, \"%.*s\"\n",  __FUNCTION__, sizeof(msg), sizeof(msg), msg);

    return lg;
}



static int my_write_nrt_function(struct rtdm_fd *fd, const void __user *buffer, size_t lg)
{
    rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

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



static struct rtdm_driver rt_driver_ultrason = {

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


static struct rtdm_device rt_device_ultrason = {

    .driver = &rt_driver_ultrason,
    .label  = "rtdm_driver_%d",
};




static int __init ultrason_init(void) {

    //static int value = -1;
    int err;

    //****** initialiser GPIO du capteur ultrason ******à
    
    if ((err = gpio_request(GPIO_Ultrason, THIS_MODULE->name)) != 0)
    {
        return err;
        rtdm_printk("gpio_request ultrason echoue %d", err);
    } //else rtdm_printk ("gpio_request ultrason reussi");
    
//    if ((err = gpio_direction_output(GPIO_Ultrason, 1)) != 0)
//    {
//        gpio_free(GPIO_Ultrason);
//        return err;
//    } //else rtdm_printk ("gpio_direction_out reussi");

    //****** initialiser le task de l‘ultrason ******
    
    ini_task_ultrason = rtdm_task_init(&task_ultrason, "ultrason_obtenir_donnees", task_ultrason_obtenir_distance, NULL, 30, periode_ultrason);

    if (ini_task_ultrason != 0)
    {
        rtdm_printk(KERN_INFO "initialisation %s ultrason echue %d", THIS_MODULE->name, ini_task_ultrason);
    } //else  rtdm_printk(KERN_INFO "tesk bien initialise");
    
    //****** initialiser le timer ******
    
    if ((err = rtdm_timer_init(&rtimer, timer_ultrason, "timer_ultrason")) !=0 )
    {
        rtdm_printk("initialiser timer echoue");
        return err;
    } //else rtdm_printk("timer initialise");
    
    if ((err = rtdm_timer_start(&rtimer, periode_us*1000*1000, periode_us*1000*1000, RTDM_TIMERMODE_RELATIVE)) != 0)
    {
        rtdm_printk("timer start echoue");
        rtdm_timer_destroy(& rtimer);
        gpio_free(GPIO_Ultrason);
        return err;
    } //else rtdm_printk("timer start");
    
    //****** initaliser le device pour transfert les donnees ******
    rtdm_dev_register(&rt_device_ultrason);
    
    rtdm_printk("%s init", THIS_MODULE->name);
        
    return 0;
}



static void __exit ultrason_exit(void) {
    
    rtdm_task_destroy(&task_ultrason);
    
    rtdm_timer_stop(& rtimer);
    rtdm_timer_destroy(& rtimer);
    
    //rtdm_printk(KERN_INFO "%s exit apres timer", THIS_MODULE->name);
    
    gpio_free(GPIO_Ultrason);
    
    //rtdm_printk(KERN_INFO "%s exit apres gpio", THIS_MODULE->name);
    
    rtdm_dev_unregister(&rt_device_ultrason);
    
    rtdm_printk(KERN_INFO "%s exit", THIS_MODULE->name);
}


static void timer_ultrason(rtdm_timer_t * unused)
{
//    gpio_set_value(GPIO_Ultrason, value);
//    value = 1 - value;
}



module_init(ultrason_init);
module_exit(ultrason_exit);
MODULE_LICENSE("GPL");
