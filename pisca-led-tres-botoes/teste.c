/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#define DEBOUNCE_US 400000  // 400ms

const int BTN_PIN_BRANCO = 15;
const int BTN_PIN_AMARELO = 14;
const int BTN_PIN_VERDE = 13;

const int LED_PIN_BRANCO = 16;
const int LED_PIN_AMARELO = 17;
const int LED_PIN_VERDE = 18;

// -1 = nenhum botão, 1 = branco, 2 = amarelo, 3 = verde
volatile int btn_pressed = -1;

volatile uint64_t last_btn_branco_time = 0;
volatile uint64_t last_btn_amarelo_time = 0;
volatile uint64_t last_btn_verde_time = 0;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) {
        uint64_t now = time_us_64();
        if (gpio == BTN_PIN_BRANCO && (now - last_btn_branco_time) > DEBOUNCE_US) {
            last_btn_branco_time = now;
            btn_pressed = 1;
        } else if (gpio == BTN_PIN_AMARELO && (now - last_btn_amarelo_time) > DEBOUNCE_US) {
            last_btn_amarelo_time = now;
            btn_pressed = 2;
        } else if (gpio == BTN_PIN_VERDE && (now - last_btn_verde_time) > DEBOUNCE_US) {
            last_btn_verde_time = now;
            btn_pressed = 3;
        }
    }
}

// Apaga todos os LEDs
void all_leds_off(void) {
    gpio_put(LED_PIN_BRANCO, 0);
    gpio_put(LED_PIN_AMARELO, 0);
    gpio_put(LED_PIN_VERDE, 0);
}

// Acende o LED correspondente por 500ms
void feedback_botao(int cor) {
    switch (cor) {
        case 1: gpio_put(LED_PIN_BRANCO, 1); break;
        case 2: gpio_put(LED_PIN_AMARELO, 1); break;
        case 3: gpio_put(LED_PIN_VERDE, 1); break;
    }
    sleep_ms(500);
    all_leds_off();
}

void setup(void) {
    // Botões
    gpio_init(BTN_PIN_BRANCO);
    gpio_set_dir(BTN_PIN_BRANCO, GPIO_IN);
    gpio_pull_up(BTN_PIN_BRANCO);

    gpio_init(BTN_PIN_AMARELO);
    gpio_set_dir(BTN_PIN_AMARELO, GPIO_IN);
    gpio_pull_up(BTN_PIN_AMARELO);

    gpio_init(BTN_PIN_VERDE);
    gpio_set_dir(BTN_PIN_VERDE, GPIO_IN);
    gpio_pull_up(BTN_PIN_VERDE);

    // LEDs
    gpio_init(LED_PIN_BRANCO);
    gpio_set_dir(LED_PIN_BRANCO, GPIO_OUT);

    gpio_init(LED_PIN_AMARELO);
    gpio_set_dir(LED_PIN_AMARELO, GPIO_OUT);

    gpio_init(LED_PIN_VERDE);
    gpio_set_dir(LED_PIN_VERDE, GPIO_OUT);

    // IRQs
    gpio_set_irq_enabled_with_callback(BTN_PIN_BRANCO, GPIO_IRQ_EDGE_FALL, true, &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_AMARELO, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BTN_PIN_VERDE, GPIO_IRQ_EDGE_FALL, true);
}

int main() {
    stdio_init_all();
    setup();

    while (true) {
        if (btn_pressed != -1) {
            feedback_botao(btn_pressed);
            btn_pressed = -1;
        }
        sleep_ms(10);
    }
}