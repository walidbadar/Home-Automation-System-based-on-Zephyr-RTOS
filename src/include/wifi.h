/*
 * Copyright (c) 2024 Muhammad Waleed.
 * Copyright (c) 2023 Craig Peacock.
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef WIFI_H
#define WIFI_H

int8_t wifi_status(void);
int8_t wifi_init(char *SSID, char *PSK);

#endif

