#include <stdio.h>

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"

#define DEBOUNCE_US 400000  // 400ms


const int BTN_YELLOW_PIN = 14;
const int BTN_GREEN_PIN = 13;

const int LED_GREEN_PIN = 18;
const int LED_YELLOW_PIN = 17;

volatile int btn_green_flag;
volatile int btn_yellow_flag;


volatile int alarm_flag_1 = 0;
volatile int alarm_flag_2 = 0;

volatile int g_timer_0 = 0;
volatile int g_timer_1 = 0;

volatile uint64_t last_btn_green_time = 0;
volatile uint64_t last_btn_yellow_time = 0;





int64_t alarm_1_callback(alarm_id_t id, void *user_data) {
    alarm_flag_1 = 1;
    return 0;
}

int64_t alarm_2_callback(alarm_id_t id, void *user_data) {
    alarm_flag_2 = 1;
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
        uint64_t now = time_us_64();
        if (gpio == BTN_GREEN_PIN && (now - last_btn_green_time) > DEBOUNCE_US) {
            btn_green_flag = 1;
            last_btn_green_time = now;
        } else if (gpio == BTN_YELLOW_PIN && (now - last_btn_yellow_time) > DEBOUNCE_US) {
            last_btn_yellow_time = now;
            btn_yellow_flag = 1;
        }
    }
}


void setup(){
    gpio_init(BTN_GREEN_PIN);
    gpio_set_dir(BTN_GREEN_PIN, GPIO_IN);
    gpio_pull_up(BTN_GREEN_PIN);

    gpio_init(BTN_YELLOW_PIN);
    gpio_set_dir(BTN_YELLOW_PIN, GPIO_IN);
    gpio_pull_up(BTN_YELLOW_PIN);

    gpio_init(LED_YELLOW_PIN);
    gpio_set_dir(LED_YELLOW_PIN, GPIO_OUT);

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(BTN_GREEN_PIN, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_YELLOW_PIN, GPIO_IRQ_EDGE_FALL, true);
}

int main() {
    stdio_init_all();
    setup();
    alarm_id_t alarm_1;
    alarm_id_t alarm_2;
    repeating_timer_t timer_0;
    repeating_timer_t timer_1;

    int led_state_g = 0;
    int led_state_y = 0;

    int timer_0_running = 0;
    int timer_1_running = 0;


    while (1) {
//---------------------------- VERDE -------------------------------------
        if(btn_green_flag){
            alarm_flag_1 = 0;
            btn_green_flag =0;
            if (!timer_0_running && alarm_flag_1 == 0){
                alarm_1 = add_alarm_in_ms(1000, alarm_1_callback, NULL, false);
                add_repeating_timer_ms(200, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;
            } 
        }
        if(alarm_flag_1){
            if(timer_0_running && timer_1_running){
                led_state_y = 0;
                cancel_repeating_timer(&timer_1);
                cancel_alarm(alarm_2);
                timer_1_running = false;
                alarm_flag_2 = 0;
                gpio_put(LED_YELLOW_PIN, led_state_y);
            }
            cancel_repeating_timer(&timer_0);
            timer_0_running = false;
            led_state_g = 0;
            alarm_flag_1 = 0;
            gpio_put(LED_GREEN_PIN, led_state_g);
        }

//----------------------------- AMARELO --------------------------------------


        if(btn_yellow_flag){
            alarm_flag_2 = 0;
            btn_yellow_flag =0;
            if (!timer_1_running && alarm_flag_2 == 0){
                alarm_2 = add_alarm_in_ms(2000, alarm_2_callback, NULL, false);
                add_repeating_timer_ms(500, timer_1_callback, NULL, &timer_1);
                timer_1_running = 1;
            } 
        }
        if(alarm_flag_2){
            if(timer_0_running && timer_1_running){
                led_state_g = 0;
                alarm_flag_1 = 0;
                cancel_repeating_timer(&timer_0);
                cancel_alarm(alarm_1);
                timer_0_running = false;
                gpio_put(LED_GREEN_PIN, led_state_g);
            }
            cancel_repeating_timer(&timer_1);
            timer_1_running = false;
            led_state_y = 0;
            alarm_flag_2 = 0;
            gpio_put(LED_YELLOW_PIN, led_state_y);
        }

        if(g_timer_0){
            g_timer_0 = 0;
            led_state_g = !led_state_g;
            gpio_put(LED_GREEN_PIN, led_state_g);
        }
        if(g_timer_1){
            g_timer_1 = 0;
            led_state_y = !led_state_y;
            gpio_put(LED_YELLOW_PIN, led_state_y);
        }
    }

    return 0;
}
