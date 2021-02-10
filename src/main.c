#include "mgos.h"
#include "mgos_mqtt.h"

#include "main.h"
#include "health_check.h"

/*
 * Global variables
 */
static mgos_timer_id relay_activation_timer = 0;

/**
 * Constants
 */
// Initial state for GPIO pins
const int GPIO_BLUE_LED = 1;
const int GPIO_INPUT_SWITCH = 2;
const int GPIO_OUTPUT_RELAY = 0;
// The ESP-Relay/01s V4.0 uses a Low signal for relay activation
const int GPIO_RELAY_ACTIVATION_SIGNAL = 0;

/**
 * Callback for button press
 */
static void button_cb(__attribute__((unused)) int pin, __attribute__((unused)) void *arg) {
	char topic[100];
	char *message = "{\"event\":\"button_press\"}";

	snprintf(topic, sizeof(topic), "devices/%s/%s",
		mgos_sys_config_get_device_id(),
		mgos_sys_config_get_app_mqtt_button_topic());

	// Publishes the event to MQTT
	bool res = mgos_mqtt_pub(topic, message, strlen(message), 1, false);

	// Logs the result
	LOG(LL_INFO, ("Button pressed - Published: %s", res ? "yes" : "no"));
}

static void turnoff_relay_cb(__attribute__((unused)) void *arg) {
	LOG(LL_INFO, ("Timer reached"));
	mgos_clear_timer(relay_activation_timer);
	relay_activation_timer = 0;
	LOG(LL_INFO, ("Timer finished"));

	mgos_gpio_write(GPIO_OUTPUT_RELAY, !GPIO_RELAY_ACTIVATION_SIGNAL);
}

/**
 * Callback handler for messages sent to the configured MQTT topic
 */
static void message_handler_cb(__attribute__((unused)) struct mg_connection *conn,
		int ev, __attribute__((unused)) void *ev_data, __attribute__((unused)) void *user_data) {
	if (ev != MG_EV_MQTT_PUBLISH) {
		return;
	}

	LOG(LL_INFO, ("Relay event received"));

	// Unbounce the delay
	if (relay_activation_timer != 0) {
		LOG(LL_INFO, ("Timer still active. Bouncing event..."));
		return;
	}

	mgos_gpio_write(GPIO_OUTPUT_RELAY, GPIO_RELAY_ACTIVATION_SIGNAL);

	// Then deactivates the relay
	relay_activation_timer = mgos_set_timer(
			mgos_sys_config_get_app_relay_time(),
			MGOS_TIMER_REPEAT, turnoff_relay_cb, NULL
	);
}

/**
 * Subscribe to the configured MQTT topic to listen for messages and trigger
 * the configured GPIO port
 */
static void setup_mqtt_subscription() {
	char topic[100];
	snprintf(topic, sizeof(topic), "devices/%s/%s",
		mgos_sys_config_get_device_id(),
		mgos_sys_config_get_app_mqtt_relay_topic());

	// Listen for MQTT events and activate GPIO
	mgos_mqtt_global_subscribe(mg_mk_str(topic), message_handler_cb, NULL);
}

/**
 * Init (main) function
 */
enum mgos_app_init_result mgos_app_init(void) {
	// Set the relay pin to output mode
	mgos_gpio_set_mode(GPIO_OUTPUT_RELAY, MGOS_GPIO_MODE_OUTPUT);

	// Set a timer to health check function
	mgos_set_timer(1000, MGOS_TIMER_REPEAT, health_check, NULL);

	// Publish to MQTT on button press
	mgos_gpio_set_button_handler(
			GPIO_INPUT_SWITCH, MGOS_GPIO_PULL_UP, MGOS_GPIO_INT_EDGE_NEG, 50, button_cb, NULL
	);

	// Subscribe for MQTT events and activate GPIO
	setup_mqtt_subscription();

	// Disable UART if requested -- that allows us to use the blue led on ESP-01
	// To disable MQTT Logging, set it on `fs/conf3.json`
	if (!mgos_sys_config_get_app_console()) {
		mgos_gpio_set_mode(GPIO_BLUE_LED, MGOS_GPIO_MODE_OUTPUT);
		mgos_debug_suspend_uart();
	}

	return MGOS_APP_INIT_SUCCESS;
}

/**
 * Do things very early in the boot process
 */
void mgos_app_preinit() {
	// GPIO 0 needs to receive a high signal on boot so ESP8266 boots the application. This might
	// conflict with applications that depends on this port, so we try, as early as possible in the
	// boot time to set the signal to the desired one so we don't face issues like relays flickering
	mgos_gpio_write(GPIO_OUTPUT_RELAY, !GPIO_RELAY_ACTIVATION_SIGNAL);
}
