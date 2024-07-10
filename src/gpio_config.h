/*
 * Copyright (c) 2024 Muhammad Waleed.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#define digital_read(input) gpio_pin_get_dt(input)
#define digital_write(output, val) gpio_pin_set_dt(output, val)

/* Define Number of button to be used. */
#define maxButtons 2

/* Define Number of relay to be used. */
#define maxRelays 2

/* Device Tree interface for Button.  */
extern struct gpio_dt_spec buttons[maxButtons];

/* Device Tree interface for Relay.  */
extern struct gpio_dt_spec relays[maxRelays];

/* GPIO Direction Control  */
uint8_t pin_mode(struct gpio_dt_spec *user_gpio, uint32_t dir);

#endif

