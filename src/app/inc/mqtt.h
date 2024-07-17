/*
 * Copyright (c) 2024 Muhammad Waleed.
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MQTT_CONFIG_H
#define MQTT_CONFIG_H

#ifdef CONFIG_NET_CONFIG_SETTINGS
#define SERVER_ADDR		"192.168.1.102"
#define SERVER_PORT		1883
#endif

#define APP_CONNECT_TIMEOUT_MS	5000
#define APP_SLEEP_MSECS		100

#define APP_CONNECT_TRIES	10

#define APP_MQTT_BUFFER_SIZE	256

#define MQTT_CLIENTID		"zephyr"

/* The mqtt client connections status */
extern bool connected;

int8_t pub_switch_state(struct gpio_dt_spec *button, uint8_t index, char *pub_topics);
int8_t sub_relay_state(struct gpio_dt_spec *relay, char *payload, char *pub_topics);
int8_t pub_sub(void);

#endif

