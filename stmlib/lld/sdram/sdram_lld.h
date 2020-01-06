/*
 *   Copyright (C) 2017  Gyorgy Stercz
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/** \file sdram_lld.h
  * \brief Sdram low level driver.
  *
  * \author Gyorgy Stercz
  */
#ifndef SDRAM_LLD_H_INCLUDED
#define SDRAM_LLD_H_INCLUDED

#if USE_SDRAM_DRIVER == TRUE

/** \brief Sdram control register values.
  *
  * \{
  */
#define SDRAMBANK_COL_ADDR_BITS_8 0x0
#define SDRAMBANK_COL_ADDR_BITS_9 FMC_SDCR1_NC_0
#define SDRAMBANK_COL_ADDR_BITS_10 FMC_SDCR1_NC_1
#define SDRAMBANK_COL_ADDR_BITS_11 FMC_SDCR1_NC

#define SDRAMBANK_ROW_ADDR_BITS_11 0x0
#define SDRAMBANK_ROW_ADDR_BITS_12 FMC_SDCR1_NR_0
#define SDRAMBANK_ROW_ADDR_BITS_13 FMC_SDCR1_NR_1

#define SDRAMBANK_MWID_8 0x0
#define SDRAMBANK_MWID_16 FMC_SDCR1_MWID_0
#define SDRAMBANK_MWID_32 FMC_SDCR1_MWID_1

#define SDRAMBANK_INTERNAL_BANK_NUM_2 0x0
#define SDRAMBANK_INTERNAL_BANK_NUM_4 FMC_SDCR1_NB

#define SDRAMBANK_CAS_LATENCY_1_CYCLE FMC_SDCR1_CAS_0
#define SDRAMBANK_CAS_LATENCY_2_CYCLE FMC_SDCR1_CAS_1
#define SDRAMBANK_CAS_LATENCY_3_CYCLE FMC_SDCR1_CAS

#define SDRAMBANK_WRITE_PROTECT FMC_SDCR1_WP

#define SDRAM_CLK_IS_2_HCLK_PERIOD FMC_SDCR1_SDCLK_1
#define SDRAM_CLK_IS_3_HCLK_PERIOD FMC_SDCR1_SDCLK

#define SDRAM_RBURST_EN FMC_SDCR1_RBURST

#define SDRAM_RPIPE_NO_DELAY 0x0
#define SDRAM_RPIPE_1HCLK_DELAY FMC_SDCR1_RPIE_0
#define SDRAM_RPIPE_2HCLK_DELAY FMC_SDCR1_RPIE_1
/**\} */

/** \brief Sdram timing register values.
  *
  * \{
  */
#define SDRAMBANK_TMRD_1_CYCLE 0x0
#define SDRAMBANK_TMRD_2_CYCLE FMC_SDTR1_TMRD_0
#define SDRAMBANK_TMRD_3_CYCLE FMC_SDTR1_TMRD_1
#define SDRAMBANK_TMRD_4_CYCLE (FMC_SDTR1_TMRD_1 | FMC_SDTR1_TMRD_0)
#define SDRAMBANK_TMRD_5_CYCLE FMC_SDTR1_TMRD_2
#define SDRAMBANK_TMRD_6_CYCLE (FMC_SDTR1_TMRD_2 | FMC_SDTR1_TMRD_0)
#define SDRAMBANK_TMRD_7_CYCLE (FMC_SDTR1_TMRD_2 | FMC_SDTR1_TMRD_1)
#define SDRAMBANK_TMRD_8_CYCLE (FMC_SDTR1_TMRD_2 | FMC_SDTR1_TMRD_1 | FMC_SDTR1_TMRD_0)
#define SDRAMBANK_TMRD_9_CYCLE FMC_SDTR1_TMRD_3
#define SDRAMBANK_TMRD_10_CYCLE (FMC_SDTR1_TMRD_3 | FMC_SDTR1_TMRD_0)
#define SDRAMBANK_TMRD_11_CYCLE (FMC_SDTR1_TMRD_3 | FMC_SDTR1_TMRD_1)
#define SDRAMBANK_TMRD_12_CYCLE (FMC_SDTR1_TMRD_3 | FMC_SDTR1_TMRD_1 | FMC_SDTR1_TMRD_0)
#define SDRAMBANK_TMRD_13_CYCLE (FMC_SDTR1_TMRD_3 | FMC_SDTR1_TMRD_2)
#define SDRAMBANK_TMRD_14_CYCLE (FMC_SDTR1_TMRD_3 | FMC_SDTR1_TMRD_2 | FMC_SDTR1_TMRD_0)
#define SDRAMBANK_TMRD_15_CYCLE (FMC_SDTR1_TMRD_3 | FMC_SDTR1_TMRD_2 | FMC_SDTR1_TMRD_1)
#define SDRAMBANK_TMRD_16_CYCLE (FMC_SDTR1_TMRD_3 | FMC_SDTR1_TMRD_2 | FMC_SDTR1_TMRD_1 | FMC_SDTR1_TMRD_0)

#define SDRAMBANK_TXSR_1_CYCLE 0x0
#define SDRAMBANK_TXSR_2_CYCLE FMC_SDTR1_TXSR_0
#define SDRAMBANK_TXSR_3_CYCLE FMC_SDTR1_TXSR_1
#define SDRAMBANK_TXSR_4_CYCLE (FMC_SDTR1_TXSR_1 | FMC_SDTR1_TXSR_0)
#define SDRAMBANK_TXSR_5_CYCLE FMC_SDTR1_TXSR_2
#define SDRAMBANK_TXSR_6_CYCLE (FMC_SDTR1_TXSR_2 | FMC_SDTR1_TXSR_0)
#define SDRAMBANK_TXSR_7_CYCLE (FMC_SDTR1_TXSR_2 | FMC_SDTR1_TXSR_1)
#define SDRAMBANK_TXSR_8_CYCLE (FMC_SDTR1_TXSR_2 | FMC_SDTR1_TXSR_1 | FMC_SDTR1_TXSR_0)
#define SDRAMBANK_TXSR_9_CYCLE FMC_SDTR1_TXSR_3
#define SDRAMBANK_TXSR_10_CYCLE (FMC_SDTR1_TXSR_3 | FMC_SDTR1_TXSR_0)
#define SDRAMBANK_TXSR_11_CYCLE (FMC_SDTR1_TXSR_3 | FMC_SDTR1_TXSR_1)
#define SDRAMBANK_TXSR_12_CYCLE (FMC_SDTR1_TXSR_3 | FMC_SDTR1_TXSR_1 | FMC_SDTR1_TXSR_0)
#define SDRAMBANK_TXSR_13_CYCLE (FMC_SDTR1_TXSR_3 | FMC_SDTR1_TXSR_2)
#define SDRAMBANK_TXSR_14_CYCLE (FMC_SDTR1_TXSR_3 | FMC_SDTR1_TXSR_2 | FMC_SDTR1_TXSR_0)
#define SDRAMBANK_TXSR_15_CYCLE (FMC_SDTR1_TXSR_3 | FMC_SDTR1_TXSR_2 | FMC_SDTR1_TXSR_1)
#define SDRAMBANK_TXSR_16_CYCLE (FMC_SDTR1_TXSR_3 | FMC_SDTR1_TXSR_2 | FMC_SDTR1_TXSR_1 | FMC_SDTR1_TXSR_0)

#define SDRAMBANK_TRAS_1_CYCLE 0x0
#define SDRAMBANK_TRAS_2_CYCLE FMC_SDTR1_TRAS_0
#define SDRAMBANK_TRAS_3_CYCLE FMC_SDTR1_TRAS_1
#define SDRAMBANK_TRAS_4_CYCLE (FMC_SDTR1_TRAS_1 | FMC_SDTR1_TRAS_0)
#define SDRAMBANK_TRAS_5_CYCLE FMC_SDTR1_TRAS_2
#define SDRAMBANK_TRAS_6_CYCLE (FMC_SDTR1_TRAS_2 | FMC_SDTR1_TRAS_0)
#define SDRAMBANK_TRAS_7_CYCLE (FMC_SDTR1_TRAS_2 | FMC_SDTR1_TRAS_1)
#define SDRAMBANK_TRAS_8_CYCLE (FMC_SDTR1_TRAS_2 | FMC_SDTR1_TRAS_1 | FMC_SDTR1_TRAS_0)
#define SDRAMBANK_TRAS_9_CYCLE FMC_SDTR1_TRAS_3
#define SDRAMBANK_TRAS_10_CYCLE (FMC_SDTR1_TRAS_3 | FMC_SDTR1_TRAS_0)
#define SDRAMBANK_TRAS_11_CYCLE (FMC_SDTR1_TRAS_3 | FMC_SDTR1_TRAS_1)
#define SDRAMBANK_TRAS_12_CYCLE (FMC_SDTR1_TRAS_3 | FMC_SDTR1_TRAS_1 | FMC_SDTR1_TRAS_0)
#define SDRAMBANK_TRAS_13_CYCLE (FMC_SDTR1_TRAS_3 | FMC_SDTR1_TRAS_2)
#define SDRAMBANK_TRAS_14_CYCLE (FMC_SDTR1_TRAS_3 | FMC_SDTR1_TRAS_2 | FMC_SDTR1_TRAS_0)
#define SDRAMBANK_TRAS_15_CYCLE (FMC_SDTR1_TRAS_3 | FMC_SDTR1_TRAS_2 | FMC_SDTR1_TRAS_1)
#define SDRAMBANK_TRAS_16_CYCLE (FMC_SDTR1_TRAS_3 | FMC_SDTR1_TRAS_2 | FMC_SDTR1_TRAS_1 | FMC_SDTR1_TRAS_0)

#define SDRAMBANK_TRC_1_CYCLE 0x0
#define SDRAMBANK_TRC_2_CYCLE FMC_SDTR1_TRC_0
#define SDRAMBANK_TRC_3_CYCLE FMC_SDTR1_TRC_1
#define SDRAMBANK_TRC_4_CYCLE (FMC_SDTR1_TRC_1 | FMC_SDTR1_TRC_0)
#define SDRAMBANK_TRC_5_CYCLE FMC_SDTR1_TRC_2
#define SDRAMBANK_TRC_6_CYCLE (FMC_SDTR1_TRC_2 | FMC_SDTR1_TRC_0)
#define SDRAMBANK_TRC_7_CYCLE (FMC_SDTR1_TRC_2 | FMC_SDTR1_TRC_1)
#define SDRAMBANK_TRC_8_CYCLE (FMC_SDTR1_TRC_2 | FMC_SDTR1_TRC_1 | FMC_SDTR1_TRC_0)
#define SDRAMBANK_TRC_9_CYCLE FMC_SDTR1_TRC_3
#define SDRAMBANK_TRC_10_CYCLE (FMC_SDTR1_TRC_3 | FMC_SDTR1_TRC_0)
#define SDRAMBANK_TRC_11_CYCLE (FMC_SDTR1_TRC_3 | FMC_SDTR1_TRC_1)
#define SDRAMBANK_TRC_12_CYCLE (FMC_SDTR1_TRC_3 | FMC_SDTR1_TRC_1 | FMC_SDTR1_TRC_0)
#define SDRAMBANK_TRC_13_CYCLE (FMC_SDTR1_TRC_3 | FMC_SDTR1_TRC_2)
#define SDRAMBANK_TRC_14_CYCLE (FMC_SDTR1_TRC_3 | FMC_SDTR1_TRC_2 | FMC_SDTR1_TRC_0)
#define SDRAMBANK_TRC_15_CYCLE (FMC_SDTR1_TRC_3 | FMC_SDTR1_TRC_2 | FMC_SDTR1_TRC_1)
#define SDRAMBANK_TRC_16_CYCLE (FMC_SDTR1_TRC_3 | FMC_SDTR1_TRC_2 | FMC_SDTR1_TRC_1 | FMC_SDTR1_TRC_0)

#define SDRAMBANK_TWR_1_CYCLE 0x0
#define SDRAMBANK_TWR_2_CYCLE FMC_SDTR1_TWR_0
#define SDRAMBANK_TWR_3_CYCLE FMC_SDTR1_TWR_1
#define SDRAMBANK_TWR_4_CYCLE (FMC_SDTR1_TWR_1 | FMC_SDTR1_TWR_0)
#define SDRAMBANK_TWR_5_CYCLE FMC_SDTR1_TWR_2
#define SDRAMBANK_TWR_6_CYCLE (FMC_SDTR1_TWR_2 | FMC_SDTR1_TWR_0)
#define SDRAMBANK_TWR_7_CYCLE (FMC_SDTR1_TWR_2 | FMC_SDTR1_TWR_1)
#define SDRAMBANK_TWR_8_CYCLE (FMC_SDTR1_TWR_2 | FMC_SDTR1_TWR_1 | FMC_SDTR1_TWR_0)
#define SDRAMBANK_TWR_9_CYCLE FMC_SDTR1_TWR_3
#define SDRAMBANK_TWR_10_CYCLE (FMC_SDTR1_TWR_3 | FMC_SDTR1_TWR_0)
#define SDRAMBANK_TWR_11_CYCLE (FMC_SDTR1_TWR_3 | FMC_SDTR1_TWR_1)
#define SDRAMBANK_TWR_12_CYCLE (FMC_SDTR1_TWR_3 | FMC_SDTR1_TWR_1 | FMC_SDTR1_TWR_0)
#define SDRAMBANK_TWR_13_CYCLE (FMC_SDTR1_TWR_3 | FMC_SDTR1_TWR_2)
#define SDRAMBANK_TWR_14_CYCLE (FMC_SDTR1_TWR_3 | FMC_SDTR1_TWR_2 | FMC_SDTR1_TWR_0)
#define SDRAMBANK_TWR_15_CYCLE (FMC_SDTR1_TWR_3 | FMC_SDTR1_TWR_2 | FMC_SDTR1_TWR_1)
#define SDRAMBANK_TWR_16_CYCLE (FMC_SDTR1_TWR_3 | FMC_SDTR1_TWR_2 | FMC_SDTR1_TWR_1 | FMC_SDTR1_TWR_0)

#define SDRAMBANK_TRP_1_CYCLE 0x0
#define SDRAMBANK_TRP_2_CYCLE FMC_SDTR1_TRP_0
#define SDRAMBANK_TRP_3_CYCLE FMC_SDTR1_TRP_1
#define SDRAMBANK_TRP_4_CYCLE (FMC_SDTR1_TRP_1 | FMC_SDTR1_TRP_0)
#define SDRAMBANK_TRP_5_CYCLE FMC_SDTR1_TRP_2
#define SDRAMBANK_TRP_6_CYCLE (FMC_SDTR1_TRP_2 | FMC_SDTR1_TRP_0)
#define SDRAMBANK_TRP_7_CYCLE (FMC_SDTR1_TRP_2 | FMC_SDTR1_TRP_1)
#define SDRAMBANK_TRP_8_CYCLE (FMC_SDTR1_TRP_2 | FMC_SDTR1_TRP_1 | FMC_SDTR1_TRP_0)
#define SDRAMBANK_TRP_9_CYCLE FMC_SDTR1_TRP_3
#define SDRAMBANK_TRP_10_CYCLE (FMC_SDTR1_TRP_3 | FMC_SDTR1_TRP_0)
#define SDRAMBANK_TRP_11_CYCLE (FMC_SDTR1_TRP_3 | FMC_SDTR1_TRP_1)
#define SDRAMBANK_TRP_12_CYCLE (FMC_SDTR1_TRP_3 | FMC_SDTR1_TRP_1 | FMC_SDTR1_TRP_0)
#define SDRAMBANK_TRP_13_CYCLE (FMC_SDTR1_TRP_3 | FMC_SDTR1_TRP_2)
#define SDRAMBANK_TRP_14_CYCLE (FMC_SDTR1_TRP_3 | FMC_SDTR1_TRP_2 | FMC_SDTR1_TRP_0)
#define SDRAMBANK_TRP_15_CYCLE (FMC_SDTR1_TRP_3 | FMC_SDTR1_TRP_2 | FMC_SDTR1_TRP_1)
#define SDRAMBANK_TRP_16_CYCLE (FMC_SDTR1_TRP_3 | FMC_SDTR1_TRP_2 | FMC_SDTR1_TRP_1 | FMC_SDTR1_TRP_0)

#define SDRAMBANK_TRCD_1_CYCLE 0x0
#define SDRAMBANK_TRCD_2_CYCLE FMC_SDTR1_TRCD_0
#define SDRAMBANK_TRCD_3_CYCLE FMC_SDTR1_TRCD_1
#define SDRAMBANK_TRCD_4_CYCLE (FMC_SDTR1_TRCD_1 | FMC_SDTR1_TRCD_0)
#define SDRAMBANK_TRCD_5_CYCLE FMC_SDTR1_TRCD_2
#define SDRAMBANK_TRCD_6_CYCLE (FMC_SDTR1_TRCD_2 | FMC_SDTR1_TRCD_0)
#define SDRAMBANK_TRCD_7_CYCLE (FMC_SDTR1_TRCD_2 | FMC_SDTR1_TRCD_1)
#define SDRAMBANK_TRCD_8_CYCLE (FMC_SDTR1_TRCD_2 | FMC_SDTR1_TRCD_1 | FMC_SDTR1_TRCD_0)
#define SDRAMBANK_TRCD_9_CYCLE FMC_SDTR1_TRCD_3
#define SDRAMBANK_TRCD_10_CYCLE (FMC_SDTR1_TRCD_3 | FMC_SDTR1_TRCD_0)
#define SDRAMBANK_TRCD_11_CYCLE (FMC_SDTR1_TRCD_3 | FMC_SDTR1_TRCD_1)
#define SDRAMBANK_TRCD_12_CYCLE (FMC_SDTR1_TRCD_3 | FMC_SDTR1_TRCD_1 | FMC_SDTR1_TRCD_0)
#define SDRAMBANK_TRCD_13_CYCLE (FMC_SDTR1_TRCD_3 | FMC_SDTR1_TRCD_2)
#define SDRAMBANK_TRCD_14_CYCLE (FMC_SDTR1_TRCD_3 | FMC_SDTR1_TRCD_2 | FMC_SDTR1_TRCD_0)
#define SDRAMBANK_TRCD_15_CYCLE (FMC_SDTR1_TRCD_3 | FMC_SDTR1_TRCD_2 | FMC_SDTR1_TRCD_1)
#define SDRAMBANK_TRCD_16_CYCLE (FMC_SDTR1_TRCD_3 | FMC_SDTR1_TRCD_2 | FMC_SDTR1_TRCD_1 | FMC_SDTR1_TRCD_0)
/**\} */

/** \brief Sdram command mode register values.
  *
  * \{
  */
#define SDRAMBANK_NRFS_1_CYCLE 0x0
#define SDRAMBANK_NRFS_2_CYCLE FMC_SDCMR_NRFS_0
#define SDRAMBANK_NRFS_3_CYCLE FMC_SDCMR_NRFS_1
#define SDRAMBANK_NRFS_4_CYCLE (FMC_SDCMR_NRFS_1 | FMC_SDCMR_NRFS_0)
#define SDRAMBANK_NRFS_5_CYCLE FMC_SDCMR_NRFS_2
#define SDRAMBANK_NRFS_6_CYCLE (FMC_SDCMR_NRFS_2 | FMC_SDCMR_NRFS_0)
#define SDRAMBANK_NRFS_7_CYCLE (FMC_SDCMR_NRFS_2 | FMC_SDCMR_NRFS_1)
#define SDRAMBANK_NRFS_8_CYCLE (FMC_SDCMR_NRFS_2 | FMC_SDCMR_NRFS_1 | FMC_SDCMR_NRFS_0)
#define SDRAMBANK_NRFS_9_CYCLE FMC_SDCMR_NRFS_3
#define SDRAMBANK_NRFS_10_CYCLE (FMC_SDCMR_NRFS_3 | FMC_SDCMR_NRFS_0)
#define SDRAMBANK_NRFS_11_CYCLE (FMC_SDCMR_NRFS_3 | FMC_SDCMR_NRFS_1)
#define SDRAMBANK_NRFS_12_CYCLE (FMC_SDCMR_NRFS_3 | FMC_SDCMR_NRFS_1 | FMC_SDCMR_NRFS_0)
#define SDRAMBANK_NRFS_13_CYCLE (FMC_SDCMR_NRFS_3 | FMC_SDCMR_NRFS_2)
#define SDRAMBANK_NRFS_14_CYCLE (FMC_SDCMR_NRFS_3 | FMC_SDCMR_NRFS_2 | FMC_SDCMR_NRFS_0)
#define SDRAMBANK_NRFS_15_CYCLE (FMC_SDCMR_NRFS_3 | FMC_SDCMR_NRFS_2 | FMC_SDCMR_NRFS_1)
#define SDRAMBANK_NRFS_16_CYCLE (FMC_SDCMR_NRFS_3 | FMC_SDCMR_NRFS_2 | FMC_SDCMR_NRFS_1 | FMC_SDCMR_NRFS_0)

#define SDRAM_CTB_1 FMC_SDCMR_CTB1
#define SDRAM_CTB_2 FMC_SDCMR_CTB2

#define SDRAM_COMMAND_NORMAL_MODE 0x0
#define SDRAM_COMMAND_CLK_EN FMC_SDCMR_MODE_0
#define SDRAM_COMMAND_PALL FMC_SDCMR_MODE_1
#define SDRAM_COMMAND_AUTO_REFRESH (FMC_SDCMR_MODE_1 | FMC_SDCMR_MODE_0)
#define SDRAM_COMMAND_LOAD_MODE 0x4
#define SDRAM_COMMAND_SELF_REFRESH (0x4 | FMC_SDCMR_MODE_0)
#define SDRAM_COMMAND_POWER_DOWN (0x4 | FMC_SDCMR_MODE_1)
/**\} */

/** \brief Sdram refresh timer register values.
  *
  * \{
  */
#define SDRAM_RES_INTERRUPT_EN FMC_SDRTR_REIE
#define SDRAM_CLEAR_RES_INTERRUPT FMC_SDRTR_CRE
#define SDRAM_RES_INTERRUPT_BIT FMC_SDSR_RE
/**\} */

/** \brief Sdram status register values.
  *
  * \{
  */
#define SDRAM_BUSY FMC_SDSR_BUSY
/**\} */

/** \brief Sdram Bank maximum values.
  */
#define SDRAM_MAX_BANK_NUM 2

/** \brief Sdram bank values and macros
  *
  * \{
  */
#define SDRAM_BANK1 0
#define SDRAM_BANK1_BASE_ADDR (uint32_t*)0xC0000000
#define SDRAM_BANK1_MAX_ADDR  (uint32_t*)0xCFFFFFFF
#define IS_SDRAM_BANK1_ADDR(addr) (SDRAM_BANK1_BASE_ADDR <= (addr) && (addr) <= SDRAM_BANK1_MAX_ADDR)

#define SDRAM_BANK2 1
#define SDRAM_BANK2_BASE_ADDR (uint32_t*)0xD0000000
#define SDRAM_BANK2_MAX_ADDR  (uint32_t*)0xDFFFFFFF
#define IS_SDRAM_BANK2_ADDR(addr) (SDRAM_BANK2_BASE_ADDR <= (addr) && (addr) <= SDRAM_BANK2_MAX_ADDR)
/**\} */

/** \brief Registers reset values.
  *
  * \{
  */
#define SDRAM_SDCR_RESET 0xD20
#define SDRAM_SDTR_RESET 0xFFFFFFF
#define SDRAM_SDCMR_RESET 0x0
#define SDRAM_SDRTR_RESET 0x0
/**\} */
#if !STM32_HAS_FSMC
    #error "FMC not presented in the selected device"
#endif

#ifndef SDRAM_TIMEOUT_US
    #define SDRAM_TIMEOUT_US 100
#endif // SDRAM_TIMEOUT_US

#if SDRAM_USE_DMA == TRUE

#ifndef STM32_SDRAM_DMA_STREAM
#define STM32_SDRAM_DMA_STREAM              STM32_DMA_STREAM_ID(2, 0)
#endif // STM32_SDRAM_DMA_STREAM

#ifndef STM32_SDRAM_DMA_CHANNEL
#define STM32_SDRAM_DMA_CHANNEL             1
#endif // STM32_SDRAM_DMA_CHANNEL

#ifndef STM32_SDRAM_DMA_PRIORITY
#define STM32_SDRAM_DMA_PRIORITY            3
#endif // STM32_SDRAM_DMA_PRIORITY

#ifndef STM32_SDRAM_DMA_IRQ_PRIORITY
#define STM32_SDRAM_DMA_IRQ_PRIORITY        15
#endif // STM32_SDRAM_DMA_IRQ_PRIORITY

#if STM32_SDRAM_DMA_NUM < 1 || STM32_SDRAM_DMA_NUM > 2
    #error "Invalid DMA number!"
#endif

#if STM32_SDRAM_DMA_STREAM_NUM < 0 || STM32_SDRAM_DMA_STREAM_NUM > 7
    #error "Invalid DMA stream number!"
#endif

#if STM32_SDRAM_DMA_CHANNEL < 0 || STM32_SDRAM_DMA_CHANNEL > 7
    #error "Invalid DMA channel!"
#endif

#if !STM32_DMA_IS_VALID_PRIORITY(STM32_SDRAM_DMA_PRIORITY)
    #error "Invalid DMA priority!"
#endif

#if !OSAL_IRQ_IS_VALID_PRIORITY(STM32_SDRAM_DMA_IRQ_PRIORITY)
    #error "Invalid DMA IRQ priority!"
#endif

#endif // SDRAM_USE_DMA



/** \brief Type of Sdram error code.
  */
typedef enum{
    /** Generated, if a new auto-refresh request occurs while the the previous one was not served. */
    SDRAM_REFRESH_ERROR=1,
    /** Generated, if DMA failure detected. */
    SDRAM_DMA_FAILURE,
}sdram_error_t;

/** \brief Type of Sdram driver structure.
  */
typedef struct SdramDriver SdramDriver;

/** \brief Sdram error callback type.
  *
  * \param err Sdram error interrupt code.
  */
typedef void (*sdram_error_cb_t)(sdram_error_t err);

/** \brief Sdram end callback type.
  */
typedef void (*sdram_end_cb_t)(void);

/** \brief Type of Sdram bank configuration structure.
  */
typedef struct {
/** Bank specific part of Sdram configuration register (FMC_SDCR1,2).
  * Contains: SDRAMBANK_WRITE_PROTECT - Write protection bit enable.
  *           SDRAMBANK_CAS_LATENCY_x_CYCLE - CAS latency number of cycle of Sdram ckl cycle.
  *           SDRAMBANK_INTERNAL_BANK_NUM_x - Number of internal banks.
  *           SDRAMBANK_MWID_x - Memory data bus width.
  *           SDRAMBANK_ROW_ADDR_BITS_x - number of row address bits.
  *           SDRAMBANK_COL_ADDR_BITS_x - Number of column address bits.
  */
uint32_t bankcr;
/** Bank specific part of Sdram timing register (FMC_SDTR1,2).
  * Contains: SDRAMBANK_TRCD_x_CYCLE - Row to column delay in number of Sdram clock cycle.
  *           SDRAMBANK_TWR_x_CYCLE - Recovery delay in number of Sdram clock cycle.
  *           SDRAMBANK_TRAS_x_CYCLE - Self refresh time in number of Sdram clock cycle.
  *           SDRAMBANK_TXSR_x_CYCLE - Exit self refresh delay in number of Sdram clock cycle,
  *                                    if two Sdram are used, must be
  *                                    set to all two bank the same TXSR value
  *                                    corresponding to the slowest Sdram device.
  *           SDRAMBANK_TMRD_x_CYCLE -  Load Mode Register to active in number of Sdram clock cycle.
  */
uint32_t banktr;
/** Number of auto refresh. Contains: SDRAMBANK_NRFS_x_CYCLE,
  * corresponding to Sdram device datasheet. (Typical value: 8)
  */
uint32_t autorefresh;
/** Mode register definition, according to the Sdram device datasheet.
  */
uint32_t mrdfield;
}SdramBankConfig;

/** \brief Type of Sdram global configuration structure.
  *        This values concern all two Sdram bank.
  */
typedef struct{
/** Global part of Sdram configuration register (FMC_SDCR1).
  * Contains: SDRAM_CLK_IS_x_HCLK_PERIOD  - Sdram clock period for both Srdram bank.
  *           SDRAM_RBURST_EN  - Enable burst read mode.
  *           SDRAM_RPIPE_x_DELAY - Read pipe delay.
  */
uint32_t crglobal;
/** Global part of Sdram timing register (FMC_SDTR1).
  * Contains: SDRAMBANK_TRP_x_CYCLE - Row precharge delay in number of Sdram clock cycle,
  *                                   if two Sdram device are used, then must be
  *                                   the TRP of slowest device used.
  *           SDRAMBANK_TRC_x_CYCLE - Row cycle delay in number of Sdram clock cycle,
  *                                   if two Sdram device are used, then must be
  *                                   the TRC of slowest device used.
  */
uint32_t trglobal;
/** Refresh timer count. COUNT[12:0] value of FMC_SDRTR register.
  */
uint32_t refreshrate;
/** Wait time after  power-up in microsecond. See Sdram device datasheet,
  * (When two Sdram device are used, then the longest time must be used.)
  */
uint32_t power_up_us;
/** Pointer to the Bank configuration object.
  * bcfgarray[0] - Pointer to Bank1 cfg, if NULL,
  *                then Bank1 configuration will be ignored.
  * bcfgarray[1] - Pointer to Bank2 cfg, if NULL,
  *                then Bank1 configuration will be ignored.
  */
SdramBankConfig *bcfgarray[SDRAM_MAX_BANK_NUM];
/** Pointer to Sdram refresh error callback, or NULL.
  */
sdram_error_cb_t error_cb;
/** Pointer to Sdram  end callback, or NULL.
  */
sdram_end_cb_t end_cb;
}SdramConfig;

/** \brief Sdram driver structure.
  */
struct SdramDriver{
/** Pointer to Sdram register block. */
FMC_Bank5_6_TypeDef *sdram;
/** Pointer to Sdram global configuration object. */
SdramConfig *gcfg;
/** Pointer to Sdram Bank1 configuration objects */
SdramBankConfig *bcfg[SDRAM_MAX_BANK_NUM];
/** Sdram driver state */
volatile sdram_state_t state;
/** Sdram driver error code*/
volatile uint32_t error_code;
#if SDRAM_USE_MUTUAL_EXCLUSION == TRUE
/** Mutex to thread safe operation */
mutex_t sdram_mtx;
#endif // SDRAM_USE_MUTUAL_EXCLUSION

#if SDRAM_USE_DMA == TRUE
/**Thread reference to waiting thread for DMA operation. */
thread_reference_t thread;
/** DMA mode bit mask*/
uint32_t dmamode;
/** Sdram DMA channel*/
const stm32_dma_stream_t *sdramdma;
#endif // SDRAM_USE_DMA
};

/** \brief External declaration of Sdram deriver.
  *
  * \note It is a black box, don't use directly.
  */
extern SdramDriver SDRAMD;

/** \brief Sdram driver low level initialization.
  */
void sdram_lld_init(void);

/** \brief Configure and activates Sdram driver.
  *         - FMC Sdram controller initialization sequence
  *         - FMC Sdram refresh error interrupt enable
  *         - Sdram DMA stream allocation
  * \param sdrp Pointer to Sdram driver object.
  * \return msg MSG_OK, if Sdram start operation is success.
  *             MSG_TIMEOUT, if either one of the two Sdram device is timeout.
  *             MSG_RESET, if all two pointer to Bank configuration object is NULL
  *                        or DMA stream allocation failed.
  */
msg_t sdram_lld_start(SdramDriver *sdrp);

/** \brief Deactivates Sdram driver.
  *
  */
void sdram_lld_stop(SdramDriver *sdrp);

/** \brief Reads byte into a buffer, from the Sdram.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  */
void sdram_lld_read_byte(uint32_t *addr, uint8_t *buffer, size_t buffersize);

/** \brief Reads 2 byte into a buffer, from the Sdram.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  */
void sdram_lld_read_2byte(uint32_t *addr, uint16_t *buffer, size_t buffersize);

/** \brief Reads 4 byte into a buffer, from the Sdram.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  */
void sdram_lld_read_4byte(uint32_t *addr, uint32_t *buffer, size_t buffersize);

/** \brief Writes byte into the Sdram, from a buffer.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  */
void sdram_lld_write_byte(uint32_t *addr, uint8_t *source, size_t buffersize);

/** \brief Writes 2 byte into the Sdram, from a buffer.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  */
void sdram_lld_write_2byte(uint32_t *addr, uint16_t *source, size_t buffersize);

/** \brief Writes 4 byte into the Sdram, from a buffer.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  */
void sdram_lld_write_4byte(uint32_t *addr, uint32_t *source, size_t buffersize);

#if SDRAM_USE_DMA == TRUE
/** \brief Reads byte into a buffer, from the Sdram with DMA controller.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_read_byte(void *addr, void *buffer, size_t buffersize);

/** \brief Reads 2 byte into a buffer, from the Sdram with DMA controller.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_read_2byte(void *addr, void *buffer, size_t buffersize);

/** \brief Reads 4 byte into a buffer, from the Sdram with DMA controller.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_read_4byte(void *addr, void *buffer, size_t buffersize);

/** \brief Writes byte into the Sdram, from a buffer, with DMA controller.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the write operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_write_byte(void *addr, void *source, size_t buffersize);

/** \brief Writes 2 byte into the Sdram, from a buffer, with DMA controller.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the write operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_write_2byte(void *addr, void *source, size_t buffersize);

/** \brief Writes 4 byte into the Sdram, from a buffer, with DMA controller.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the write operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_write_4byte(void *addr, void *source, size_t buffersize);

#endif // SDRAM_USE_DMA

/** Makes Sdram bank to write protected.
  *
  * \param sdrp Pointer to Sdram driver object.
  * \param banknum Number of sdram bank.
  */
void sdram_lld_set_wp(SdramDriver *sdrp, uint8_t banknum);

/** Makes Sdram bank to writable .
  *
  * \param sdrp Pointer to Sdram driver object.
  * \param banknum Number of sdram bank.
  */
void sdram_lld_unset_wp(SdramDriver *sdrp, uint8_t banknum);

/** Says Sdram bank to write protection status.
  *
  * \param sdrp Pointer to Sdram driver object.
  * \param banknum Number of sdram bank.
  * \return 0, if the selected sdram bank is writable,
  *         1, if the selected sdram bank is write protected,
  */
int8_t sdram_lld_get_wp(SdramDriver *sdrp, uint8_t banknum);

#endif // USE_SDRAM_DRIVER

#endif // SDRAM_LLD_H_INCLUDED
