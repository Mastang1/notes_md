#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)

static const struct gpio_dt_spec led0 =
    GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct gpio_dt_spec led1 =
    GPIO_DT_SPEC_GET(LED1_NODE, gpios);

int main(void)
{
    int ret;

    printk("week2 f407 explorer smoke\n");
    printk("led0 ready=%d\n", gpio_is_ready_dt(&led0));
    printk("led1 ready=%d\n", gpio_is_ready_dt(&led1));

    if (!gpio_is_ready_dt(&led0) ||
        !gpio_is_ready_dt(&led1)) {
        printk("GPIO device not ready\n");
        return 0;
    }

    ret = gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        printk("led0 configure failed: %d\n", ret);
        return 0;
    }

    ret = gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
        printk("led1 configure failed: %d\n", ret);
        return 0;
    }

    for (unsigned int count = 0; ; ++count) {
        gpio_pin_toggle_dt(&led0);
        gpio_pin_toggle_dt(&led1);

        printk("count=%u\n", count);
        k_sleep(K_MSEC(500));
    }

    return 0;
}
