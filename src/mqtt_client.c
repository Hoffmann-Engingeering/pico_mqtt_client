/** Includes *************************************************************************************/
#include "mqtt_client.h"

#include "hardware/adc.h"
/** Defines **************************************************************************************/
/** Typedefs *************************************************************************************/
#define MQTT_TEMPERATURE_TOPIC CLIENT_ID "/temperature"
#define MQTT_LED_TOPIC CLIENT_ID "/led"
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/

/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

//
// Created by elliot on 25/05/24.
//
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"






#ifndef DEBUG_printf
#ifndef NDEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif
#endif

#ifndef INFO_printf
#define INFO_printf printf
#endif

#ifndef ERROR_printf
#define ERROR_printf printf
#endif

/* References for this implementation:
 * raspberry-pi-pico-c-sdk.pdf, Section '4.1.1. hardware_adc'
 * pico-examples/adc/adc_console/adc_console.c */
 static float read_onboard_temperature_c(const char unit) {

    /* 12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    return tempC;
}

static void pub_request_cb(__unused void *arg, err_t err)
{
    if (err != 0)
    {
        ERROR_printf("pub_request_cb failed %d", err);
    }
}

static void sub_request_cb(void *arg, err_t err)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    if (err != 0)
    {
        panic("subscribe request failed %d", err);
    }

    INFO_printf("Subscribed to topic\n");
    state->subscribe_count++;
}

static void unsub_request_cb(void *arg, err_t err)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    if (err != 0)
    {
        panic("unsubscribe request failed %d", err);
    }
    state->subscribe_count--;
    assert(state->subscribe_count >= 0);

    // Stop if requested
    if (state->subscribe_count <= 0 && state->stop_client)
    {
        mqtt_disconnect(state->mqttClientInst);
    }
}

static void sub_unsub_topics(MqttClientData_t *state, bool sub)
{
    // Subscribe to topics
    INFO_printf("Subscribing to topics\n");
    mqtt_request_cb_t cb = sub ? sub_request_cb : unsub_request_cb;

    /** TODO: Need to connect one at a time and then verify connected. */
    if( ERR_OK != mqtt_sub_unsub(state->mqttClientInst, "led", MQTT_SUBSCRIBE_QOS, cb, state, sub)) {
        ERROR_printf("Failed to subscribe to topic\n");
    }
    else {
        INFO_printf("Subscribed to topic led\n");
    }
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    /** Need to handle NULL */
    if (data == NULL || len == 0)
    {
        return;
    }

    const char *basic_topic = state->topic;
    strncpy(state->data, (const char *)data, len);
    state->len = len;
    state->data[len] = '\0';

    INFO_printf("Topic: %s, Message: %s\n", state->topic, state->data);
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;
    INFO_printf("Incoming publish topic: %s, length: %d\n", topic, tot_len);
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    MqttClientData_t *state = (MqttClientData_t *)arg;

     if (status == MQTT_CONNECT_ACCEPTED)
     {
        INFO_printf("Connected to mqtt server\n");
        state->connect_done = true;
        sub_unsub_topics(state, true); // subscribe;
    }
    else if (status == MQTT_CONNECT_DISCONNECTED)
    {
        INFO_printf("Disconnected from mqtt server\n");
    }
    else
    {
        INFO_printf("mqtt_connection_cb error %d\n", status);
        
    }
}

static void start_client(MqttClientData_t *state)
{
    INFO_printf("Starting mqtt client\n");
    const int port = MQTT_PORT;
    INFO_printf("Warning: Not using TLS\n");
    /** TODO: CH - Before cleaning out the memory ensure that the mqttClientInst is free */
    if (state->mqttClientInst != NULL)
    {
        mqtt_client_free(state->mqttClientInst);
        state->mqttClientInst = NULL;
    }

    /** Ensure that the client structure is cleaned out */
    memset(state, 0, sizeof(MqttClientData_t));

    state->mqttClientInfo.client_id = CLIENT_ID;          /** See CMakeLists.txt */
    state->mqttClientInfo.keep_alive = 60; // Keep alive in sec
    state->mqttClientInfo.will_topic = "boot";
    state->mqttClientInfo.will_msg = "booted";
    state->mqttClientInfo.will_qos = MQTT_PUBLISH_QOS;
    state->mqttClientInfo.will_retain = MQTT_PUBLISH_RETAIN;


    /** Convert the string IP address to an ip4 address  */
    if(!ipaddr_aton(SERVER_IP, &state->mqtt_server_address)) {
        panic("Failed to convert IP address %s", SERVER_IP);
    }

    state->mqttClientInst = mqtt_client_new();
    if (!state->mqttClientInst)
    {
        panic("MQTT client instance creation error");
    }
    INFO_printf("IP address of this device %s\n", ipaddr_ntoa(&(netif_list->ip_addr)));
    INFO_printf("Connecting to mqtt server at %s\n", ipaddr_ntoa(&state->mqtt_server_address));

    if (mqtt_client_connect(state->mqttClientInst, &state->mqtt_server_address, MQTT_PORT, mqtt_connection_cb, state, &state->mqttClientInfo) != ERR_OK)
    {
        panic("MQTT broker connection error");
    }
    
    INFO_printf("MQTT set callbacks\n");
    mqtt_set_inpub_callback(state->mqttClientInst, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, state);
}

int mqtt_client_task(MqttClientData_t *client)
{

    /** Implement state machine with timeout and rollover protection */
    static uint32_t timeLastRunMs = 0;
    uint32_t currentTimeMs = to_ms_since_boot(get_absolute_time());

    // clang-format off
    uint32_t timePassedMs = currentTimeMs < timeLastRunMs
                          ? UINT32_MAX - timeLastRunMs + currentTimeMs
                          : currentTimeMs - timeLastRunMs;
    // clang-format on

    if (timePassedMs < MQTT_CLIENT_TASK_TIMEOUT_ms)
    {
        return 0;
    }

    /** Update the last run time and catch the roll-over */
    timeLastRunMs = currentTimeMs;

    switch (client->taskState)
    {
    case MQTT_CLIENT_DISCONNECTED:
        start_client(client);
        client->taskState = MQTT_CLIENT_CONNECTING;
        break;
    case MQTT_CLIENT_CONNECTING:
        if (client->connect_done)
        {
            /** We are connected yay */
            client->taskState = MQTT_CLIENT_CONNECTED;
            INFO_printf("MQTT client connected\n");
        }
        break;

    case MQTT_CLIENT_CONNECTED:
        /** Send ON message to the led topic every 5 seconds */
        static int32_t timeout = 5000;
        if (timeout > 0)
        {
            timeout -= MQTT_CLIENT_TASK_TIMEOUT_ms;
        }
        else
        {
            timeout = 5000;
            float temp = read_onboard_temperature_c('C');
            char buffer[20] = {0};
            sprintf(buffer,"{ \"t\": %.2f }", temp);
            INFO_printf("Sending temperature to topic: %s\n", MQTT_TEMPERATURE_TOPIC);
            mqtt_publish(client->mqttClientInst, MQTT_TEMPERATURE_TOPIC, buffer, strlen(buffer), MQTT_PUBLISH_QOS, MQTT_PUBLISH_RETAIN, pub_request_cb, client);
        }

        break;

    default:
        break;
    }

    return 0;
}