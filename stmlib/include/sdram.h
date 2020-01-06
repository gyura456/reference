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
/** \file sdram.h
  * \brief Sdram high level driver.
  *
  * \author Gyorgy Stercz
  */
#ifndef SDRAM_H_INCLUDED
#define SDRAM_H_INCLUDED

#include <stmlib.h>

#if USE_SDRAM_DRIVER == TRUE

/** \brief Sdram driver error codes
  *
  * \{
  */
#define SDRAM_START_PARAM_ERROR         1
#define SDRAM_START_STATE_ERROR         2
#define SDRAM_TIMEOUT                   4
#define SDRAM_NO_BANK_CONFIG            8
#define SDRAM_DMA_STREAM_ALLOCATE_ERROR 10
#define SDRAM_BANKNUM_ERROR             12
#define SDRAM_WP_ERROR                  14
/**\} */

/** \brief Enables mutual exclusion API-s
  */
#ifndef SDRAM_USE_MUTUAL_EXCLUSION
#define SDRAM_USE_MUTUAL_EXCLUSION  FALSE
#endif // SDRAM_USE_MUTUAL_EXCLUSION

/** \brief Sdram driver FSM states
  */
typedef enum {SDRAM_UNINT=0, SDRAM_STOP, SDRAM_READY, SDRAM_BUSY, SDRAM_COMPLETE, SDRAM_ERROR} sdram_state_t;

#include <sdram_lld.h>

#if SDRAM_USE_DMA == TRUE
/** \brief Wakes up the waiting thread in case of Sdram Refresh error interrupt
  *        or DMA error interrupt with a MSG_RESET massage.
  * \param sdrp Pointer to SdramDriver object.
  */
#define _sdram_wakeup_error_isr(sdrp) {                     \
            osalSysLockFromISR();                           \
            osalThreadResumeI(&(sdrp)->thread, MSG_RESET);  \
            osalSysUnlockFromISR();                         \
        }

/** \brief Wakes up the waiting thread in case of read or write operation
  *        is complete. Only when you use DMA read/write functions.
  * \param sdrp Pointer to SdramDriver object.
  */
#define _sdram_wakeup_complete_isr(sdrp) {                  \
            osalSysLockFromISR();                           \
            osalThreadResumeI(&(sdrp)->thread, MSG_OK);     \
            osalSysUnlockFromISR();                         \
        }
#endif

/** \brief Common part of error ISR code, in case Sdram refresh error interrupt
  *         and in case of DMA error interrupt. Error callback invocation and driver state machine transaction.
  * \param sdrp Pointer to SdramDriver object.
  * \param err  Error code.
  */
#define _sdram_isr_error_code(sdrp, err) {              \
            sdram_state_t curr_state = (sdrp)->state;   \
            if ((sdrp)->gcfg->error_cb != NULL){        \
                (sdrp)->state = SDRAM_ERROR;            \
                (sdrp)->gcfg->error_cb(err);            \
            }                                           \
            if ((sdrp)->state == SDRAM_ERROR)           \
                (sdrp)->state = curr_state;             \
        }

/** \brief ISR code in case of DMA operation is complete.
  *        End callback invocation and driver state machine transaction.
  * \param sdrp Pointer to SdramDriver object.
  */
#define _sdram_isr_complete_code(sdrp)  {               \
            if ((sdrp)->gcfg->end_cb != NULL){          \
                (sdrp)->state = SDRAM_COMPLETE;         \
                (sdrp)->gcfg->end_cb();                 \
            }                                           \
        }

/** \brief Sdram driver initialization.
  *
  * \note This function is invoked by stmlibInit() function.
  *
  */
void sdramInit(void);

/** \brief Sdram driver (SDRAMD) object initialization.
  *
  * \param sdrp Pointer to Sdram driver object.
  */
void sdramObjectInit(SdramDriver *sdrp);

/** \brief Activates and configure the Sdram driver.
  *
  * \param gcfg Pointer to the SdramConfig object, null save.
  */
void sdramStart(SdramConfig *gcfg);

/** \brief Deactivates Sdram driver.
  *
  */
void sdramStop(void);

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
msg_t sdramReadByte(uint32_t *addr,  uint8_t *buffer, size_t buffersize);

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
msg_t sdramRead2Byte(uint32_t *addr,  uint16_t *buffer, size_t buffersize);

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
msg_t sdramRead4Byte(uint32_t *addr,  uint32_t *buffer, size_t buffersize);

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
msg_t sdramWriteByte(uint32_t *addr,  uint8_t *source, size_t buffersize);

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
msg_t sdramWrite2Byte(uint32_t *addr,  uint16_t *source, size_t buffersize);

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
msg_t sdramWrite4Byte(uint32_t *addr,  uint32_t *source, size_t buffersize);

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
msg_t sdramDMAReadByte(uint32_t*addr, uint8_t *buffer, size_t buffersize);

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
msg_t sdramDMARead2Byte(uint32_t*addr, uint16_t *buffer, size_t buffersize);

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
msg_t sdramDMARead4Byte(uint32_t*addr, uint32_t *buffer, size_t buffersize);

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
msg_t sdramDMAWriteByte(uint32_t*addr, uint8_t *source, size_t buffersize);

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
msg_t sdramDMAWrite2Byte(uint32_t*addr, uint16_t *source, size_t buffersize);

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
msg_t sdramDMAWrite4Byte(uint32_t*addr, uint32_t *source, size_t buffersize);

#endif // SDRAM_USE_DMA

#if SDRAM_USE_MUTUAL_EXCLUSION == TRUE

/** \brief Sdram mutex lock to thread safe operation.
  *
  */
void sdramAcquireBus(void);

/** \brief Release sdram mutex.
  *
  */
void sdramReleaseBus(void);

#endif // SDRAM_USE_MUTUAL_EXCLUSION

/** Makes Sdram bank to write protected.
  *
  * \param banknum Number of sdram bank.
  */
void setSdramBankWP(uint8_t banknum);

/** Makes Sdram bank to writable .
  *
  * \param banknum Number of sdram bank.
  */
void unsetSdramBankWP(uint8_t banknum);

/** Says Sdram bank write protection status.
  *
  * \param banknum Number of sdram bank.
  * \return 0, if the selected sdram bank is writable,
  *         1, if the selected sdram bank is write protected,
  *         -1, if banknum is wrong.
  */
int8_t getSdramBankWP(uint8_t banknum);

/** Says Sdram driver current state.
  *
  * \retval Code of sdram driver current state.
  */
sdram_state_t getSdramDriverState(void);

/** Says Sdram driver error code.
  *
  * \retval Sdram driver error code.
  */
uint32_t getSdramErrorCode(void);

/** Clear Sdram driver error code.
  */
void clearSdramErrorCode(void);

#endif // USE_SDRAM_DRIVER

#endif // SDRAM_H_INCLUDED
