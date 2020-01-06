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
/** \file inner_buffer.c
  * \brief Buffer inter threads operation source.
  */
#include <strings.h>
#include <inner_buffer.h>

static MUTEX_DECL(inbfmtx);

/** \brief Initializes inner buffer.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \param mempool  Pointer to buffer memory pool, NULL save.
  * \param items    Pointer to memory pool items array, NULL save.
  * \param size     Number of buffer item, must be equal with the memory pool item size.
  */
void innerBufferInit(inner_buffer_t *bfp, memory_pool_t *mempool, void *items,  uint16_t buffersize){
    if (!(bfp&&mempool&&items&&buffersize))
        return;
    bzero(bfp, sizeof(inner_buffer_t));
    bfp->mempool = mempool;
    bfp->buffersize = buffersize;
    chPoolLoadArray(bfp->mempool, items, bfp->buffersize);
    STAILQ_INIT(&bfp->emptyhead);
    STAILQ_INIT(&bfp->fullhead);
    for (bfp->freeitem = 0; bfp->freeitem<bfp->buffersize; bfp->freeitem++){
        struct inner_buffer_item *buffer;
        buffer = chCoreAlloc(sizeof(struct inner_buffer_item));
        if (!buffer){
            bfp->malloc_error = 1;
            return;
            }
        bzero(buffer, sizeof(struct inner_buffer_item));
        buffer->data = chPoolAlloc(bfp->mempool);
        if (!buffer->data){
            bfp->pool_error = 1;
            return;
            }
        if (STAILQ_EMPTY(&bfp->emptyhead))
            STAILQ_INSERT_HEAD(&bfp->emptyhead, buffer, entries);
        else
            STAILQ_INSERT_TAIL(&bfp->emptyhead, buffer, entries);

    }
}

/** \brief Get a new empty buffer item from empty buffer queue.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \return Pointer to the empty buffer item or NULL,
  *         if there are no more empty buffer item or bfp is NULL.
  */
struct inner_buffer_item *getEmptyInnerBufferItem(inner_buffer_t *bfp){
    if (!bfp)
        return NULL;
    if (STAILQ_EMPTY(&bfp->emptyhead)){
        chMtxLock(&inbfmtx);
        bfp->underflow++;
        chMtxUnlock(&inbfmtx);
        return NULL;
    }
    chMtxLock(&inbfmtx);
    struct inner_buffer_item *item = STAILQ_FIRST(&bfp->emptyhead);
    STAILQ_REMOVE_HEAD(&bfp->emptyhead, entries);
    bfp->freeitem--;
    chMtxUnlock(&inbfmtx);
    return item;
}

/** \brief Post a filled buffer item into full buffer queue.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \param item     Pointer to filed buffer item, NULL save.
  */
void postFullInnerBufferItem(inner_buffer_t *bfp, struct inner_buffer_item *item){
    if (!(bfp&&item))
        return;
    if (bfp->itemnum == bfp->buffersize){
        bfp->postoverflow++;
        return;
    }
    chMtxLock(&inbfmtx);
    if (STAILQ_EMPTY(&bfp->fullhead))
        STAILQ_INSERT_HEAD(&bfp->fullhead, item, entries);
    else
        STAILQ_INSERT_TAIL(&bfp->fullhead, item, entries);
    bfp->itemnum++;
    chMtxUnlock(&inbfmtx);
}

/** \brief Get a new filled buffer item from the full buffer queue.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \return Pointer to the filled buffer item or NULL,
  *         if there are no more filled buffer item or bfp is NULL.
  */
struct inner_buffer_item *getFullInnerBufferItem(inner_buffer_t *bfp){
    if (!bfp||STAILQ_EMPTY(&bfp->fullhead))
        return NULL;
    chMtxLock(&inbfmtx);
    struct inner_buffer_item *item = STAILQ_FIRST(&bfp->fullhead);
    STAILQ_REMOVE_HEAD(&bfp->fullhead, entries);
    bfp->itemnum--;
    chMtxUnlock(&inbfmtx);
    return item;
}

/** \brief Put back a buffer item into the empty list.
  *
  * \param bfp      Pointer to inner buffer object, NULL save.
  * \param item     Pointer to a buffer item, NULL save.
  */
void releaseEmptyInnerBufferItem(inner_buffer_t *bfp, struct inner_buffer_item *item){
    if (!(bfp&&item))
        return;
    if (bfp->freeitem == bfp->buffersize){
        bfp->overflow++;
        return;
    }
    chMtxLock(&inbfmtx);
    //chPoolFree(bfp->mempool, item->data);
    if (STAILQ_EMPTY(&bfp->emptyhead))
        STAILQ_INSERT_HEAD(&bfp->emptyhead, item, entries);
    else
        STAILQ_INSERT_TAIL(&bfp->emptyhead, item, entries);
    bfp->freeitem++;
    chMtxUnlock(&inbfmtx);
}

/** \brief Says that the inner buffer is empty.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return 1 if, the buffer is empty, false if not,
  *         -1 if the pointer to inner buffer is NULL.
  */
int8_t isInnerBufferEmpty(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return STAILQ_EMPTY(&bfp->fullhead);
}

/** \brief Says that the inner buffer is Full.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return 1 if, the buffer is full, 0 if not,
  *         -1 if the pointer to inner buffer is NULL.
  */
int8_t isInnerBufferFull(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return STAILQ_EMPTY(&bfp->emptyhead);
}

/** \brief Says the inner buffer size.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return Number of buffer items, -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferSize(inner_buffer_t *bfp){
        if (!bfp)
            return -1;
        return bfp->buffersize;
    }

/** \brief Says number of filled buffer items.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return Number of filled buffer items,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferFullItem(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return bfp->itemnum;
}

/** \brief Says number of empty buffer items.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return Number of empty buffer items,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferFreeItem(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return bfp->freeitem;
}

/** \brief Says post overflow statistic.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return number of post overflow,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferPostOverflow(inner_buffer_t *bfp){
     if (!bfp)
        return -1;
    return bfp->postoverflow;
}

/** \brief Says underflow statistic.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return number of underflow,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferUnderflow(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return bfp->underflow;
}

/** \brief Says overflow statistic.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return number of overflow,  -1 if the pointer to inner buffer is NULL.
  */
int32_t innerBufferOverflow(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return bfp->overflow;
}

/** \brief Says memory allocation error.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return false if it was not allocation error or
  *         true if it was,  -1 if the pointer to inner buffer is NULL.
  */
int8_t innerBufferMallocError(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return bfp->malloc_error;
}

/** \brief Says pool allocation error.
  *
  * \param bfp  Pointer to inner buffer, NULL save.
  * \return false if it was not allocation error or
  *         true if it was,  -1 if the pointer to inner buffer is NULL.
  */
int8_t innerBufferPoolError(inner_buffer_t *bfp){
    if (!bfp)
        return -1;
    return bfp->pool_error;
    }
