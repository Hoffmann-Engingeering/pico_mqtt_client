#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_
/** Includes *************************************************************************************/
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h" // needed to set hostname

/** Defines **************************************************************************************/
#ifndef MQTT_TOPIC_LEN
#define MQTT_TOPIC_LEN 100
#endif

// keep alive in seconds
#define MQTT_KEEP_ALIVE_S 60

// qos passed to mqtt_subscribe
// At most once (QoS 0)
// At least once (QoS 1)
// Exactly once (QoS 2)
#define MQTT_SUBSCRIBE_QOS 1
#define MQTT_PUBLISH_QOS 1
#define MQTT_PUBLISH_RETAIN 0

#define MQTT_CLIENT_TASK_TIMEOUT_ms 100


/** Typedefs *************************************************************************************/

/** MQTT client task states */
typedef enum
{
    MQTT_CLIENT_DISCONNECTED,
    MQTT_CLIENT_CONNECTING,
    MQTT_CLIENT_CONNECTED,
    MQTT_CLIENT_SUBSCRIBED,
} MqttClientState_t;

/** Available topics to push to */
typedef enum 
{
    MQTT_TOPIC_BOOT,
    MQTT_TOPIC_TEMP,
    MQTT_TOPIC_HUMIDITY,
    MQTT_TOPIC_PRESSURE,
    MQTT_TOPIC_MAX
} MqttTopic_t;

/** The client data structure */
typedef struct
{
    mqtt_client_t *mqttClientInst;
    struct mqtt_connect_client_info_t mqttClientInfo;
    char data[MQTT_OUTPUT_RINGBUF_SIZE];
    char topic[MQTT_TOPIC_LEN];
    uint32_t len;
    ip_addr_t mqtt_server_address;
    bool connect_done;
    int subscribe_count;
    bool stop_client;
    MqttTopic_t taskState;
} MqttClientData_t;


/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/
int mqtt_client_task(MqttClientData_t *client);

#endif /* _MQTT_CLIENT_H_ */