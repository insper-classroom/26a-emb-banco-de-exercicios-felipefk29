#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define DEBOUNCE_US 400000

#define STATE_GREEN   0
#define STATE_BLINK   1  // verde piscando antes de virar amarelo
#define STATE_YELLOW  2
#define STATE_RED     3

#define GREEN_TIME_MS  5000
#define BLINK_TIME_MS  2000  // tempo total piscando
#define BLINK_RATE_MS  200   // frequencia do piscar
#define YELLOW_TIME_MS 2000
#define RED_TIME_MS    5000

const int LED_RED_PIN = 5;
const int LED_YELLOW_PIN = 9;
const int LED_GREEN_PIN = 14;

const int BTN_PED_PIN = 22;

volatile int btn_ped_flag = 0;
volatile uint64_t last_btn_ped_time = 0;
volatile int alarm_flag = 0;
volatile int g_timer_0 = 0;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        uint64_t now = time_us_64();
        if (gpio == BTN_PED_PIN && (now - last_btn_ped_time) > DEBOUNCE_US) {
            last_btn_ped_time = now;
            btn_ped_flag = 1;
        }
    }
}

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    alarm_flag = 1;
    return 0;
}

bool timer_0_callback(repeating_timer_t *rt) {
    g_timer_0 = 1;
    return true;
}

void set_leds(int r, int y, int g) {
    gpio_put(LED_RED_PIN, r);
    gpio_put(LED_YELLOW_PIN, y);
    gpio_put(LED_GREEN_PIN, g);
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

    gpio_set_irq_enabled_with_callback(BTN_PED_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
}

int main() {
    stdio_init_all();
    setup();

    alarm_id_t alarm;
    repeating_timer_t timer_0;

    int state = STATE_GREEN;
    int timer_0_running = 0;
    int led_green_state = 0;

    set_leds(0, 0, 1);
    alarm = add_alarm_in_ms(GREEN_TIME_MS, alarm_callback, NULL, false);

    while (true) {
//---------------------------- PEDESTRE -------------------------------------
        if (btn_ped_flag) {
            btn_ped_flag = 0;
            if (state == STATE_GREEN) {
                // Pula direto pro verde piscando
                cancel_alarm(alarm);
                alarm_flag = 0;

                state = STATE_BLINK;
                led_green_state = 1;
                add_repeating_timer_ms(BLINK_RATE_MS, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;
                alarm = add_alarm_in_ms(BLINK_TIME_MS, alarm_callback, NULL, false);
            }
        }

//---------------------------- TRANSICOES -------------------------------------
        if (alarm_flag) {
            alarm_flag = 0;

            if (state == STATE_GREEN) {
                // Verde acabou -> verde piscando
                state = STATE_BLINK;
                led_green_state = 1;
                add_repeating_timer_ms(BLINK_RATE_MS, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;
                alarm = add_alarm_in_ms(BLINK_TIME_MS, alarm_callback, NULL, false);

            } else if (state == STATE_BLINK) {
                // Piscar acabou -> amarelo
                cancel_repeating_timer(&timer_0);
                timer_0_running = 0;
                g_timer_0 = 0;

                state = STATE_YELLOW;
                set_leds(0, 1, 0);
                alarm = add_alarm_in_ms(YELLOW_TIME_MS, alarm_callback, NULL, false);

            } else if (state == STATE_YELLOW) {
                state = STATE_RED;
                set_leds(1, 0, 0);
                alarm = add_alarm_in_ms(RED_TIME_MS, alarm_callback, NULL, false);

            } else if (state == STATE_RED) {
                state = STATE_GREEN;
                set_leds(0, 0, 1);
                alarm = add_alarm_in_ms(GREEN_TIME_MS, alarm_callback, NULL, false);
            }
        }

//---------------------------- BLINK VERDE -------------------------------------
        if (g_timer_0) {
            g_timer_0 = 0;
            if (state == STATE_BLINK) {
                led_green_state = !led_green_state;
                gpio_put(LED_GREEN_PIN, led_green_state);
            }
        }
    }

    return 0;
}