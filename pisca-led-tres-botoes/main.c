#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"

#define DEBOUNCE_US 400000  // 400ms

const int BTN_YELLOW_PIN = 28;
const int BTN_GREEN_PIN = 18;
const int BTN_BLUE_PIN = 22;

const int LED_YELLOW_PIN = 6;
const int LED_BLUE_PIN = 10;
const int LED_GREEN_PIN = 14;

volatile int btn_y_flag_r = 0;
volatile int btn_g_flag_r = 0;
volatile int btn_b_flag_r = 0;

volatile int btn_y_flag_f = 0;
volatile int btn_g_flag_f = 0;
volatile int btn_b_flag_f = 0;

volatile uint64_t last_btn_green_time = 0;
volatile uint64_t last_btn_yellow_time = 0;
volatile uint64_t last_btn_blue_time = 0;

volatile int g_timer_0 = 0;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        uint64_t now = time_us_64();
        if (gpio == BTN_GREEN_PIN) {
            last_btn_green_time = now;
            btn_g_flag_f = 1;
        } else if (gpio == BTN_YELLOW_PIN && (now - last_btn_yellow_time) > DEBOUNCE_US) {
            last_btn_yellow_time = now;
            btn_y_flag_f = 1;
        } else if (gpio == BTN_BLUE_PIN && (now - last_btn_blue_time) > DEBOUNCE_US) {
            last_btn_blue_time = now;
            btn_b_flag_f = 1;
        }
    } else if(events == 0x8){ // rise edge
        uint64_t now = time_us_64();
        if (gpio == BTN_GREEN_PIN) {
            last_btn_green_time = now;
            btn_g_flag_r = 1;
        } else if (gpio == BTN_YELLOW_PIN) {
            last_btn_yellow_time = now;
            btn_y_flag_r = 1;
        } else if (gpio == BTN_BLUE_PIN) {
            last_btn_blue_time = now;
            btn_b_flag_r = 1;
        }
    }
}

bool timer_0_callback(repeating_timer_t *rt) {
    g_timer_0= 1;
    return true; // repetir
}

void setup(){
    gpio_init(BTN_GREEN_PIN);
    gpio_set_dir(BTN_GREEN_PIN, GPIO_IN);
    gpio_pull_up(BTN_GREEN_PIN);

    gpio_init(BTN_YELLOW_PIN);
    gpio_set_dir(BTN_YELLOW_PIN, GPIO_IN);
    gpio_pull_up(BTN_YELLOW_PIN);

    gpio_init(BTN_BLUE_PIN);
    gpio_set_dir(BTN_BLUE_PIN, GPIO_IN);
    gpio_pull_up(BTN_BLUE_PIN);

    gpio_init(LED_YELLOW_PIN);
    gpio_set_dir(LED_YELLOW_PIN, GPIO_OUT);

    gpio_init(LED_GREEN_PIN);
    gpio_set_dir(LED_GREEN_PIN, GPIO_OUT);

    gpio_init(LED_BLUE_PIN);
    gpio_set_dir(LED_BLUE_PIN, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(BTN_GREEN_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);
    gpio_set_irq_enabled(BTN_YELLOW_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);
    gpio_set_irq_enabled(BTN_BLUE_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true);

}


int main() {
  stdio_init_all();
  setup();
  repeating_timer_t timer_0;

  int led_state = 0;

  int green_active = 0;
  int blue_active = 0;
  int yellow_active = 0;

  int timer_0_running = 0;

  while (true) {
    if(btn_y_flag_f){
      btn_y_flag_f = 0;
      yellow_active = !yellow_active;
      if(!yellow_active){
        int led_state_y = 0;
        gpio_put(LED_YELLOW_PIN, led_state_y);
      }else if(!timer_0_running){
        add_repeating_timer_ms(200, timer_0_callback, NULL, &timer_0);
        timer_0_running = 1;
      }
    }



    if(btn_b_flag_r){
      btn_b_flag_r = 0;
      blue_active = !blue_active;
      if(!blue_active){
        int led_state_b = 0;
        gpio_put(LED_BLUE_PIN, led_state_b);
      } else if(!timer_0_running){
        add_repeating_timer_ms(200, timer_0_callback, NULL, &timer_0);
        timer_0_running = 1;
      }
    }
  
    if(btn_g_flag_f){
      btn_g_flag_f = 0;
      green_active = 1;
      if(!timer_0_running){
        add_repeating_timer_ms(200, timer_0_callback, NULL, &timer_0);
        timer_0_running = 1;
      }
    }

    if(btn_g_flag_r){
      btn_g_flag_r = 0;
      green_active = 0;
      int led_state_g = 0;
      gpio_put(LED_GREEN_PIN, led_state_g);

    }

    if(g_timer_0){
      g_timer_0 = 0;
      led_state = !led_state;
      if(yellow_active){
        gpio_put(LED_YELLOW_PIN, led_state);
      }
      if(green_active){
        gpio_put(LED_GREEN_PIN, led_state);
      }
      if(blue_active){
        gpio_put(LED_BLUE_PIN, led_state);
      }
    }
  }
}