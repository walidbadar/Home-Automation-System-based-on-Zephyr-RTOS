Overview
********
![Alt text](https://github.com/walidbadar/Home-Automation-using-Zephyr-RTOS/blob/main/Home-Automation.png)

`MQTT <http://mqtt.org/>`_ (MQ Telemetry Transport) is a lightweight
publish/subscribe messaging protocol optimized for small sensors and
mobile devices.

The Zephyr Home Automation application is a MQTT client 
that sends MQTT PUBLISH messages to a MQTT broker and 
receives MQTT SUBSCRIBE messages from an MQTT broker. 
It also uses GPIO to manually toggle electrical appliances
connected to the device. This application supports MQTT v3.1.1.
See the `MQTT V3.1.1 spec`_ for more information.

.. _MQTT V3.1.1 spec: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html

Requirements
************

- Linux machine (Raspberry Pi)
- ESP32 Devkit-C
- Home Assistant
- Mosquitto server: any version that supports MQTT v3.1.1. This sample
  was tested with mosquitto 1.3.4.

Build and Running
*****************

Currently, this sample application only supports static IP addresses.
Open the :file:`src/config.h` file and set the IP addresses according
to the LAN environment.
Alternatively, set the IP addresses in the :file:`prj.conf` file.

The file :file:`src/config.h` also contains some variables that may be changed:

MQTT broker TCP port:

.. code-block:: c

	#define SERVER_PORT		1883

Application sleep time:

.. code-block:: c

	#define APP_SLEEP_MSECS		500

Max number of connection tries:

.. code-block:: c

	#define APP_CONNECT_TRIES	10

MQTT Client Identifier:

.. code-block:: c

	#define MQTT_CLIENTID		"zephyr_publisher"
