
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"


const int BTN_PIN = 28;

const int LED_PIN_YELLOW = 5;
const int LED_PIN_BLUE = 9;

volatile int alarm_flag_1 = 0;

volatile int g_timer_0 = 0;
volatile int g_timer_1 = 0;

volatile int btn_flag;

int64_t alarm_1_callback(alarm_id_t id, void *user_data) {
    alarm_flag_1 = 1;
    return 0;
}

bool timer_0_callback(repeating_timer_t *rt) {
    g_timer_0= 1;
    return true; // repetir
}
bool timer_1_callback(repeating_timer_t *rt) {
    g_timer_1 = 1;
    return true; // keep repeating
}

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        if (gpio == BTN_PIN) {
            btn_flag= 1;
        }
    }
}

void setup(){
    gpio_init(BTN_PIN);
    gpio_set_dir(BTN_PIN, GPIO_IN);
    gpio_pull_up(BTN_PIN);

    gpio_init(LED_PIN_YELLOW);
    gpio_set_dir(LED_PIN_YELLOW, GPIO_OUT);

    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(BTN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
}


int main() {
    stdio_init_all();
    setup();

    alarm_id_t alarm_1;
    repeating_timer_t timer_0;
    repeating_timer_t timer_1;



    int led_b_state = 0;
    int led_y_state = 0;

    int timer_0_running = 0;
    int timer_1_running = 0;

    while (true) {
        if (btn_flag){
            btn_flag = 0;
            alarm_flag_1 = 0;
            if(!timer_0_running && !timer_1_running){
                alarm_1 = add_alarm_in_ms(5000, alarm_1_callback, NULL, false);
                add_repeating_timer_ms(150, timer_0_callback, NULL, &timer_0);
                add_repeating_timer_ms(500, timer_1_callback, NULL, &timer_1);
                timer_0_running = 1;
                timer_1_running = 1;
            }
        }
        if (alarm_flag_1){
            alarm_flag_1 = 0;
            cancel_repeating_timer(&timer_0);
            cancel_repeating_timer(&timer_1);
            g_timer_0 = 0;
            g_timer_1 = 0;

            led_b_state = 0;
            gpio_put(LED_PIN_BLUE, led_b_state);

            led_y_state = 0;
            gpio_put(LED_PIN_YELLOW, led_y_state);
            timer_0_running = 0;
            timer_1_running = 0;

        }
        if(g_timer_0){
            g_timer_0 = 0;
            led_b_state = !led_b_state;
            gpio_put(LED_PIN_BLUE, led_b_state);
        }
        if(g_timer_1){
            g_timer_1 = 0;
            led_y_state = !led_y_state;
            gpio_put(LED_PIN_YELLOW, led_y_state);
        }
    

    }
}
