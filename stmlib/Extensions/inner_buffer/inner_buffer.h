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
/** \file inner_buffer.h
  * \brief Buffer handler for inter threads operation. The buffer use memory pool,
  *        therefore can be used fixed size items with constant time allocation.
  * \author Gyorgy Stercz
  */

#ifndef INNER_BUFFER_H_INCLUDED
#define INNER_BUFFER_H_INCLUDED

#include <ch.h>
#include <sys/queue.h>

/** \brief Structure of buffer item.
  */
struct inner_buffer_item{
    STAILQ_ENTRY(inner_buffer_item) entries;
    void *data;
};

/** \brief Structure type of inner buffer.
  *
  */
typedef struct{
/** List of empty buffer items. */
    STAILQ_HEAD(headempty, inner_buffer_item) emptyhead;
/** List of empty buffer item head pointer. */
    struct headempty *emptyheadp;
/** List of full buffer items. */
    STAILQ_HEAD(headfull, inner_buffer_item) fullhead;
/** List of full buffer items head pointer. */
    struct headfull *fullheadp;
/** Pointer to buffer memory pool. */
    memory_pool_t *mempool;
/** Number of buffer items. */
    uint16_t buffersize;
/** Number of a filled buffer items. */
    uint16_t itemnum;
/** Number of a empty buffer items .*/
    uint16_t freeitem;
/** Statistic of pool underflow. */
    uint16_t underflow;
/** Statistic of pool overflow. */
    uint16_t overflow;
/** Statistic of full buffer post overflow. */
    uint16_t postoverflow;
/** Memory allocation error. */
    bool malloc_error;
/** Memory pool error. */
    bool pool_error;
}inner_buffer_t;

/** \brief Initializes inner buffer.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \param mempool  Pointer to buffer memory pool, NULL save.
  * \param items    Pointer to memory pool items array, NULL save.
  * \param size     Number of buffer item, must be equal with the memory pool item size.
  */
void innerBufferInit(inner_buffer_t *bfp, memory_pool_t *mempool, void *items,  uint16_t buffersize);

/** \brief Get a new empty buffer item from empty buffer queue.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \return Pointer to the empty buffer item or NULL,
  *         if there are no more empty buffer item or bfp is NULL.
  */
struct inner_buffer_item *getEmptyInnerBufferItem(inner_buffer_t *bfp);

/** \brief Post a filled buffer item into full buffer queue.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \param item     Pointer to filed buffer item, NULL save.
  */
void postFullInnerBufferItem(inner_buffer_t *bfp, struct inner_buffer_item *item);

/** \brief Get a new filled buffer item from the full buffer queue.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \return Pointer to the filled buffer item or NULL,
  *         if there are no more filled buffer item or bfp is NULL.
  */
struct inner_buffer_item *getFullInnerBufferItem(inner_buffer_t *bfp);

/** \brief Put back a buffer item into the empty list.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \param item     Pointer to a buffer item, NULL save.
  */
void releaseEmptyInnerBufferItem(inner_buffer_t *bfp, struct inner_buffer_item *item);

/** \brief Says that the inner buffer is empty.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return 1 if, the buffer is empty, false if not,
  *         -1 if the pointer to inner buffer is NULL.
  */
int8_t isInnerBufferEmpty(inner_buffer_t *bfp);
/** \brief Says that the inner buffer is Full.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return 1 if, the buffer is full, 0 if not,
  *         -1 if the pointer to inner buffer is NULL.
  */
int8_t isInnerBufferFull(inner_buffer_t *bfp);

/** \brief Says the inner buffer size.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return Number of buffer items, -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferSize(inner_buffer_t *bfp);

/** \brief Says number of filled buffer items.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return Number of filled buffer items,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferFullItem(inner_buffer_t *bfp);

/** \brief Says number of empty buffer items.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return Number of empty buffer items,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferFreeItem(inner_buffer_t *bfp);


/** \brief Says post overflow statistic.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return number of post overflow,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferPostOverflow(inner_buffer_t *bfp);

/** \brief Says underflow statistic.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return number of underflow,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferUnderflow(inner_buffer_t *bfp);


/** \brief Says overflow statistic.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return number of overflow,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferOverflow(inner_buffer_t *bfp);

/** \brief Says memory allocation error.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return false if it was not allocation error or
  *         true if it was,  -1 if the pointer to inner buffer is NULL.
  */
int8_t innerBufferMallocError(inner_buffer_t *bfp);

/** \brief Says pool allocation error.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return false if it was not allocation error or
  *         true if it was,  -1 if the pointer to inner buffer is NULL.
  */
int8_t innerBufferPoolError(inner_buffer_t *bfp);

#endif // INNER_BUFFER_H_INCLUDED
