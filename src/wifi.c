/** Includes *************************************************************************************/
#include "wifi.h"
/** Defines **************************************************************************************/
#define WIFI_CONNECTION_TIMEOUT_MS 5000
#define WIFI_TASK_INTERVAL_MS 100
#define WIFI_SSID_MAX_LENGTH 32
#define WIFI_PASSWORD_MAX_LENGTH 64

/** Typedefs *************************************************************************************/
typedef struct
{
    WifiTaskState_t state;
    uint32_t last_run_ms;
    char ssid[WIFI_SSID_MAX_LENGTH];
    char pw[WIFI_PASSWORD_MAX_LENGTH];
} WifiTask_t;

/** Variables ************************************************************************************/
static WifiTask_t WifiTask = {
    .state = WIFI_TASK_DISCONNECTED,
    .last_run_ms = 0,
    .ssid = {0},
    .pw = {0},
};

/** Prototypes ***********************************************************************************/
/** Functions ************************************************************************************/

int wifi_init(const char *ssid, const char *password)
{
    /** Ensure that the ssid and password are not NULL */
    assert(ssid != NULL);
    assert(password != NULL);

    /** Copy the ssid and password to the task struct */
    memcpy(WifiTask.ssid, ssid, strlen(ssid) + 1);
    WifiTask.ssid[strlen(ssid)] = '\0';

    memcpy(WifiTask.pw, password, strlen(password) + 1);
    WifiTask.pw[strlen(password)] = '\0';

    printf("Initialising Wi-Fi with SSID: %s and password: %s\n", WifiTask.ssid, WifiTask.pw);

    /** Initialise the Wi-Fi chip */
    int rc = cyw43_arch_init();
    if (rc != 0)
    {
        printf("Wi-Fi init failed with rc \n", rc);
        return -1;
    }

    /** Enable wifi station */
    cyw43_arch_enable_sta_mode();

    return 0;
}

int wifi_task(void)
{

    /** Check if the task should run */
    static uint32_t timeLastRunMs = 0;
    uint32_t currentTimeMs = to_ms_since_boot(get_absolute_time());

    // clang-format off
    uint32_t timePassedMs = currentTimeMs < timeLastRunMs
                            ? UINT32_MAX - timeLastRunMs + currentTimeMs
                            : currentTimeMs - timeLastRunMs;
    // clang-format on

    if (timePassedMs < WIFI_TASK_INTERVAL_MS)
    {
        return 0;
    }

    /** Time to run. Update last run */
    timeLastRunMs = currentTimeMs;

    /** Run poll to check if there is new info on the lower driver */
    cyw43_arch_poll();

    /** Get the current wifi status */
    int currentWifiStatus = cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA);

    /** Keep track of connection timeout */
    static uint32_t connectionTimeoutMs = 0;

    switch (WifiTask.state)
    {

    case WIFI_TASK_DISCONNECTED:
        /** WiFi is disconnected let's reconnect */
        char *ssid = WifiTask.ssid;
        char *pw = WifiTask.pw;

        /** Enable station mode again */
        cyw43_arch_enable_sta_mode();

        if (currentWifiStatus != CYW43_LINK_UP)
        {
            /** Try to connect */
            connectionTimeoutMs = WIFI_CONNECTION_TIMEOUT_MS;
            cyw43_arch_wifi_connect_bssid_async(ssid, NULL, pw, CYW43_AUTH_WPA2_AES_PSK);
            printf("Connecting to Wi-Fi\n");
            WifiTask.state = WIFI_TASK_CONNECTING;
        }

        break;

    case WIFI_TASK_CONNECTING:
        /** Check if connected */
        if (currentWifiStatus == CYW43_LINK_UP)
        {
            /** WiFi is connected */
            uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
            printf("Connected to Wi-Fi\n");
            printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
            /** Set the state to connected */
            WifiTask.state = WIFI_TASK_CONNECTED;
        }
        else if (currentWifiStatus == CYW43_LINK_FAIL)
        {
            // Failed to connect
            printf("Failed to connect to Wi-Fi\n");
            cyw43_arch_disable_sta_mode();
            WifiTask.state = WIFI_TASK_DISCONNECTED;
        }
        else if (currentWifiStatus == CYW43_LINK_BADAUTH)
        {
            /** Bad authentication */
            printf("Bad auth\n");
            cyw43_arch_disable_sta_mode();
            WifiTask.state = WIFI_TASK_DISCONNECTED;
        }

        /** Reduce the timeout */
        if (connectionTimeoutMs > 0)
        {
            connectionTimeoutMs -= WIFI_TASK_INTERVAL_MS;
        }
        else
        {
            /** Timeout reached */
            printf("Connection timeout\n");
            /** Reset station mode just incase it gets locked up */
            cyw43_arch_disable_sta_mode();
            WifiTask.state = WIFI_TASK_DISCONNECTED;
        }
        break;

    case WIFI_TASK_CONNECTED:
        /** Check if we are still connected */
        if (currentWifiStatus != CYW43_LINK_UP)
        {
            /** Disconnected */
            printf("Disconnected from Wi-Fi\n");
            cyw43_arch_disable_sta_mode();
            WifiTask.state = WIFI_TASK_DISCONNECTED;
        }
        break;

    default:
        /** Huston we have a problem */
        break;
    }

    return 0;
}

WifiTaskState_t wifi_get_state(void)
{
    return WifiTask.state;
}
