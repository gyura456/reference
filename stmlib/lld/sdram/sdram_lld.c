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
/** \file sdram_lld.c
  * \brief Sdram driver low level source.
  */

#include <stmlib.h>

#if USE_SDRAM_DRIVER == TRUE

/** \brief External declaration of Sdram deriver.
  *
  * \note It is a black box, don't use directly.
  */
SdramDriver SDRAMD;

#if SDRAM_USE_DMA == TRUE
/** \brief Sdram DMA isr code.
  *
  * \param sdrp pointer to SdramDriver object;
  * \param DMA flags to isr detection.
  */
static void sdram_dma_isr(SdramDriver *sdrp, uint32_t flags){
    if (flags & STM32_DMA_ISR_TEIF){
        _sdram_isr_error_code(sdrp, SDRAM_DMA_FAILURE);
        _sdram_wakeup_error_isr(sdrp);
    }
    else if (flags & STM32_DMA_ISR_TCIF){
        _sdram_isr_complete_code(sdrp);
        _sdram_wakeup_complete_isr(sdrp);
    }

}
#endif // SDRAM_USE_DMA

/** \brief Send command to the Sdram bank.
  *
  * \param sdrp Pointer to SdramDriver object.
  * \param command Code of command.
  * \param timeout Send operation timeout value, in number of system tick.
  * \retval MSG_OK, if operation success.
  *         MSG_TIMEOUT, if timeout occurred before send operation executed.
  */
static inline msg_t sdram_lld_send_command(SdramDriver *sdrp, uint32_t command, systime_t timeout){
    systime_t start, end;
    sdrp->sdram->SDCMR = command;
    start = osalOsGetSystemTimeX();
    end = start+timeout;
    while (sdrp->sdram->SDSR & SDRAM_BUSY){
        if (!osalOsIsTimeWithinX(osalOsGetSystemTimeX(), start, end))
            return MSG_TIMEOUT;
    }
    return MSG_OK;
}

/** \brief Sdram driver low level initialization.
  */
void sdram_lld_init(void){
    sdramObjectInit(&SDRAMD);
    SDRAMD.sdram = FMC_Bank5_6;
#if SDRAM_USE_DMA ==TRUE
    SDRAMD.thread = NULL;
    SDRAMD.sdramdma = STM32_DMA_STREAM(STM32_SDRAM_DMA_STREAM);
    SDRAMD.dmamode = STM32_DMA_CR_CHSEL(STM32_SDRAM_DMA_CHANNEL) |
                     STM32_DMA_CR_PL(STM32_SDRAM_DMA_PRIORITY) |
                     STM32_DMA_CR_TCIE | STM32_DMA_CR_TEIE;
#endif // SDRAM_USE_DMA
}

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
msg_t sdram_lld_start(SdramDriver *sdrp){
    msg_t  start_msg = MSG_RESET;
    sdrp->sdram->SDCR[0] = sdrp->gcfg->crglobal;
    sdrp->sdram->SDTR[0] = sdrp->gcfg->trglobal;
    uint8_t i;
    systime_t start, end;
    uint32_t bank_en_bit[SDRAM_MAX_BANK_NUM] = {SDRAM_CTB_1, SDRAM_CTB_2};
    for(i=0; i<SDRAM_MAX_BANK_NUM; i++){
        if (sdrp->bcfg[i]){
            sdrp->sdram->SDCR[i] |= sdrp->bcfg[i]->bankcr;
            sdrp->sdram->SDTR[i] |= sdrp->bcfg[i]->banktr;
            if((start_msg = sdram_lld_send_command(&SDRAMD, (SDRAM_COMMAND_CLK_EN | bank_en_bit[i]), US2ST(SDRAM_TIMEOUT_US))) == MSG_TIMEOUT)
                return start_msg;
            start = osalOsGetSystemTimeX();
            end = start + OSAL_US2ST(sdrp->gcfg->power_up_us);
            while (!osalOsIsTimeWithinX(osalOsGetSystemTimeX(), start, end))
            ;
            if ((start_msg = sdram_lld_send_command(sdrp, (SDRAM_COMMAND_PALL | bank_en_bit[i]), US2ST(SDRAM_TIMEOUT_US))) == MSG_TIMEOUT)
                return start_msg;
            if ((start_msg = sdram_lld_send_command(sdrp, (SDRAM_COMMAND_AUTO_REFRESH | bank_en_bit[i] | sdrp->bcfg[i]->autorefresh), US2ST(SDRAM_TIMEOUT_US))) == MSG_TIMEOUT)
                return start_msg;
            if ((start_msg = sdram_lld_send_command(sdrp, (SDRAM_COMMAND_LOAD_MODE | bank_en_bit[i] | (sdrp->bcfg[i]->mrdfield << 9)), US2ST(SDRAM_TIMEOUT_US))) == MSG_TIMEOUT)
                return start_msg;
        }
    }
    if (start_msg == MSG_RESET){
        sdrp->error_code |= SDRAM_NO_BANK_CONFIG;
        return start_msg;
    }
    sdrp->sdram->SDRTR = (SDRAM_RES_INTERRUPT_EN | (sdrp->gcfg->refreshrate << 1));
#if SDRAM_USE_DMA == TRUE
    if (dmaStreamAllocate(sdrp->sdramdma, STM32_SDRAM_DMA_IRQ_PRIORITY, (stm32_dmaisr_t)sdram_dma_isr, sdrp)){
        sdrp->error_code |= SDRAM_DMA_STREAM_ALLOCATE_ERROR;
        start_msg = MSG_RESET;
        return start_msg;
    }
    dmaStreamSetFIFO(sdrp->sdramdma, STM32_DMA_FCR_FTH_FULL)
#endif // SDRAM_USE_DMA
    return start_msg;
}

/** \brief Deactivates Sdram driver.
  *
  */
void sdram_lld_stop(SdramDriver *sdrp){
    uint8_t i;
    for(i=0; i<SDRAM_MAX_BANK_NUM; i++)
        sdrp->sdram->SDCR[i] = SDRAM_SDCR_RESET;
    for(i=0; i<SDRAM_MAX_BANK_NUM; i++)
        sdrp->sdram->SDTR[i] = SDRAM_SDTR_RESET;
    sdrp->sdram->SDCMR = SDRAM_SDCMR_RESET;
    sdrp->sdram->SDRTR = SDRAM_SDRTR_RESET;
}

/** \brief Reads byte into a buffer, from the Sdram.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  */
void sdram_lld_read_byte(uint32_t *addr, uint8_t *buffer, size_t buffersize){
    uint8_t *src = (uint8_t*)addr;
    for(;buffersize != 0; buffersize--, buffer++, src++)
        *buffer = *src;
}

/** \brief Reads 2 byte into a buffer, from the Sdram.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  */
void sdram_lld_read_2byte(uint32_t *addr, uint16_t *buffer, size_t buffersize){
    uint16_t *src = (uint16_t*)addr;
    for(;buffersize != 0; buffersize--, buffer++, src++)
        *buffer= *src;
}

/** \brief Reads 4 byte into a buffer, from the Sdram.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  */
void sdram_lld_read_4byte(uint32_t *addr, uint32_t *buffer, size_t buffersize){
    uint32_t *src = addr;
    for(;buffersize != 0; buffersize--, buffer++, src++)
        *buffer = *src;
}

/** \brief Writes byte into the Sdram, from a buffer.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  */
void sdram_lld_write_byte(uint32_t *addr, uint8_t *source, size_t buffersize){
    uint8_t *dst = (uint8_t*)addr;
    for(; buffersize!=0; buffersize--, dst++, source++)
        *dst = *source;
}

/** \brief Writes 2 byte into the Sdram, from a buffer.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  */
void sdram_lld_write_2byte(uint32_t *addr, uint16_t *source, size_t buffersize){
    uint16_t *dst = (uint16_t*)addr;
    for(; buffersize!=0; buffersize--, dst++, source++)
        *dst = *source;
}

/** \brief Writes 4 byte into the Sdram, from a buffer.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  */
void sdram_lld_write_4byte(uint32_t *addr, uint32_t *source, size_t buffersize){
    uint32_t *dst = addr;
    for(; buffersize!=0; buffersize--, dst++, source++)
        *dst = *source;
}

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
msg_t sdram_lld_dma_read_byte(void *addr, void *buffer, size_t buffersize){
    osalSysUnlock();
    dmaStartMemCopy(SDRAMD.sdramdma, SDRAMD.dmamode, addr, buffer, buffersize);
    osalSysLock();
    return osalThreadSuspendS(&SDRAMD.thread);
}

/** \brief Reads 2 byte into a buffer, from the Sdram with DMA controller.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_read_2byte(void *addr, void *buffer, size_t buffersize){
    osalSysUnlock();
    dmaStartMemCopy(SDRAMD.sdramdma, (SDRAMD.dmamode | STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD), addr, buffer, buffersize);
    osalSysLock();
    return osalThreadSuspendS(&SDRAMD.thread);
}

/** \brief Reads 4 byte into a buffer, from the Sdram with DMA controller.
  *
  * \param addr Pointer to start address of reading.
  * \param buffer Pointer to destination buffer.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_read_4byte(void *addr, void *buffer, size_t buffersize){
    osalSysUnlock();
    dmaStartMemCopy(SDRAMD.sdramdma, (SDRAMD.dmamode | STM32_DMA_CR_PSIZE_WORD | STM32_DMA_CR_MSIZE_WORD), addr, buffer, buffersize);
    osalSysLock();
    return osalThreadSuspendS(&SDRAMD.thread);
}

/** \brief Writes byte into the Sdram, from a buffer, with DMA controller.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the write operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_write_byte(void *addr, void *source, size_t buffersize){
    osalSysUnlock();
    dmaStartMemCopy(SDRAMD.sdramdma, SDRAMD.dmamode, source, addr, buffersize);
    osalSysLock();
    return osalThreadSuspendS(&SDRAMD.thread);
}

/** \brief Writes 2 byte into the Sdram, from a buffer, with DMA controller.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the write operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_write_2byte(void *addr, void *source, size_t buffersize){
    osalSysUnlock();
    dmaStartMemCopy(SDRAMD.sdramdma, (SDRAMD.dmamode | STM32_DMA_CR_PSIZE_HWORD | STM32_DMA_CR_MSIZE_HWORD), source, addr, buffersize);
    osalSysLock();
    return osalThreadSuspendS(&SDRAMD.thread);
}

/** \brief Writes 4 byte into the Sdram, from a buffer, with DMA controller.
  *
  * \param addr Pointer to start address of writing.
  * \param source Pointer to source buffer.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the write operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, if DMA failure is detected.
  */
msg_t sdram_lld_dma_write_4byte(void *addr, void *source, size_t buffersize){
    osalSysUnlock();
    dmaStartMemCopy(SDRAMD.sdramdma, (SDRAMD.dmamode | STM32_DMA_CR_PSIZE_WORD | STM32_DMA_CR_MSIZE_WORD), source, addr, buffersize);
    osalSysLock();
    return osalThreadSuspendS(&SDRAMD.thread);
}

#endif // SDRAM_USE_DMA

/** Makes Sdram bank to write protected.
  *
  * \param sdrp Pointer to Sdram driver object.
  * \param banknum Number of sdram bank.
  */
void sdram_lld_set_wp(SdramDriver *sdrp, uint8_t banknum){
    sdrp->sdram->SDCR[banknum] |= SDRAMBANK_WRITE_PROTECT;
}

/** Makes Sdram bank to writable .
  *
  * \param sdrp Pointer to Sdram driver object.
  * \param banknum Number of sdram bank.
  */
void sdram_lld_unset_wp(SdramDriver *sdrp, uint8_t banknum){
    sdrp->sdram->SDCR[banknum] &= ~(SDRAMBANK_WRITE_PROTECT);
}

/** Says Sdram bank to write protection status.
  *
  * \param sdrp Pointer to Sdram driver object.
  * \param banknum Number of sdram bank.
  * \return 0, if the selected sdram bank is writable,
  *         1, if the selected sdram bank is write protected,
  */
int8_t sdram_lld_get_wp(SdramDriver *sdrp, uint8_t banknum){
   return (sdrp->sdram->SDCR[banknum] & SDRAMBANK_WRITE_PROTECT) ? 1 : 0;
}
#endif // USE_SDRAM_DRIVER
