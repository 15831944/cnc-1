// File       : buttons.c
// Author     : Jeff Schornick
//
// MSP432 button configuration
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include "msp432p401r.h"
#include "gpio.h"
#include "buttons.h"

// Button flags that may be set during GPIO interrupt
volatile uint8_t B1_flag;
volatile uint8_t B2_flag;

// Function: button_init
//
// Initializes GPIOs attached to the two hardware buttons on the MSP432
// board to trigger an interupt when either button is pressed.
void button_init(void)
{
  gpio_set_input(BUTTON1);
  gpio_set_input(BUTTON2);

  gpio_set_pullup(BUTTON1, GPIO_PULL_UP);
  gpio_set_pullup(BUTTON2, GPIO_PULL_UP);

  gpio_set_interrupt(BUTTON1, GPIO_FALLING);
  gpio_set_interrupt(BUTTON2, GPIO_FALLING);

  B1_flag = 0;
  B2_flag = 0;

  __NVIC_EnableIRQ(PORT1_IRQn);
}

// Interrupt handler for Port 1 (buttons)
void PORT1_IRQHandler(void)
{
  // check if it was S1 or S2
  if (P1->IFG & BIT1) {
  /* if (gpio_intr_flag(BUTTON1)) { */
    P1->IFG &= ~BIT1; // clear interrupt flag
    /* gpio_intr_clear(BUTTON1); */
    B1_flag = 1;
  }
  if (P1->IFG & BIT4) {
    P1->IFG &= ~BIT4;
    B2_flag = 1;
  }
}

