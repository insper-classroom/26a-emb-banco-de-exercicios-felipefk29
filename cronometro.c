#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define DEBOUNCE_US 400000

const int BTN_PIN_WHITE = 22;
const int BTN_PIN_YELLOW = 26;
const int BTN_PIN_RED = 28;

const int LED_PIN_WHITE = 5;
const int LED_PIN_YELLOW = 9;
const int LED_PIN_RED = 14;

volatile int btn_white_flag = 0;
volatile int btn_yellow_flag = 0;
volatile int btn_red_flag = 0;

volatile uint64_t last_btn_white_time = 0;
volatile uint64_t last_btn_yellow_time = 0;
volatile uint64_t last_btn_red_time = 0;

volatile int g_timer_0 = 0;
volatile int alarm_flag_30 = 0;
volatile int alarm_flag_red = 0;

int64_t alarm_30_callback(alarm_id_t id, void *user_data) {
    alarm_flag_30 = 1;
    return 0;
}

int64_t alarm_red_callback(alarm_id_t id, void *user_data) {
    alarm_flag_red = 1;
    return 0;
}

bool timer_0_callback(repeating_timer_t *rt) {
    g_timer_0 = 1;
    return true;
}

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        uint64_t now = time_us_64();
        if (gpio == BTN_PIN_WHITE && (now - last_btn_white_time) > DEBOUNCE_US) {
            last_btn_white_time = now;
            btn_white_flag = 1;
        } else if (gpio == BTN_PIN_YELLOW && (now - last_btn_yellow_time) > DEBOUNCE_US) {
            last_btn_yellow_time = now;
            btn_yellow_flag = 1;
        } else if (gpio == BTN_PIN_RED && (now - last_btn_red_time) > DEBOUNCE_US) {
            last_btn_red_time = now;
            btn_red_flag = 1;
        }
    }
}

void all_leds_off(void) {
    gpio_put(LED_PIN_WHITE, 0);
    gpio_put(LED_PIN_YELLOW, 0);
    gpio_put(LED_PIN_RED, 0);
}

void setup(void) {
    gpio_init(BTN_PIN_WHITE);
    gpio_set_dir(BTN_PIN_WHITE, GPIO_IN);
    gpio_pull_up(BTN_PIN_WHITE);

    gpio_init(BTN_PIN_YELLOW);
    gpio_set_dir(BTN_PIN_YELLOW, GPIO_IN);
    gpio_pull_up(BTN_PIN_YELLOW);

    gpio_init(BTN_PIN_RED);
    gpio_set_dir(BTN_PIN_RED, GPIO_IN);
    gpio_pull_up(BTN_PIN_RED);

    gpio_init(LED_PIN_WHITE);
    gpio_set_dir(LED_PIN_WHITE, GPIO_OUT);

    gpio_init(LED_PIN_YELLOW);
    gpio_set_dir(LED_PIN_YELLOW, GPIO_OUT);

    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    gpio_set_irq_enabled_with_callback(BTN_PIN_WHITE, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_YELLOW, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_PIN_RED, GPIO_IRQ_EDGE_FALL, true);
}

int main() {
    stdio_init_all();
    setup();

    repeating_timer_t timer_0;
    alarm_id_t alarm_30;

    int timer_0_running = 0;
    int led_white_state = 0;
    int seconds = 0;

    // 0=idle, 1=running, 2=paused, 3=stopped
    int running = 0;
    int paused = 0;

    printf("Cronometro pronto. Aperte o botao branco para iniciar.\n");

    while (true) {
//---------------------------- INICIAR (BRANCO) -------------------------------------
        if (btn_white_flag) {
            btn_white_flag = 0;
            if (!running && !paused) {
                running = 1;
                paused = 0;
                seconds = 0;
                led_white_state = 0;
                alarm_flag_30 = 0;
                alarm_flag_red = 0;

                all_leds_off();

                add_repeating_timer_ms(1000, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;
                alarm_30 = add_alarm_in_ms(30000, alarm_30_callback, NULL, false);

                printf("Cronometro iniciado!\n");
            }
        }

//---------------------------- PAUSAR (AMARELO) -------------------------------------
        if (btn_yellow_flag) {
            btn_yellow_flag = 0;
            if (running && !paused) {
                // Pausa
                paused = 1;
                cancel_repeating_timer(&timer_0);
                timer_0_running = 0;
                g_timer_0 = 0;

                gpio_put(LED_PIN_WHITE, 0);
                led_white_state = 0;
                gpio_put(LED_PIN_YELLOW, 1);

                printf("Pausado em %ds\n", seconds);

            } else if (running && paused) {
                // Retoma
                paused = 0;
                gpio_put(LED_PIN_YELLOW, 0);

                add_repeating_timer_ms(1000, timer_0_callback, NULL, &timer_0);
                timer_0_running = 1;

                printf("Retomado em %ds\n", seconds);
            }
        }

//---------------------------- PARAR (VERMELHO) -------------------------------------
        if (btn_red_flag) {
            btn_red_flag = 0;
            if (running) {
                // Cancela tudo
                if (timer_0_running) {
                    cancel_repeating_timer(&timer_0);
                    timer_0_running = 0;
                    g_timer_0 = 0;
                }
                cancel_alarm(alarm_30);
                alarm_flag_30 = 0;

                running = 0;
                paused = 0;

                all_leds_off();
                gpio_put(LED_PIN_RED, 1);
                add_alarm_in_ms(5000, alarm_red_callback, NULL, false);

                printf("Parado em %ds\n", seconds);
            }
        }

//---------------------------- ALARME 30s (TEMPO ESGOTADO) -------------------------------------
        if (alarm_flag_30) {
            alarm_flag_30 = 0;
            if (running) {
                if (timer_0_running) {
                    cancel_repeating_timer(&timer_0);
                    timer_0_running = 0;
                    g_timer_0 = 0;
                }

                running = 0;
                paused = 0;

                all_leds_off();
                gpio_put(LED_PIN_RED, 1);
                add_alarm_in_ms(5000, alarm_red_callback, NULL, false);

                printf("Tempo esgotado! 30s\n");
            }
        }

//---------------------------- ALARME LED VERMELHO (5s) -------------------------------------
        if (alarm_flag_red) {
            alarm_flag_red = 0;
            gpio_put(LED_PIN_RED, 0);
            printf("Cronometro pronto. Aperte o botao branco para iniciar.\n");
        }

//---------------------------- TIMER 1s (BLINK + CONTAGEM) -------------------------------------
        if (g_timer_0) {
            g_timer_0 = 0;
            if (running && !paused) {
                seconds++;
                led_white_state = !led_white_state;
                gpio_put(LED_PIN_WHITE, led_white_state);
                printf("%ds\n", seconds);
            }
        }
    }

    return 0;
}