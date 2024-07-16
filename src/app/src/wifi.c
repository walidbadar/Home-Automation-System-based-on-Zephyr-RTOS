/*
 * Copyright (c) 2024 Muhammad Waleed.
 * Copyright (c) 2023 Craig Peacock.
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/toolchain.h>
#include <zephyr/logging/log.h>

#include "wifi.h"
#include "config.h"

LOG_MODULE_REGISTER(wifi_app, LOG_LEVEL_DBG);

static K_SEM_DEFINE(wifi_connected, 0, 1);
static K_SEM_DEFINE(ipv4_address_obtained, 0, 1);

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static void handle_ipv4_result(struct net_if *iface)
{
    int i = 0;

    for (i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {

        char buf[NET_IPV4_ADDR_LEN];

        if (iface->config.ip.ipv4->unicast[i].addr_type != NET_ADDR_DHCP) {
            continue;
        }

        LOG_INF("IPv4 address: %s",
                net_addr_ntop(AF_INET,
                                &iface->config.ip.ipv4->unicast[i].address.in_addr,
                                buf, sizeof(buf)));
        LOG_INF("Subnet: %s",
                net_addr_ntop(AF_INET,
                                &iface->config.ip.ipv4->netmask,
                                buf, sizeof(buf)));
        LOG_INF("Router: %s",
                net_addr_ntop(AF_INET,
                                &iface->config.ip.ipv4->gw,
                                buf, sizeof(buf)));
    }

    k_sem_give(&ipv4_address_obtained);
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status)
    {
        LOG_INF("Connection request failed (%d)", status->status);
    }
    else
    {
        LOG_INF("Connected");
        k_sem_give(&wifi_connected);
    }
}

static int handle_wifi_disconnect_result(struct net_mgmt_event_callback *cb)
{
    const struct wifi_status *status = (const struct wifi_status *)cb->info;

    if (status->status)
    {
        LOG_INF("Disconnection request (%d)", status->status);
    }
    else
    {
        LOG_INF("Disconnected");
        k_sem_take(&wifi_connected, K_NO_WAIT);
    }
	return status->status;
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event, struct net_if *iface)
{
	LOG_INF("mgmt_event: %d", mgmt_event);
    switch (mgmt_event)
    {
        case NET_EVENT_WIFI_AP_ENABLE_RESULT:
            LOG_INF("ESP32 WiFi AP enabled");
            break;

        case NET_EVENT_WIFI_CONNECT_RESULT:
            handle_wifi_connect_result(cb);
            break;

        case NET_EVENT_WIFI_DISCONNECT_RESULT:
            handle_wifi_disconnect_result(cb);
            break;

        case NET_EVENT_IPV4_ADDR_ADD:
            handle_ipv4_result(iface);
            break;

        default:
            break;
    }
}

void wifi_connect(char *SSID, char *PSK)
{
    struct net_if *iface = net_if_get_default();

    struct wifi_connect_req_params wifi_params = {0};

    wifi_params.ssid = SSID;
    wifi_params.psk = PSK;
    wifi_params.ssid_length = strlen(SSID);
    wifi_params.psk_length = strlen(PSK);
    wifi_params.channel = WIFI_CHANNEL_ANY;
    wifi_params.security = WIFI_SECURITY_TYPE_PSK;
    wifi_params.band = WIFI_FREQ_BAND_2_4_GHZ;
    wifi_params.mfp = WIFI_MFP_OPTIONAL;

    LOG_INF("Connecting to SSID: %s", wifi_params.ssid);

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(struct wifi_connect_req_params)))
    {
        LOG_INF("WiFi Connection Request Failed");
    }
}

int8_t wifi_status(void)
{
    struct net_if *iface = net_if_get_default();

    struct wifi_iface_status status = {0};

    if (net_mgmt(NET_REQUEST_WIFI_IFACE_STATUS, iface, &status,	sizeof(struct wifi_iface_status)))
    {
        LOG_INF("WiFi Status Request Failed");
        return 0;
    }

    if (status.state >= WIFI_STATE_ASSOCIATED) {
        LOG_INF("SSID: %-32s", status.ssid);
        LOG_INF("Band: %s", wifi_band_txt(status.band));
        LOG_INF("Channel: %d", status.channel);
        LOG_INF("Security: %s", wifi_security_txt(status.security));
        LOG_INF("RSSI: %d", status.rssi);
    }

    return (int8_t)status.state;
}

void wifi_disconnect(void)
{
    struct net_if *iface = net_if_get_default();

    if (net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0))
    {
        LOG_INF("WiFi Disconnection Request Failed");
    }
}

void wifi_ap(void){

    struct wifi_connect_req_params ap_params = {
        .ssid = "ZephyrAP",
        .ssid_length = strlen("ZephyrAP"),
        .channel = 6,
        .security = WIFI_SECURITY_TYPE_PSK,
        .psk = "password123",
        .psk_length = strlen("password123"),
    };

    struct net_if *iface = net_if_get_default();
    if (!iface) {
        LOG_ERR("Default network interface not found");
        return;
    }

    struct in_addr my_addr, gw, netmask;

    if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_ADDR, &my_addr) < 0) {
        LOG_ERR("Invalid IP address: %s", CONFIG_NET_CONFIG_MY_IPV4_ADDR);
        return;
    }

    if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_NETMASK, &netmask) < 0) {
        LOG_ERR("Invalid netmask: %s", CONFIG_NET_CONFIG_MY_IPV4_NETMASK);
        return;
    }

    if (net_addr_pton(AF_INET, CONFIG_NET_CONFIG_MY_IPV4_GW, &gw) < 0) {
        LOG_ERR("Invalid gateway: %s", CONFIG_NET_CONFIG_MY_IPV4_GW);
        return;
    }

    net_if_ipv4_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
    net_if_ipv4_set_gw(iface, &gw);
    net_if_ipv4_set_netmask(iface, &netmask);

	LOG_INF("Attempting to enable WiFi AP mode...");
    int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &ap_params, sizeof(ap_params));
    if (ret) {
        LOG_ERR("Failed to enable AP mode. Error code: %d", ret);
        return;
    }

    LOG_INF("WiFi AP enabled successfully. SSID: %s, IP: %s", ap_params.ssid, CONFIG_NET_CONFIG_MY_IPV4_ADDR);
}

int8_t wifi_init(char *SSID, char *PSK)
{
    LOG_INF("WiFi Initialized: %s", CONFIG_BOARD);

    net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_event_handler,
                                 NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);

    net_mgmt_init_event_callback(&ipv4_cb, wifi_mgmt_event_handler, NET_EVENT_IPV4_ADDR_ADD);

    net_mgmt_add_event_callback(&wifi_cb);
    net_mgmt_add_event_callback(&ipv4_cb);

    // wifi_ap();
    wifi_connect(SSID, PSK);
    k_sem_take(&wifi_connected, K_FOREVER);
    wifi_status();
    k_sem_take(&ipv4_address_obtained, K_FOREVER);
	// LOG_INF("Wifi Status: %d", handle_wifi_disconnect_result(&wifi_cb));
    LOG_INF("Ready...");

	return 0;
}