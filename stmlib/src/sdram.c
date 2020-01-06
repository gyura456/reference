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
/** \file sdram.c
  * \brief Sdram driver high level source.
  */

#include <stmlib.h>

#if USE_SDRAM_DRIVER == TRUE

/** \brief Sdram driver initialization.
  *
  * \note This function is invoked by stmlibInit() function.
  *
  */
void sdramInit(void){
    sdram_lld_init();
}

/** \brief Sdram driver (SDRAMD) object initialization.
  *
  * \param sdrp Pointer to Sdram driver object.
  */
void sdramObjectInit(SdramDriver *sdrp){
    sdrp->state= SDRAM_STOP;
    sdrp->error_code = 0;
    sdrp->gcfg = NULL;
    uint8_t i;
    for(i=1; i<SDRAM_MAX_BANK_NUM; i++)
        sdrp->bcfg[i] = NULL;
#if SDRAM_USE_MUTUAL_EXCLUSION == TRUE
    osalMutexObjectInit(&sdrp->sdram_mtx);
#endif // SDRAM_USE_MUTUAL_EXCLUSION

#if SDRAM_USE_DMA == TRUE
    sdrp->thread = NULL;
#endif // SDRAM_USE_DMA
}

/** \brief Activates and configure the Sdram driver.
  *
  * \param gcfg Pointer to the SdramConfig object, null save.
  */
void sdramStart(SdramConfig *gcfg){
    if (!gcfg){
        SDRAMD.error_code |= SDRAM_START_PARAM_ERROR;
        return;
    }
    osalSysLock();
    if (SDRAMD.state != SDRAM_STOP && SDRAMD.state != SDRAM_READY){
        SDRAMD.error_code |= SDRAM_START_STATE_ERROR;
        osalSysUnlock();
        return;
    }
    SDRAMD.gcfg = gcfg;
    uint8_t i;
    for(i=0; i<SDRAM_MAX_BANK_NUM; i++)
            SDRAMD.bcfg[i] = gcfg->bcfgarray[i];
    msg_t start_msg = sdram_lld_start(&SDRAMD);
    switch (start_msg){
        case MSG_OK: SDRAMD.state = SDRAM_READY;
             break;
        case MSG_TIMEOUT: SDRAMD.error_code |= SDRAM_TIMEOUT;
        default: break;
    }
    osalSysUnlock();
}

/** \brief Deactivates Sdram driver.
  *
  */
void sdramStop(void){
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return;
    }
    sdram_lld_stop(&SDRAMD);
    SDRAMD.gcfg = NULL;
    uint8_t i;
    for(i=0; i<SDRAM_MAX_BANK_NUM; i++)
        SDRAMD.bcfg[i] = NULL;
    SDRAMD.error_code = 0;
    SDRAMD.state = SDRAM_STOP;
    osalSysUnlock();
}

/** \brief Reads byte into a buffer, from the Sdram memory.
  *
  * \param addr Pointer to start address of reading, null save.
  * \param buffer Pointer to destination buffer, null save.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                 can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state.
  */
msg_t sdramReadByte(uint32_t *addr,  uint8_t *buffer, size_t buffersize){
    if (!(buffer && buffersize))
        return MSG_RESET;
    if (!(IS_SDRAM_BANK1_ADDR(addr) || IS_SDRAM_BANK2_ADDR(addr)))
        return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    sdram_lld_read_byte(addr, buffer, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return MSG_OK;
}

/** \brief Reads 2 byte into a buffer, from the Sdram memory.
  *
  * \param addr Pointer to start address of reading, null save.
  * \param buffer Pointer to destination buffer, null save.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                 can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state.
  */
msg_t sdramRead2Byte(uint32_t *addr,  uint16_t *buffer, size_t buffersize){
    if (!(buffer && buffersize))
        return MSG_RESET;
    if (!(IS_SDRAM_BANK1_ADDR(addr) || IS_SDRAM_BANK2_ADDR(addr)))
        return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    sdram_lld_read_2byte(addr, buffer, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return MSG_OK;
}

/** \brief Reads 4 byte into a buffer, from the Sdram memory.
  *
  * \param addr Pointer to start address of reading, null save.
  * \param buffer Pointer to destination buffer, null save.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                 can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state.
  */
msg_t sdramRead4Byte(uint32_t *addr,  uint32_t *buffer, size_t buffersize){
    if (!(buffer && buffersize))
        return MSG_RESET;
    if (!(IS_SDRAM_BANK1_ADDR(addr) || IS_SDRAM_BANK2_ADDR(addr)))
        return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    sdram_lld_read_4byte(addr, buffer, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return MSG_OK;
}

/** \brief Writes byte into the Sdram memory, from a buffer.
  *
  * \param addr Pointer to start address of writing, null save.
  * \param source Pointer to source buffer, null save.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, - if, the pointer to address is invalid,
  *                    - if, pointer to buffer is NULL, or buffersize is 0,
  *                    - if, Sdram bank is read-only,
  *                    - if, Sdram driver has wrong state.
  */
msg_t sdramWriteByte(uint32_t *addr,  uint8_t *source, size_t buffersize){
    if (!(source && buffersize))
        return MSG_RESET;
    if (IS_SDRAM_BANK1_ADDR(addr)){
        if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK1))
            return MSG_RESET;
        }
        else if (IS_SDRAM_BANK2_ADDR(addr)){
            if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK2))
                return MSG_RESET;
            }
            else
                return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    sdram_lld_write_byte(addr, source, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return MSG_OK;
}

/** \brief Writes 2 byte into the Sdram memory, from a buffer.
  *
  * \param addr Pointer to start address of writing, null save.
  * \param source Pointer to source buffer, null save.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, - if, the pointer to address is invalid,
  *                    - if, pointer to buffer is NULL, or buffersize is 0,
  *                    - if, Sdram bank is read-only,
  *                    - if, Sdram driver has wrong state.
  */
msg_t sdramWrite2Byte(uint32_t *addr,  uint16_t *source, size_t buffersize){
    if (!(source && buffersize))
        return MSG_RESET;
    if (IS_SDRAM_BANK1_ADDR(addr)){
        if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK1))
            return MSG_RESET;
        }
        else if (IS_SDRAM_BANK2_ADDR(addr)){
            if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK2))
                return MSG_RESET;
            }
            else
                return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    sdram_lld_write_2byte(addr, source, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return MSG_OK;
}

/** \brief Writes byte into the Sdram memory, from a buffer.
  *
  * \param addr Pointer to start address of writing, null save.
  * \param source Pointer to source buffer, null save.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, - if, the pointer to address is invalid,
  *                    - if, pointer to buffer is NULL, or buffersize is 0,
  *                    - if, Sdram bank is read-only,
  *                    - if, Sdram driver has wrong state.
  */
msg_t sdramWrite4Byte(uint32_t *addr,  uint32_t *source, size_t buffersize){
    if (!(source && buffersize))
        return MSG_RESET;
    if (IS_SDRAM_BANK1_ADDR(addr)){
        if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK1))
            return MSG_RESET;
        }
        else if (IS_SDRAM_BANK2_ADDR(addr)){
            if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK2))
                return MSG_RESET;
            }
            else
                return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    sdram_lld_write_4byte(addr, source, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return MSG_OK;
}

#if SDRAM_USE_DMA == TRUE

/** \brief Reads byte into a buffer, from the Sdram memory, with DMA controller.
  *
  * \param addr Pointer to start address of reading, null save.
  * \param buffer Pointer to destination buffer, null save.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state,
  *                    - if DMA failure is detected.
  */
msg_t sdramDMAReadByte(uint32_t*addr, uint8_t *buffer, size_t buffersize){
    if (!(buffer && buffersize))
        return MSG_RESET;
    if (!(IS_SDRAM_BANK1_ADDR(addr) || IS_SDRAM_BANK2_ADDR(addr)))
        return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    msg_t read_msg = sdram_lld_dma_read_byte((void*)addr, (void*)buffer, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return read_msg;
}

/** \brief Reads 2 byte into a buffer, from the Sdram memory, with DMA controller.
  *
  * \param addr Pointer to start address of reading, null save.
  * \param buffer Pointer to destination buffer, null save.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state,
  *                    - if DMA failure is detected.
  */
msg_t sdramDMARead2Byte(uint32_t*addr, uint16_t *buffer, size_t buffersize){
    if (!(buffer && buffersize))
        return MSG_RESET;
    if (!(IS_SDRAM_BANK1_ADDR(addr) || IS_SDRAM_BANK2_ADDR(addr)))
        return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    msg_t read_msg = sdram_lld_dma_read_2byte((void*)addr, (void*)buffer, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return read_msg;
}

/** \brief Reads 4 byte into a buffer, from the Sdram memory, with DMA controller.
  *
  * \param addr Pointer to start address of reading, null save.
  * \param buffer Pointer to destination buffer, null save.
  * \param buffersize Size of a destination buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                  can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state,
  *                    - if DMA failure is detected.
  */
msg_t sdramDMARead4Byte(uint32_t*addr, uint32_t *buffer, size_t buffersize){
    if (!(buffer && buffersize))
        return MSG_RESET;
    if (!(IS_SDRAM_BANK1_ADDR(addr) || IS_SDRAM_BANK2_ADDR(addr)))
        return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    msg_t read_msg = sdram_lld_dma_read_4byte((void*)addr, (void*)buffer, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return read_msg;
}

/** \brief Writes byte into the Sdram memory, from a buffer with DMA controller.
  *
  * \param addr Pointer to start address of writing, null save.
  * \param source Pointer to source buffer, null save.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                 can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state,
  *                    - if Sdram bank is read-only,
  *                    - if DMA failure is detected.
  */
msg_t sdramDMAWriteByte(uint32_t*addr, uint8_t *source, size_t buffersize){
    if (!(source && buffersize))
        return MSG_RESET;
     if (IS_SDRAM_BANK1_ADDR(addr)){
        if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK1))
            return MSG_RESET;
        }
        else if (IS_SDRAM_BANK2_ADDR(addr)){
            if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK2))
                return MSG_RESET;
            }
            else
                return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    msg_t write_msg = sdram_lld_dma_write_byte((void*)addr, (void*)source, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return write_msg;
}

/** \brief Writes 2 byte into the Sdram memory, from a buffer with DMA controller.
  *
  * \param addr Pointer to start address of writing, null save.
  * \param source Pointer to source buffer, null save.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                 can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state,
  *                    - if Sdram bank is read-only,
  *                    - if DMA failure is detected.
  */
msg_t sdramDMAWrite2Byte(uint32_t*addr, uint16_t *source, size_t buffersize){
    if (!(source && buffersize))
        return MSG_RESET;
     if (IS_SDRAM_BANK1_ADDR(addr)){
        if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK1))
            return MSG_RESET;
        }
        else if (IS_SDRAM_BANK2_ADDR(addr)){
            if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK2))
                return MSG_RESET;
            }
            else
                return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    msg_t write_msg = sdram_lld_dma_write_2byte((void*)addr, (void*)source, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return write_msg;
}

/** \brief Writes 4 byte into the Sdram memory, from a buffer with DMA controller.
  *
  * \param addr Pointer to start address of writing, null save.
  * \param source Pointer to source buffer, null save.
  * \param buffersize Size of a source buffer, in size_t unit.
  * \retval MSG_OK, if the read operation executed. Sdram refresh errors
  *                 can be detected with the error callback.
  *         MSG_RESET, - if the pointer to address is invalid,
  *                    - if pointer to buffer is NULL, or buffersize is 0,
  *                    - if Sdram driver has wrong state,
  *                    - if Sdram bank is read-only,
  *                    - if DMA failure is detected.
  */
msg_t sdramDMAWrite4Byte(uint32_t*addr, uint32_t *source, size_t buffersize){
    if (!(source && buffersize))
        return MSG_RESET;
     if (IS_SDRAM_BANK1_ADDR(addr)){
        if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK1))
            return MSG_RESET;
        }
        else if (IS_SDRAM_BANK2_ADDR(addr)){
            if (sdram_lld_get_wp(&SDRAMD, SDRAM_BANK2))
                return MSG_RESET;
            }
            else
                return MSG_RESET;
    osalSysLock();
    if (SDRAMD.state != SDRAM_READY){
        osalSysUnlock();
        return MSG_RESET;
    }
    SDRAMD.state = SDRAM_BUSY;
    msg_t write_msg = sdram_lld_dma_write_4byte((void*)addr, (void*)source, buffersize);
    SDRAMD.state = SDRAM_READY;
    osalSysUnlock();
    return write_msg;
}

#endif // SDRAM_USE_DMA

#if SDRAM_USE_MUTUAL_EXCLUSION == TRUE

/** \brief Sdram mutex lock to thread safe operation.
  *
  */
void sdramAcquireBus(void){
    osalMutexLock(&SDRAMD.sdram_mtx);
}

/** \brief Release sdram mutex.
  *
  */
void sdramReleaseBus(void){
    osalMutexUnlock(&SDRAMD.sdram_mtx);
}

#endif // SDRAM_USE_MUTUAL_EXCLUSION

/** Makes Sdram bank to write protected.
  *
  * \param banknum Number of sdram bank.
  */
void setSdramBankWP(uint8_t banknum){
    if (banknum >= SDRAM_MAX_BANK_NUM)
        SDRAMD.error_code |= SDRAM_BANKNUM_ERROR;
    osalSysLock();
    if (SDRAMD.state == SDRAM_READY)
        sdram_lld_set_wp(&SDRAMD, banknum);
    else
        SDRAMD.error_code |= SDRAM_WP_ERROR;
    osalSysUnlock();
}

/** Makes Sdram bank to writable .
  *
  * \param banknum Number of sdram bank.
  */
void unsetSdramBankWP(uint8_t banknum){
    if (banknum >= SDRAM_MAX_BANK_NUM)
        SDRAMD.error_code |= SDRAM_BANKNUM_ERROR;
    osalSysLock();
    if (SDRAMD.state == SDRAM_READY)
        sdram_lld_unset_wp(&SDRAMD, banknum);
    else
        SDRAMD.error_code |= SDRAM_WP_ERROR;
    osalSysUnlock();
}

/** Says Sdram bank write protection status.
  *
  * \param banknum Number of sdram bank.
  * \return 0, if the selected sdram bank is writable,
  *         1, if the selected sdram bank is write protected,
  *         -1, if banknum is wrong.
  */
int8_t getSdramBankWP(uint8_t banknum){
    int8_t wp = -1;
    if (banknum >= SDRAM_MAX_BANK_NUM){
        SDRAMD.error_code |= SDRAM_BANKNUM_ERROR;
        return wp;
    }
    wp = sdram_lld_get_wp(&SDRAMD, banknum);
    return wp;
}

/** Says Sdram driver current state.
  *
  * \retval Code of sdram driver current state.
  */
sdram_state_t getSdramDriverState(void){
    return SDRAMD.state;
}

/** Says Sdram driver error code.
  *
  * \retval Sdram driver error code.
  */
uint32_t getSdramErrorCode(void){
    return SDRAMD.error_code;
}

/** Clear Sdram driver error code.
  */
void clearSdramErrorCode(void){
    SDRAMD.error_code = 0;
}

#endif // USE_SDRAM_DRIVER
