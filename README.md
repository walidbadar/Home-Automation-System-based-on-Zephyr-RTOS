Overview
********
![Alt text](https://github.com/walidbadar/Home-Automation-using-Zephyr-RTOS/blob/main/System-Block-Diagram.png)

MQTT http://mqtt.org (MQ Telemetry Transport) is a lightweight
publish/subscribe messaging protocol optimized for small sensors and
mobile devices.

The Zephyr Home Automation application is a MQTT client, enabling it to both send MQTT PUBLISH messages and receive MQTT SUBSCRIBE messages from the MQTT broker. Additionally, it utilizes GPIO Interrupts capabilities to allow manually switch on/off electrical appliances connected to the device. This application is compatible with MQTT version 3.1.1

MQTT V3.1.1 spec: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/mqtt-v3.1.1.html

Requirements
************

- Linux machine (Raspberry Pi)
- ESP32 Devkit-C
- Home Assistant
- Mosquitto Broker
