#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define DEBOUNCE_US 400000

#define GREEN_TIME_MS  5000
#define YELLOW_TIME_MS 2000
#define RED_TIME_MS    5000
#define EMERG_BLINK_MS 500

const int LED_RED_PIN = 5;
const int LED_YELLOW_PIN = 9;
const int LED_GREEN_PIN = 14;

const int BTN_PED_PIN = 22;
const int BTN_EMERG_PIN = 28;

volatile int btn_ped_flag = 0;
volatile int btn_emerg_flag = 0;

volatile uint64_t last_btn_ped_time = 0;
volatile uint64_t last_btn_emerg_time = 0;

volatile int alarm_green_flag = 0;
volatile int alarm_yellow_flag = 0;
volatile int alarm_red_flag = 0;
volatile int g_timer_0 = 0;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        uint64_t now = time_us_64();
        if (gpio == BTN_PED_PIN && (now - last_btn_ped_time) > DEBOUNCE_US) {
            last_btn_ped_time = now;
            btn_ped_flag = 1;
        } else if (gpio == BTN_EMERG_PIN && (now - last_btn_emerg_time) > DEBOUNCE_US) {
            last_btn_emerg_time = now;
            btn_emerg_flag = 1;
        }
    }
}

int64_t alarm_green_callback(alarm_id_t id, void *user_data) {
    alarm_green_flag = 1;
    return 0;
}

int64_t alarm_yellow_callback(alarm_id_t id, void *user_data) {
    alarm_yellow_flag = 1;
    return 0;
}

int64_t alarm_red_callback(alarm_id_t id, void *user_data) {
    alarm_red_flag = 1;
    return 0;
}

bool timer_0_callback(repeating_timer_t *rt) {
    g_timer_0 = 1;
    return true;
}

void setup() {
    gpio_init(LED_RED_PIN);
    gpio_set_dir(LED_RED_PIN, GPIO_OUT);

    gpio_init(LED_YELLOW_PIN);
    gpio_set_dir(LED_YELLOW_PIN, GPIO_OUT);

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);

    gpio_init(BTN_PED_PIN);
    gpio_set_dir(BTN_PED_PIN, GPIO_IN);
    gpio_pull_up(BTN_PED_PIN);

    gpio_init(BTN_EMERG_PIN);
    gpio_set_dir(BTN_EMERG_PIN, GPIO_IN);
    gpio_pull_up(BTN_EMERG_PIN);

    gpio_set_irq_enabled_with_callback(BTN_PED_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_EMERG_PIN, GPIO_IRQ_EDGE_FALL, true);
}

int main() {
    stdio_init_all();
    setup();

    alarm_id_t alarm;
    repeating_timer_t timer_0;

    int green_on = 1;
    int yellow_on = 0;
    int red_on = 0;
    int emergency = 0;
    int timer_0_running = 0;
    int led_yellow_state = 0;

    // Inicia no verde
    gpio_put(LED_GREEN_PIN, 1);
    gpio_put(LED_YELLOW_PIN, 0);
    gpio_put(LED_RED_PIN, 0);
    alarm = add_alarm_in_ms(GREEN_TIME_MS, alarm_green_callback, NULL, false);

    while (true) {
//---------------------------- EMERGENCIA -------------------------------------
        if (btn_emerg_flag) {
            btn_emerg_flag = 0;

            if (!emergency) {
                cancel_alarm(alarm);
                alarm_green_flag = 0;
                alarm_yellow_flag = 0;
                alarm_red_flag = 0;

                green_on = 0;
                yellow_on = 0;
                red_on = 0;
                emergency = 1;
                led_yellow_state = 0;

                gpio_put(LED_GREEN_PIN, 0);
                gpio_put(LED_YELLOW_PIN, 0);
                gpio_put(LED_RED_PIN, 0);

                add_repeating_timer_ms(EMERG_BLINK_MS, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;
            } else {
                cancel_repeating_timer(&timer_0);
                timer_0_running = 0;
                g_timer_0 = 0;
                emergency = 0;

                red_on = 1;
                gpio_put(LED_GREEN_PIN, 0);
                gpio_put(LED_YELLOW_PIN, 0);
                gpio_put(LED_RED_PIN, 1);
                alarm = add_alarm_in_ms(RED_TIME_MS, alarm_red_callback, NULL, false);
            }
        }

//---------------------------- PEDESTRE -------------------------------------
        if (btn_ped_flag) {
            btn_ped_flag = 0;
            if (green_on && !emergency) {
                cancel_alarm(alarm);
                alarm_green_flag = 0;

                green_on = 0;
                yellow_on = 1;
                gpio_put(LED_GREEN_PIN, 0);
                gpio_put(LED_YELLOW_PIN, 1);
                alarm = add_alarm_in_ms(YELLOW_TIME_MS, alarm_yellow_callback, NULL, false);
            }
        }

//---------------------------- VERDE ACABOU -------------------------------------
        if (alarm_green_flag) {
            alarm_green_flag = 0;
            green_on = 0;
            yellow_on = 1;
            gpio_put(LED_GREEN_PIN, 0);
            gpio_put(LED_YELLOW_PIN, 1);
            alarm = add_alarm_in_ms(YELLOW_TIME_MS, alarm_yellow_callback, NULL, false);
        }

//---------------------------- AMARELO ACABOU -------------------------------------
        if (alarm_yellow_flag) {
            alarm_yellow_flag = 0;
            yellow_on = 0;
            red_on = 1;
            gpio_put(LED_YELLOW_PIN, 0);
            gpio_put(LED_RED_PIN, 1);
            alarm = add_alarm_in_ms(RED_TIME_MS, alarm_red_callback, NULL, false);
        }

//---------------------------- VERMELHO ACABOU -------------------------------------
        if (alarm_red_flag) {
            alarm_red_flag = 0;
            red_on = 0;
            green_on = 1;
            gpio_put(LED_RED_PIN, 0);
            gpio_put(LED_GREEN_PIN, 1);
            alarm = add_alarm_in_ms(GREEN_TIME_MS, alarm_green_callback, NULL, false);
        }

//---------------------------- BLINK EMERGENCIA -------------------------------------
        if (g_timer_0) {
            g_timer_0 = 0;
            if (emergency) {
                led_yellow_state = !led_yellow_state;
                gpio_put(LED_YELLOW_PIN, led_yellow_state);
            }
        }
    }

    return 0;
}