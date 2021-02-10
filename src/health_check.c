#include "mgos.h"
#include "mgos_mqtt.h"

#include "main.h"
#include "health_check.h"

/*
 * Global variables
 */
static mgos_timer_id health_check_timer = 0;

/**
 * Callback for led looping
 */
static void led_timer_cb(__attribute__((unused)) void *arg) {
  mgos_gpio_toggle(GPIO_BLUE_LED);
}

static void start_blinking() {
	// We disable the warning led if console is on. That's because we use the same
	// GPIO port for both UART (console) communication and the led output. Using
	// both is incompatible with this board (ESP-01).
  if (mgos_sys_config_get_app_console()) {
    return;
  }

  if (health_check_timer != 0) {
    return;
  }

  health_check_timer = mgos_set_timer(300, MGOS_TIMER_REPEAT, led_timer_cb, NULL);
}

static void stop_blinking() {
	// We disable the warning led if console is on. That's because we use the same
	// GPIO port for both UART (console) communication and the led output. Using
	// both is incompatible with this board (ESP-01).
  if (mgos_sys_config_get_app_console()) {
    return;
  }

  mgos_clear_timer(health_check_timer);
  health_check_timer = 0;
  mgos_gpio_write(GPIO_BLUE_LED, true);
}

/**
 * Get a formatted uptime string
 */
static void get_formatted_uptime(char *string, size_t size) {
  int seconds = (int) mgos_uptime();
  int days = seconds / 86400;
  seconds = seconds % 86400;
  int hours = seconds / 3600;
  seconds = seconds % 3600;
  int minutes = seconds / 60;
  seconds = seconds % 60;

  snprintf(string, size, "%d days, %.2d:%.2d:%.2d",
           days, hours, minutes, seconds);
}

/**
 * Health check function
 *
 * This function will check for the vital signs of the system and if something
 * is not right, it will make the led blink
 */
void health_check(__attribute__((unused)) void *arg) {
  bool wifi_connected = mgos_wifi_get_status() == MGOS_WIFI_IP_ACQUIRED;
  bool mqtt_connected = mgos_mqtt_global_is_connected();
  char uptime[50];

  get_formatted_uptime(uptime, sizeof(uptime));

  LOG(LL_INFO, ("Uptime: %s | Wi-Fi: %s | MQTT: %s | RAM: %.1f%% used",
        uptime,
        wifi_connected ? "Yes" : "No",
        mqtt_connected ? "Yes" : "No",
        (float) mgos_get_free_heap_size() / (float) mgos_get_heap_size()));

  if (!wifi_connected || !mqtt_connected) {
    // Start blinking the LED to indicate that something is wrong
    start_blinking();
  } else {
    stop_blinking();
  }
}
