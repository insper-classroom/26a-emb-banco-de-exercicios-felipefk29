#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"

const int LED_PIN_Y = 10;
const int LED_PIN_B = 14;

const int BTN_PIN_Y = 26;
const int BTN_PIN_B = 19;

volatile int btn_y_flag;
volatile int btn_b_flag;

volatile int g_timer_0 = 0;
volatile int g_timer_1 = 0;

volatile int alarm_flag_1 = 0;
volatile int alarm_flag_2 = 0;

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
        if (gpio == BTN_PIN_B) {
            btn_b_flag = 1;
        } else if (gpio == BTN_PIN_Y) {
            btn_y_flag = 1;
        }
    }
}

void setup(){
    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);

    gpio_init(BTN_PIN_B);
    gpio_set_dir(BTN_PIN_B, GPIO_IN);
    gpio_pull_up(BTN_PIN_B);

    gpio_init(LED_PIN_Y);
    gpio_set_dir(LED_PIN_Y, GPIO_OUT);

    gpio_init(LED_PIN_B);
    gpio_set_dir(LED_PIN_B, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(BTN_PIN_B, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);
}

int main()
{
    stdio_init_all();
    setup();
    alarm_id_t alarm_1;
    alarm_id_t alarm_2;
    repeating_timer_t timer_0;
    repeating_timer_t timer_1;

    int led_state_b = 0;
    int led_state_y = 0;

    int btn_y_pressed = 0;
    int btn_b_pressed = 0;

    int timer_0_running = 0;
    int timer_1_running = 0;

    int sequence_running = 0;

    while (1) {
//---------------------- AMARELO ------------------------------
        if(btn_y_flag){
            alarm_flag_1 = 0;
            btn_y_flag = 0;
            if (!timer_0_running && alarm_flag_1 == 0 && !sequence_running){
                btn_y_pressed = 1;
                sequence_running = 1;
                alarm_1 = add_alarm_in_ms(1000, alarm_1_callback, NULL, false);
                add_repeating_timer_ms(100, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;
            } 
        }
        if(alarm_flag_1){
            alarm_flag_1 = 0;
            cancel_repeating_timer(&timer_0);
            g_timer_0 = 0;
            led_state_y = 0;
            gpio_put(LED_PIN_Y, led_state_y);
            timer_0_running = false;
            if(btn_y_pressed){
                btn_y_pressed = 0;
                alarm_2 = add_alarm_in_ms(2000, alarm_2_callback, NULL, false);
                add_repeating_timer_ms(250, timer_1_callback, NULL, &timer_1);
                timer_1_running = 1;
            } else {
                sequence_running = 0;
            }
        }

        if(btn_b_flag){
            btn_b_flag = 0;
            alarm_flag_2 = 0;
            if (!timer_1_running && alarm_flag_2 == 0 && !sequence_running){
                btn_b_pressed = 1;
                sequence_running =1;
                alarm_2 = add_alarm_in_ms(2000, alarm_2_callback, NULL, false);
                add_repeating_timer_ms(250, timer_1_callback, NULL, &timer_1);
                timer_1_running = 1;
            } 
        }  
        
        if(alarm_flag_2){
            alarm_flag_2 = 0;
            cancel_repeating_timer(&timer_1);
            g_timer_1 =0;
            led_state_b = 0;
            gpio_put(LED_PIN_B, led_state_b);
            timer_1_running = false;
            if(btn_b_pressed){
                btn_b_pressed = 0;
                alarm_1 = add_alarm_in_ms(1000, alarm_1_callback, NULL, false);
                add_repeating_timer_ms(100, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;
            } else {
                sequence_running = 0;
            }
        }
  
        if(g_timer_0){
            g_timer_0 = 0;
            led_state_y = !led_state_y;
            gpio_put(LED_PIN_Y, led_state_y);
        }
        if(g_timer_1){
            g_timer_1 = 0;
            led_state_b = !led_state_b;
            gpio_put(LED_PIN_B, led_state_b);
        }
    }

    
        
    

    return 0;
}