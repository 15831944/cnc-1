// File       : tmc.c
// Author     : Jeff Schornick
//
// Functions to control the Trinamic TMC-26x series
//
// Compilation: GCC cross compiler for ARM, v4.9.3+
// Version    : See GitHub repository jschornick/cnc for revision details

#include "spi.h"
#include "gpio.h"
#include "tmc.h"
#include "uart.h"

uint8_t tmc;

tmc_config_t tmc_config[3];
tmc_pinout_t tmc_pins[3] = {
  { .cs_port   = TMC0_CS_PORT,   .cs_pin   = TMC0_CS_PIN,
    .en_port   = TMC0_EN_PORT,   .en_pin   = TMC0_EN_PIN,
    .step_port = TMC0_STEP_PORT, .step_pin = TMC0_STEP_PIN,
    .dir_port  = TMC0_DIR_PORT,  .dir_pin  = TMC0_DIR_PIN
  },
  { .cs_port   = TMC1_CS_PORT,   .cs_pin   = TMC1_CS_PIN,
    .en_port   = TMC1_EN_PORT,   .en_pin   = TMC1_EN_PIN,
    .step_port = TMC1_STEP_PORT, .step_pin = TMC1_STEP_PIN,
    .dir_port  = TMC1_DIR_PORT,  .dir_pin  = TMC1_DIR_PIN
  },
  { .cs_port   = TMC2_CS_PORT,   .cs_pin   = TMC2_CS_PIN,
    .en_port   = TMC2_EN_PORT,   .en_pin   = TMC2_EN_PIN,
    .step_port = TMC2_STEP_PORT, .step_pin = TMC2_STEP_PIN,
    .dir_port  = TMC2_DIR_PORT,  .dir_pin  = TMC2_DIR_PIN
  },
};

#define drvconf_init (DRVCONF                   \
                      | DRVCONF_VSENSE_LOW      \
                      | DRVCONF_SDOFF_STEP )

#define drvctl_init (DRVCTL                     \
                     | DRVCTL_MRES_2            \
                     | DRVCTL_DEDGE_BOTH        \
                     | DRVCTL_INTPOL_OFF )
                     /* | DRVCTL_INTPOL_ON ) */

#define chopconf_init ( CHOPCONF                \
                        | CHOPCONF_CHM_SPREAD   \
                        | CHOPCONF_TBL_24       \
                        | CHOPCONF_TOFF(8) )

#define sgcsconf_init (SGCSCONF                 \
                       | SGCSCONF_SFILT_ON )

//uint8_t cscale_init[] =  { 0x6, 0x4, 0xe };
//uint8_t cscale_init[] =  { 0x6, 0x2, 0xe };
uint8_t cscale_init[] =  { 0x6, 0x2, 0xa };

#define smarten_init (SMARTEN)

uint8_t tmc_axis_conf[] = { TMC_POLARITY_NORMAL, TMC_POLARITY_NORMAL, TMC_POLARITY_NORMAL };

void tmc_init(void)
{
  uint8_t i;

  for(i=0; i<3; i++) {
    // Set SPI CS as output and initialize high (inactive)
    gpio_set_output(tmc_pins[i].cs_port, tmc_pins[i].cs_pin);
    gpio_high(tmc_pins[i].cs_port, tmc_pins[i].cs_pin);

    // Set MOSFET enable as output and initialize high (inactive)
    gpio_set_output(tmc_pins[i].en_port, tmc_pins[i].en_pin);
    gpio_high(tmc_pins[i].en_port, tmc_pins[i].en_pin);

    // Set Step/Dir pins as outputs and initialize
    gpio_set_output(tmc_pins[i].step_port, tmc_pins[i].step_pin);
    gpio_high(tmc_pins[i].step_port, tmc_pins[i].step_pin);

    gpio_set_output(tmc_pins[i].dir_port, tmc_pins[i].dir_pin);
    gpio_high(tmc_pins[i].dir_port, tmc_pins[i].dir_pin);
  }

  tmc_config_t config;
  config.drvconf.raw = drvconf_init;
  config.drvctl.raw = drvctl_init;
  config.chopconf.raw = chopconf_init;
  config.sgcsconf.raw = sgcsconf_init;
  config.smarten.raw = smarten_init;

  for(i=0; i<3; i++) {
    config.sgcsconf.CSCALE = cscale_init[i];
    tmc_reconfigure(i, &config);
  }

  tmc = 0;
}

void tmc_reconfigure(uint8_t tmc, tmc_config_t *config)
{
  tmc_config[tmc].drvconf = config->drvconf;
  tmc_config[tmc].drvctl = config->drvctl;
  tmc_config[tmc].chopconf = config->chopconf;
  tmc_config[tmc].sgcsconf = config->sgcsconf;
  tmc_config[tmc].smarten = config->smarten;

  tmc_send(tmc, tmc_config[tmc].drvconf.raw);
  tmc_send(tmc, tmc_config[tmc].drvctl.raw);
  tmc_send(tmc, tmc_config[tmc].chopconf.raw);
  tmc_send(tmc, tmc_config[tmc].sgcsconf.raw);
  tmc_send(tmc, tmc_config[tmc].smarten.raw);
}

// shift in 24 bits (last 20 bits will be command)
// read in 24 bits (first 20 bits will be response)?
uint32_t tmc_send(uint8_t tmc, uint32_t tx_data)
{

  uint32_t response = 0x0;

  uint8_t tx_byte;

  uart_queue_str("\r\nTMC[");
  uart_queue_hex(tmc,4);
  uart_queue_str("] Sending: ");
  uart_queue_hex(tx_data, 20);
  uart_queue_str("\r\n");

  gpio_low(tmc_pins[tmc].cs_port, tmc_pins[tmc].cs_pin);

  tx_byte = tx_data >> 16;
  response = spi_send_byte(tx_byte) << 12;

  tx_byte = (tx_data >> 8) & 0xff ; // next 8-bits
  response |= spi_send_byte(tx_byte) << 4;

  tx_byte = tx_data & 0xff; // last 8 bits
  response |= spi_send_byte(tx_byte) >> 4;

  gpio_high(tmc_pins[tmc].cs_port, tmc_pins[tmc].cs_pin);

  uart_queue_str("TMC: Received: ");
  uart_queue_hex(response, 20);
  uart_queue_str("\r\n");

  return response;
}

uint8_t tmc_get_current_scale(uint8_t tmc)
{
  //return (tmc_config[tmc].sgcsconf & SGCSCONF_CS_MASK);
  return tmc_config[tmc].sgcsconf.CSCALE;
}

void tmc_set_current_scale(uint8_t tmc, uint32_t value)
{
  tmc_config[tmc].sgcsconf.CSCALE = value;
  tmc_reconfigure(tmc, &tmc_config[tmc]);
}

uint8_t tmc_get_microstep(uint8_t tmc)
{
  return tmc_config[tmc].drvctl.MRES;
}

void tmc_set_microstep(uint8_t tmc, uint32_t value)
{
  tmc_config[tmc].drvctl.MRES = value;
  tmc_reconfigure(tmc, &tmc_config[tmc]);
}

// set fwd/rev direction depending on axis polarity
void tmc_set_dir(uint8_t tmc, uint8_t dir) {
  if (tmc_axis_conf[tmc] == TMC_POLARITY_NORMAL) {
    if (dir == TMC_FWD) {
      gpio_high(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    } else {
      gpio_low(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    }
  } else {
    if (dir == TMC_FWD) {
      gpio_low(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    } else {
      gpio_high(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin);
    }
  }
}

// returns TMC_FWD / TMC_REV depending on polarity and dir pin
int8_t tmc_get_dir(uint8_t tmc) {
  if ( gpio_get_output(tmc_pins[tmc].dir_port, tmc_pins[tmc].dir_pin) ) {
    return (tmc_axis_conf[tmc] == TMC_POLARITY_NORMAL) ? TMC_FWD : TMC_REV;
  } else {
    return (tmc_axis_conf[tmc] == TMC_POLARITY_NORMAL) ? TMC_REV : TMC_FWD;
  }
}
