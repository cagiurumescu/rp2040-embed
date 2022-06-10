/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// Example of writing via DMA to the SPI interface and similarly reading it back via a loopback.

#define PICO_DEFAULT_SPI 0
#define PICO_DEFAULT_SPI_CSN_PIN 5
#define PICO_DEFAULT_SPI_SCK_PIN 2
#define PICO_DEFAULT_SPI_TX_PIN  3
#define PICO_DEFAULT_SPI_RX_PIN  4

#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#define TEST_SIZE 16

uint8_t do_read = 0;
#define GPIO_SPI_INT 8
void gpio_callback(uint gpio, uint32_t events) {
   if (gpio==GPIO_SPI_INT) {
      gpio_set_irq_enabled(GPIO_SPI_INT, events, false);
      gpio_acknowledge_irq(GPIO_SPI_INT, events);
      do_read = 1;
   }
}

#ifdef PICO_DEFAULT_SPI_CSN_PIN
static inline void cs_select() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 0);  // Active low
    asm volatile("nop \n nop \n nop");
}

static inline void cs_deselect() {
    asm volatile("nop \n nop \n nop");
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    asm volatile("nop \n nop \n nop");
}
#endif


int main() {
    // Enable UART so we can print status output
    stdio_init_all();
#if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
#warning spi/spi_dma example requires a board with SPI pins
    puts("Default SPI pins were not defined");
#else

    gpio_set_irq_enabled_with_callback(GPIO_SPI_INT, GPIO_IRQ_EDGE_RISE, true, &gpio_callback);


    int spi_baud;
    // Enable SPI at 1 MHz and connect to GPIOs
    spi_baud = spi_init(spi_default, 20 * 1000 * 1000);
    printf("SPI DMA example @ %f MHz\n", ((float)spi_baud)/1E6);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_dir(PICO_DEFAULT_SPI_CSN_PIN, GPIO_OUT);
    gpio_put(PICO_DEFAULT_SPI_CSN_PIN, 1);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));
    // Make the CS pin available to picotool
    bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));

    // Grab some unused dma channels
    const uint dma_tx = dma_claim_unused_channel(true);
    const uint dma_rx = dma_claim_unused_channel(true);

    // Force loopback for testing (I don't have an SPI device handy)
    //hw_set_bits(&spi_get_hw(spi_default)->cr1, SPI_SSPCR1_LBM_BITS);

    static uint8_t txbuf[TEST_SIZE];
    static uint8_t rxbuf[TEST_SIZE];
    for (uint i = 0; i < TEST_SIZE; ++i) {
        txbuf[i] = rand();
    }

    // We set the outbound DMA to transfer from a memory buffer to the SPI transmit FIFO paced by the SPI TX FIFO DREQ
    // The default is for the read address to increment every element (in this case 1 byte = DMA_SIZE_8)
    // and for the write address to remain unchanged.

    printf("Configure TX DMA\n");
    dma_channel_config c = dma_channel_get_default_config(dma_tx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(spi_default, true));
    channel_config_set_read_increment(&c, true);
    channel_config_set_write_increment(&c, false);
    channel_config_set_ring(&c,false,4); // use log2(TEST_SIZE)
    dma_channel_configure(dma_tx, &c,
                          &spi_get_hw(spi_default)->dr, // write address
                          txbuf, // read address
                          TEST_SIZE, // element count (each element is of size transfer_data_size)
                          false); // don't start yet

    printf("Configure RX DMA\n");

    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
    // We configure the read address to remain unchanged for each element, but the write
    // address to increment (so data is written throughout the buffer)
    c = dma_channel_get_default_config(dma_rx);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8);
    channel_config_set_dreq(&c, spi_get_dreq(spi_default, false));
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_ring(&c,true,4); // use log2(TEST_SIZE)
    dma_channel_configure(dma_rx, &c,
                          rxbuf, // write address
                          &spi_get_hw(spi_default)->dr, // read address
                          TEST_SIZE, // element count (each element is of size transfer_data_size)
                          false); // don't start yet


    printf("Starting DMAs...\n");
    // start them exactly simultaneously to avoid races (in extreme cases the FIFO could overflow)

    uint k = 0;
#define DMA_RPT 10
    static uint8_t rxbuf_all[TEST_SIZE*DMA_RPT];
    while (1) {
       if (do_read) {
          cs_select();
          for (uint j = 0; j < DMA_RPT; ++j) {
             dma_start_channel_mask((1u << dma_tx) | (1u << dma_rx));
             dma_channel_wait_for_finish_blocking(dma_rx);
             for (uint i = 0; i < TEST_SIZE; ++i) {
                rxbuf_all[j*TEST_SIZE+i] = rxbuf[i];
             }
          }
          cs_deselect();
          if (k==0) {
             k = rxbuf_all[0];
          } else {
             if ((k%256)!=rxbuf_all[0]) {
                printf("ERROR: %x %x\n", k, rxbuf_all[0]);
                k = rxbuf_all[0];
             }
          }
          /*
          printf("DMARX %3d/%3d: %02x\n", 0, DMA_RPT*TEST_SIZE, rxbuf_all[0]);
          if (dma_channel_get_irq0_status(dma_tx)) {
             printf("DMATX IRQ0\n");
          }
          if (dma_channel_get_irq0_status(dma_rx)) {
             printf("DMARX IRQ0\n");
          }
          printf("Done. Checking...\n");
          for (uint i = 0; i < TEST_SIZE; ++i) {
             printf("DMARX %3d/%3d: %02x\n", i, DMA_RPT*TEST_SIZE, rxbuf_all[i]);
          }
          for (uint i = (DMA_RPT-2)*TEST_SIZE; i < DMA_RPT*TEST_SIZE; ++i) {
             printf("DMARX %3d/%3d: %02x\n", i, DMA_RPT*TEST_SIZE, rxbuf_all[i]);
          }
          */
          k++;
          do_read = 0;
          gpio_set_irq_enabled(GPIO_SPI_INT, GPIO_IRQ_EDGE_RISE, true);
       }
       asm volatile("nop \n nop \n nop");
       //sleep_ms(10);
    }
    /*
    while(1) {
       tight_loop_contents();
    }
    */
    printf("Done\n");
    dma_channel_unclaim(dma_tx);
    dma_channel_unclaim(dma_rx);
    return 0;
#endif
}
