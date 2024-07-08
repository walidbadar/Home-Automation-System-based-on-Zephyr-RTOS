/*
 * Copyright (c) 2024 Muhammad Waleed.
 * Copyright (c) 2023 Craig Peacock.
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>

#include <stdlib.h>
#include <stdbool.h>

#include "include/wifi.h"
#include "include/mqtt_config.h"
#include "include/gpio_config.h"

/**
 * @brief The main loop.
 *
 * This function initializes Wi-Fi and then enters an infinite loop,
 * calling the pubSub() function every 100 milliseconds.
 */
int main(void)
{
    int rc = 0;

    /* Initialize the GPIO pins for the buttons and relays */

    // Loop over the number of buttons and relays
    for (int index = 0; index < LIMIT; index++) {
        // Set the direction of each button to input
        pin_mode(&buttons[index], GPIO_INPUT);
        // Set the direction of each relay to output
        pin_mode(&relays[index], GPIO_OUTPUT);
    }

    for (int index = 0; index<LIMIT; i++) {
        bool state = digital_read(&buttons[index]);
        digital_write(&relays[index], state);
    }

    /* Initialize Wi-Fi with the SSID and password */
    rc = wifi_init("SSID", "PSK");

    while (1) {
        /*
         * Publish and subscribe to MQTT topics every 100 milliseconds.
         * This function will handle connecting to the MQTT broker,
         * subscribing to topics, and publishing messages.
         */
        pub_sub();

        // Sleep for 100 milliseconds before the next iteration
        k_msleep(100);
    }
}

