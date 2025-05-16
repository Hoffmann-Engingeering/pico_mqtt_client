/** Includes *************************************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/adc.h"

#include "mqtt_client.h"
#include "wifi.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

/** Defines **************************************************************************************/
#ifndef LED_DELAY_MS
#define LED_DELAY_MS 250
#endif

/** Typedefs *************************************************************************************/
/** Variables ************************************************************************************/
/** Prototypes ***********************************************************************************/
int pico_led_init(void);
void pico_set_led(bool led_on);

int led_task(void);

/** Functions ************************************************************************************/

int main()
{
    /** Initialise the stdio library */
    stdio_init_all();

    /** Initial sleep to give the user time to plug in an connect to the COM port */
    sleep_ms(5000);
    adc_init();
    adc_set_temp_sensor_enabled(true);
    adc_select_input(4);

    /** Initialise the LED */
    // TODO: CH - There is a bug in the driver that when running cyw43_arch_init() twice
    // causes a crash. Figure out if we can check if system is already initialised
    // to avoid calling it twice.
    // if (pico_led_init() != PICO_OK)
    // {
    //     printf("Failed to initialise LED\n");
    //     return -1;
    // }

    /** Initialise and connect to the wifi network with 10sec timeout */
    if (wifi_init(SSID, PASSWORD) != 0)
    {
        printf("Failed to initialise Wi-Fi\n");
        return -1;
    }

    printf("Wi-Fi initialised\n");

    /** Initialise the client with the server IP address */
    /** Ensure that the client data is initialised to 0 */
    static MqttClientData_t client = {0};
    // if (client_init(&client, SERVER_IP) != 0)
    // {
    //     printf("Failed to initialise client\n");
    //     return -1;
    // }

    printf("Client initialised\n");

    while (true)
    {
        /** Run the wifi task to check if we are connected */
        wifi_task();

        /**
         * Check if the wifi task state is connected.
         * LED should be on if connected and blinking if not connected.
         */
        if (wifi_get_state() == WIFI_TASK_CONNECTED)
        {
            /** Set the LED on if connected */
            pico_set_led(true);

            /** Run the client task to check if we are connected */
            if (mqtt_client_task(&client) != 0)
            {
                printf("Failed to run client task\n");
            }

            /** Check if we have data */
            // if (client.buffer_len > 0)
            // {
            //     printf("Received data: %s\n", client.buffer);
            //     memset(client.buffer, 0, sizeof(client.buffer)); // Clear the buffer
            //     client.buffer_len = 0;                           // Reset the buffer length after processing
            // }
        }
        else
        {
            /** Set the client task to disconnected */
            client.taskState = MQTT_CLIENT_DISCONNECTED;
            /** Blink the LED if not connected */
            led_task();
        }

        /** Sleep for 10ms */
        sleep_ms(10);
    }
}

/**
 * @brief A simple LED task to blink the LED on and off every LED_DELAY_MS milliseconds.
 * @return int 0 on success, -1 on failure.
 */
int led_task(void)
{
    static bool led_on = false;
    static uint32_t timeLastRunMs = 0;
    uint32_t currentTimeMs = to_ms_since_boot(get_absolute_time());

    if (currentTimeMs - timeLastRunMs < LED_DELAY_MS)
    {
        return 0;
    }
    timeLastRunMs = currentTimeMs;

    led_on = !led_on;
    pico_set_led(led_on);

    return 0;
}

/**
 * @brief Initialise the LED
 * This function is supplied in the Raspberry Pi Pico SDK blinky example code.
 * It is used to initialise the LED. The LED is defined by the PICO_DEFAULT_LED_PIN
 * macro. If this is not defined, then the LED is controlled by the CYW43_WL_GPIO_LED_PIN
 * macro. If this is not defined, then the LED is not controlled by the SDK.
 *
 * @return int 0 on success, -1 on failure.
 * @warning This function should not be called if cyw43_arch_init() has been called.
 */
int pico_led_init(void)
{
#if defined(PICO_DEFAULT_LED_PIN)
    // A device like Pico that uses a GPIO for the LED will define PICO_DEFAULT_LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // For Pico W devices we need to initialise the driver etc
    return cyw43_arch_init();
#endif
}

/**
 * @brief Set the LED on or off
 * This function is supplied in the Raspberry Pi Pico SDK blinky example code.
 * It is used to set the LED on or off. The LED is defined by the PICO_DEFAULT_LED_PIN
 * macro. If this is not defined, then the LED is controlled by the CYW43_WL_GPIO_LED_PIN
 * macro. If this is not defined, then the LED is not controlled by the SDK.
 *
 * @param led_on true to turn the LED on, false to turn it off.
 * @return void
 */
void pico_set_led(bool led_on)
{
#if defined(PICO_DEFAULT_LED_PIN)
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
#elif defined(CYW43_WL_GPIO_LED_PIN)
    // Ask the wifi "driver" to set the GPIO on or off
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
#endif
}
