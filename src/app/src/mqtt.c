/*
 * Copyright (c) 2024 Muhammad Waleed.
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

#include "mqtt.h"
#include "gpio.h"
#include "config.h"

LOG_MODULE_REGISTER(mqtt_app, LOG_LEVEL_DBG);


#define SUCCESS_OR_EXIT(rc) { if (rc != 0) { return 1; } }
#define SUCCESS_OR_BREAK(rc) { if (rc != 0) { break; } }

/* Publish Topic list*/
char *pub_topics[] = {"/room2/status/outlet1", "/room2/status/outlet2"};
size_t size_of_pub_topics = ARRAY_SIZE(pub_topics);

/* Subscribed Topic list*/
char *sub_topics[] = {"/room2/set/outlet1", "/room2/set/outlet2"};
size_t size_of_sub_topics = ARRAY_SIZE(sub_topics);

/* Buffers for MQTT client. */
static uint8_t rx_buffer[APP_MQTT_BUFFER_SIZE];
static uint8_t tx_buffer[APP_MQTT_BUFFER_SIZE];

/* The mqtt client struct */
struct mqtt_client client_ctx;

/* MQTT Broker details. */
static struct sockaddr_storage broker;

static struct zsock_pollfd fds[1];
static int nfds;

bool connected;

static void prepare_fds(struct mqtt_client *client)
{
	if (client->transport.type == MQTT_TRANSPORT_NON_SECURE) {
		fds[0].fd = client->transport.tcp.sock;
	}

	fds[0].events = ZSOCK_POLLIN;
	nfds = 1;
}

static void clear_fds(void)
{
	nfds = 0;
}

static int wait(int timeout)
{
	int ret = 0;

	if (nfds > 0) {
		ret = zsock_poll(fds, nfds, timeout);
		if (ret < 0) {
			LOG_ERR("poll error: %d", errno);
		}
	}

	return ret;
}

void mqtt_evt_handler(struct mqtt_client *const client,
		      const struct mqtt_evt *evt)
{
	int err;

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT connect failed %d", evt->result);
			break;
		}

		connected = true;
		LOG_INF("MQTT client connected!");

		break;

	case MQTT_EVT_DISCONNECT:
		LOG_INF("MQTT client disconnected %d", evt->result);

		connected = false;
		clear_fds();

		break;

	case MQTT_EVT_SUBACK:
		if (evt->result != 0) {
			LOG_INF("MQTT SUBACK error %d", evt->result);
			break;
		}
		LOG_INF("SUB ACK Recieved");
	
		break;

	case MQTT_EVT_PUBACK:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBACK error %d", evt->result);
			break;
		}

		LOG_INF("PUBACK packet id: %u", evt->param.puback.message_id);

		break;

	case MQTT_EVT_PUBREC:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBREC error %d", evt->result);
			break;
		}

		LOG_INF("PUBREC packet id: %u", evt->param.pubrec.message_id);

		const struct mqtt_pubrel_param rel_param = {
			.message_id = evt->param.pubrec.message_id
		};

		err = mqtt_publish_qos2_release(client, &rel_param);
		if (err != 0) {
			LOG_ERR("Failed to send MQTT PUBREL: %d", err);
		}

		break;

	case MQTT_EVT_PUBCOMP:
		if (evt->result != 0) {
			LOG_ERR("MQTT PUBCOMP error %d", evt->result);
			break;
		}

		LOG_INF("PUBCOMP packet id: %u",
			evt->param.pubcomp.message_id);

		break;

	case MQTT_EVT_PINGRESP:
		LOG_INF("PINGRESP packet");
		break;

	case MQTT_EVT_PUBLISH:
		struct mqtt_puback_param puback;
		uint8_t data[2];
		char* subTopic = (uint8_t*)evt->param.publish.message.topic.topic.utf8;
		int len = evt->param.publish.message.payload.len;
		int bytes_read;
		
		LOG_INF("MQTT publish received %d, %d bytes", evt->result, len);
		LOG_INF("MQTT publish received topic: %s", subTopic);
		LOG_INF(" id: %d, qos: %d", evt->param.publish.message_id,
			evt->param.publish.message.topic.qos);

		while (len) {
			bytes_read = mqtt_read_publish_payload(&client_ctx,
					data,
					len >= sizeof(data) - 1 ?
					sizeof(data) - 1 : len);
			if (bytes_read < 0 && bytes_read != -EAGAIN) {
				LOG_ERR("failure to read payload");
				break;
			}

			data[bytes_read] = '\0';
			len -= bytes_read;
		}

		/* Toggle Relay State when payload is recieved from Home Assistant*/
		for(int index=0; index<LIMIT; index++)
			if (!strcmp(subTopic, sub_topics[index])) 
				sub_relay_state(&relays[index], data, pub_topics[index]);

		puback.message_id = evt->param.publish.message_id;
		mqtt_publish_qos1_ack(&client_ctx, &puback);
		break;

	default:
		break;
	}
}

int subscribe(struct mqtt_client *client, char *sub_topics[], size_t size_of_pub_topics)
{
	int ret;

	struct mqtt_topic topics[size_of_pub_topics];
	struct mqtt_subscription_list sub;

	for (size_t i = 0; i < ARRAY_SIZE(topics); ++i) {
        topics[i].topic.utf8 = sub_topics[i];
        topics[i].topic.size = strlen(sub_topics[i]);
        topics[i].qos = MQTT_QOS_0_AT_MOST_ONCE;
    }

	sub.list = topics;
	sub.list_count = size_of_pub_topics;
	sub.message_id = sys_rand32_get();

	LOG_INF("Subscribing to %hu topic(s)", sub.list_count);

	ret = mqtt_subscribe(client, &sub);
	if (ret != 0) {
		LOG_ERR("Failed to subscribe to topics: %d", ret);
	}

	return ret;
}

int publish(struct mqtt_client *client, char *pub_topics, char *payload)
{
	struct mqtt_publish_param param;

	param.message.topic.qos = 0;
	param.message.topic.topic.utf8 = (uint8_t *)pub_topics;
	param.message.topic.topic.size =
			strlen(param.message.topic.topic.utf8);
	param.message.payload.data = payload;
	param.message.payload.len =
			strlen(param.message.payload.data);
	param.message_id = sys_rand32_get();
	param.dup_flag = 0U;
	param.retain_flag = 0U;

	return mqtt_publish(client, &param);
}

#define RC_STR(rc) ((rc) == 0 ? "OK" : "ERROR")

#define PRINT_RESULT(func, rc) \
	LOG_INF("%s: %d <%s>", (func), rc, RC_STR(rc))

static void broker_init(void)
{
	struct sockaddr_in *broker4 = (struct sockaddr_in *)&broker;

	broker4->sin_family = AF_INET;
	broker4->sin_port = htons(SERVER_PORT);
	zsock_inet_pton(AF_INET, SERVER_ADDR, &broker4->sin_addr);
}

static void client_init(struct mqtt_client *client)
{
	mqtt_client_init(client);

	broker_init();

	/* MQTT client configuration */
	client->broker = &broker;
	client->evt_cb = mqtt_evt_handler;
	client->client_id.utf8 = (uint8_t *)MQTT_CLIENTID;
	client->client_id.size = strlen(MQTT_CLIENTID);
	client->password = NULL;
	client->user_name = NULL;
	client->protocol_version = MQTT_VERSION_3_1_1;

	/* MQTT buffers configuration */
	client->rx_buf = rx_buffer;
	client->rx_buf_size = sizeof(rx_buffer);
	client->tx_buf = tx_buffer;
	client->tx_buf_size = sizeof(tx_buffer);
}

/* In this routine we block until the connected variable is 1 */
int try_to_connect(struct mqtt_client *client)
{
	int rc, i = 0;

	while (i++ < APP_CONNECT_TRIES && !connected) {

		client_init(client);

		rc = mqtt_connect(client);
		if (rc != 0) {
			PRINT_RESULT("mqtt_connect", rc);
			k_sleep(K_MSEC(APP_SLEEP_MSECS));
			continue;
		}

		prepare_fds(client);

		if (wait(APP_CONNECT_TIMEOUT_MS)) {
			mqtt_input(client);
		}

		if (!connected) {
			mqtt_abort(client);
		}
	}

	if (connected) {
		return 0;
	}

	return -EINVAL;
}

int process_mqtt_and_sleep(struct mqtt_client *client, int timeout)
{
	int64_t remaining = timeout;
	int64_t start_time = k_uptime_get();
	int rc;

	while (remaining > 0 && connected) {
		if (wait(remaining)) {
			rc = mqtt_input(client);
			if (rc != 0) {
				PRINT_RESULT("mqtt_input", rc);
				return rc;
			}
		}

		rc = mqtt_live(client);
		if (rc != 0 && rc != -EAGAIN) {
			PRINT_RESULT("mqtt_live", rc);
			return rc;
		} else if (rc == 0) {
			rc = mqtt_input(client);
			if (rc != 0) {
				PRINT_RESULT("mqtt_input", rc);
				return rc;
			}
		}

		remaining = timeout + start_time - k_uptime_get();
	}

	return 0;
}

/*Publish Physical Switch State*/
int8_t pub_switch_state(struct gpio_dt_spec *button, uint8_t index, char *pub_topics){
    int8_t rc;
    bool currentState = digital_read(button);
    static bool previousState[32] = {false};

    // Check if the state has changed
	if(previousState[index] ^ currentState){
		if(currentState) rc = publish(&client_ctx, pub_topics, "1");
		else rc = publish(&client_ctx, pub_topics, "0");
	}
	previousState[index] = currentState;

	return rc;
}

/*Subsribe to Home Assistant Switch States*/
int8_t sub_relay_state(struct gpio_dt_spec *relay, char *payload, char *pub_topics){
	int8_t rc;

	if(!strcmp(payload, "1")) {
		rc = publish(&client_ctx, pub_topics, "1");
		digital_write(relay, 1);
		LOG_INF("Relay State: 1");
	} else if(!strcmp(payload, "0")) {
		rc = publish(&client_ctx, pub_topics, "0");
		digital_write(relay, 0);
		LOG_INF("Relay State: 0");
	}

	return rc;
}

int8_t pub_sub(void)
{
	int8_t rc, r = 0;

	rc = try_to_connect(&client_ctx);
	SUCCESS_OR_EXIT(rc);

	if (connected)
		subscribe(&client_ctx, sub_topics, size_of_pub_topics);

	while (connected) {
		r = -1;

		for(int index=0; index<LIMIT; index++)
			pub_switch_state(&buttons[index], index, pub_topics[index]);

		rc = process_mqtt_and_sleep(&client_ctx, APP_SLEEP_MSECS);
		SUCCESS_OR_BREAK(rc);

		r = 0;
	}

	return r;
}