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
/** \file fmc_lld.h
  * \brief Flexible memory controller low level driver.
  *         -FMC interrupt handling (only sdram interrupt yet).
  *
  * \author Gyorgy Stercz
  */
#ifndef FMC_LLD_ISR_H_INCLUDED
#define FMC_LLD_ISR_H_INCLUDED

#if !STM32_HAS_FSMC
    #error "FMC not presented in the selected device"
#endif

#ifndef STM32_FMC_IRQ_PRIORITY
    #define STM32_FMC_IRQ_PRIORITY 15
#endif // STM32_SDRAM_IRQ_PRIORITY

#if !OSAL_IRQ_IS_VALID_PRIORITY(STM32_FMC_IRQ_PRIORITY)
    #error "STM32_FMC_IRQ_PRIORITY has wrong value"
#endif

/** \brief Initializes Flexible memory controller.
  *             - RCC clock enable
  *             - FMC controller interrupt enable
  */
void fmc_lld_init(void);

#endif // FMC_LLD_ISR_H_INCLUDED
