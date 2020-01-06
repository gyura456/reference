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
/** \file fmc_lld.c
  * \brief Flexible memory controller low level source.
  */

#include <stmlib.h>

/** \brief FMC IRQ handler (only Sdram IRQ yet).
  */
OSAL_IRQ_HANDLER(STM32_FSMC_HANDLER){
    OSAL_IRQ_PROLOGUE();
    uint32_t sdram_err_isr = SDRAMD.sdram->SDSR;
    SDRAMD.sdram->SDRTR |= SDRAM_CLEAR_RES_INTERRUPT;
    if (sdram_err_isr & SDRAM_RES_INTERRUPT_BIT)
        _sdram_isr_error_code(&SDRAMD, SDRAM_REFRESH_ERROR);
    OSAL_IRQ_EPILOGUE();
}

/** \brief Initializes Flexible memory controller.
  *             - RCC clock enable
  *             - FMC controller interrupt enable
  */
void fmc_lld_init(void){
    rccEnableFSMC(FALSE);
    nvicEnableVector(STM32_FSMC_NUMBER, STM32_FMC_IRQ_PRIORITY);
}

