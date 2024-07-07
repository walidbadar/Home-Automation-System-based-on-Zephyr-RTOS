/*
 * Copyright (c) 2024 Muhammad Waleed.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include "include/gpio_config.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_config, LOG_LEVEL_DBG);

/* Device Tree interface for Buttons*/
struct gpio_dt_spec buttons[maxButtons] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0})
};

static struct gpio_callback gpio_cb[maxButtons];

/* Device Tree interface for Relay.  */
struct gpio_dt_spec relays[maxRelays] = {
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(rly0), gpios, {0}),
    GPIO_DT_SPEC_GET_OR(DT_ALIAS(rly1), gpios, {0})
};

/* Generic button handler */
void button(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    for (int i = 0; i<LIMIT; i++) {
        if (pins & BIT(buttons[i].pin)) {
            bool state = digital_read(&buttons[i]);
            digital_write(&relays[i], state);
            // LOG_INF("Button %d pressed, Relay %d set to %d", i, i, state);
        }
    }
}

uint8_t pin_mode(struct gpio_dt_spec *user_gpio, uint32_t dir) {
    uint8_t ret;

    if (!gpio_is_ready_dt(user_gpio)) {
        LOG_ERR("Error: GPIO device %s is not ready", user_gpio->port->name);
        return 0;
    }

    ret = gpio_pin_configure_dt(user_gpio, dir);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure %s pin %d", ret, user_gpio->port->name, user_gpio->pin);
        return 0;
    }

    if (dir == GPIO_INPUT) {
        ret = gpio_pin_interrupt_configure_dt(user_gpio, GPIO_INT_EDGE_BOTH);
        if (ret != 0) {
            LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret, user_gpio->port->name, user_gpio->pin);
            return 0;
        }

        /* Initialize callback */
        gpio_init_callback(&gpio_cb[user_gpio->pin], button, BIT(user_gpio->pin));
        gpio_add_callback(user_gpio->port, &gpio_cb[user_gpio->pin]);
    }

    return ret;
}