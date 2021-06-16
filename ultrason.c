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

#define MILLIARD 1000000000

static int delai=500000000;
static int ret;
int TIMEOUT1=500000000;
int TIMEOUT2=10000000;
int distance;

rtdm_task_t task_desc;
#define GPIO_OSCILLATEUR 5

void task(void *arg)
{
    while (!rtdm_task_should_stop())
    {
        int err;
        long count = 0;
        if ((err = gpio_direction_output(GPIO_OSCILLATEUR, 0)) != 0)
        {
            gpio_free(GPIO_OSCILLATEUR);
            return err;
        }
        gpio_set_value(GPIO_OSCILLATEUR, 0);
        rtdm_task_sleep(2000);
        gpio_set_value(GPIO_OSCILLATEUR, 1);
        rtdm_task_sleep(10000);
        gpio_set_value(GPIO_OSCILLATEUR, 0);
        
        int t0 = rtdm_clock_read();

        if ((err = gpio_direction_input(GPIO_OSCILLATEUR)) != 0)
        {
            gpio_free(GPIO_OSCILLATEUR);
            return err;
        }
        
        while (count < TIMEOUT1)
        {
            if (gpio_get_value(GPIO_OSCILLATEUR) == 1)
                break;
            count++;
        }

        if (count >= TIMEOUT1)
            return;

        int t1 = rtdm_clock_read();
        count = 0;
        while (count < TIMEOUT2)
        {
            if (gpio_get_value(GPIO_OSCILLATEUR) == 0)
                break;
            count++;
        }

        int t2 = rtdm_clock_read();
        
        int dt=(t1 - t0) * MILLIARD;

        distance = (t2 - t1) * 340 / 200000; //0.1 mm

        rtdm_printk("Distance = %d", distance);

        rtdm_task_wait_period(NULL);
    }
}

static int __init example_init(void)
{

    rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);

    ret = rtdm_task_init(&task_desc, "rtdm-ultrason", task, NULL, 30, delai);

    int err;

    if ((err = gpio_request(GPIO_OSCILLATEUR, THIS_MODULE->name)) != 0)
    {
        return err;
    }
    if ((err = gpio_direction_output(GPIO_OSCILLATEUR, 1)) != 0)
    {
        gpio_free(GPIO_OSCILLATEUR);
        return err;
    }
    if (ret)
    {
        rtdm_printk(KERN_INFO "%s.%s() : error rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);
    }

    else
        rtdm_printk(KERN_INFO "%s.%s() : success rtdm_task_init\n", THIS_MODULE->name, __FUNCTION__);

    return 0;
}

static void __exit example_exit(void)
{

    rtdm_task_destroy(&task_desc);
    gpio_free(GPIO_OSCILLATEUR);

    rtdm_printk(KERN_INFO "%s.%s()\n", THIS_MODULE->name, __FUNCTION__);
}

module_init(example_init);
module_exit(example_exit);
MODULE_LICENSE("GPL");
