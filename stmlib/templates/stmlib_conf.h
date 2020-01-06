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
/** \file
  * \brief
  *
  * \author
  */
#ifndef STMLIB_CONF_H_INCLUDED
#define STMLIB_CONF_H_INCLUDED

/** \brief Sdram Driver settings
  *
  * \{
  */
#define USE_SDRAM_DRIVER    TRUE
/* Sdram device timeout in microsecond */
    #define SDRAM_TIMEOUT_US                       100
/* Enable sdramAcqiureBus() and sdramReleaseBus() functions to thread safe operation */
    #define SDRAM_USE_MUTUAL_EXCLUSION          TRUE
/* DMA settings */
    #define SDRAM_USE_DMA                       FALSE
        #define STM32_SDRAM_DMA_NUM             2
        #define STM32_SDRAM_DMA_STREAM_NUM      0
        #define STM32_SDRAM_DMA_STREAM          STM32_DMA_STREAM_ID(STM32_SDRAM_DMA_NUM, STM32_SDRAM_DMA_STREAM_NUM)
        #define STM32_SDRAM_DMA_CHANNEL         1
        #define STM32_SDRAM_DMA_PRIORITY        3
        #define STM32_SDRAM_DMA_IRQ_PRIORITY    5
    #define STM32_FMC_IRQ_PRIORITY              5
#if SDRAM_USE_DMA
#ifndef STM32_DMA_REQUIRED
#define STM32_DMA_REQUIRED
#endif // STM32_DMA_REQUIRED
#endif // SDRAM_USE_DMA
/**\} */

#endif // STMLIB_CONF_H_INCLUDED
