/*------------------------------------------------------------------------------
--       Copyright (c) 2015, VeriSilicon Inc. All rights reserved             --
--         Copyright (c) 2011-2014, Google Inc. All rights reserved.          --
--         Copyright (c) 2007-2010, Hantro OY. All rights reserved.           --
--------------------------------------------------------------------------------
-- Redistribution and use in source and binary forms, with or without         --
-- modification, are permitted provided that the following conditions are met:--
-- * Redistributions of source code must retain the above copyright notice,   --
--       this list of conditions and the following disclaimer.                --
-- * Redistributions in binary form must reproduce the above copyright        --
--       notice, this list of conditions and the following disclaimer in the  --
--       documentation and/or other materials provided with the distribution. --
-- * Neither the names of the copyright holder nor the names of its           --
--       contributors may be used to endorse or promote products derived from --
--       this software without specific prior written permission.             --
--------------------------------------------------------------------------------
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"--
-- AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE  --
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE --
-- ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE  --
-- LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR        --
-- CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF       --
-- SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS   --
-- INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN    --
-- CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)    --
-- ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE --
-- POSSIBILITY OF SUCH DAMAGE.                                                --
--------------------------------------------------------------------------------
------------------------------------------------------------------------------*/
#include "dwlthread.h"
#ifdef _USE_VSI_STRING_
#include "vsi_string.h"
#else
#include <string.h>
#endif

#include "input_queue.h"
#include "fifo.h"
#include "sw_util.h"
#include "dec_log.h"
#include "deccfg.h"
//#define BUFFER_QUEUE_PRINT_STATUS
#ifdef BUFFER_QUEUE_PRINT_STATUS
#define PRINT_COUNTS(x) PrintCounts(x)
#else
#define PRINT_COUNTS(x)
#endif /* BUFFER_QUEUE_PRINT_STATUS */

static void DecreaseInputQueueCntById(InputQueue queue, u32 id);
static void IncreaseInputQueueCntById(InputQueue queue, u32 id);

/* Data structure to hold this picture buffer queue instance data. */
struct IQueue {
  pthread_mutex_t cs; /* Critical section to protect data. */
  i32 max_buffers;    /* Number of max buffers accepted. */
  i32 n_buffers;      /* Number of buffers contained in total. */
  struct DWLDecBuffers buffers[MAX_PIC_BUFFERS];
  u32 pp_out_ctrl[MAX_PIC_BUFFERS];
  u32 buffer_used[MAX_PIC_BUFFERS];
  u32 ref_cnt[MAX_PIC_BUFFERS];
  u32 ref_flag[MAX_PIC_BUFFERS];/* flag that buffer get by threaad.(not increase cnt yet) */
  pthread_mutex_t cnt_release_mutex;
  pthread_cond_t cnt_release_cv;
  pthread_mutex_t buf_release_mutex;
  pthread_cond_t buf_release_cv;
  u32 abort;
};

InputQueue InputQueueInit(i32 n_buffers) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("(n_buffers=%i)", n_buffers);
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(n_buffers >= 0);
  struct IQueue* q = (struct IQueue*)DWLcalloc(1, sizeof(struct IQueue));
  if (q == NULL) {
    return NULL;
  }

  q->max_buffers = MAX_PIC_BUFFERS;
  q->n_buffers = 0;
  DWLmemset(q->buffers, 0, sizeof(q->buffers));
  DWLmemset(q->buffer_used, 0, sizeof(q->buffer_used));
  DWLmemset(q->ref_cnt, 0, sizeof(q->ref_cnt));
  DWLmemset(q->ref_flag, 0, sizeof(q->ref_flag));
  pthread_mutex_init(&q->cnt_release_mutex, NULL);
  pthread_cond_init(&q->cnt_release_cv, NULL);
  pthread_mutex_init(&q->buf_release_mutex, NULL);
  pthread_cond_init(&q->buf_release_cv, NULL);
  if (pthread_mutex_init(&q->cs, NULL)) {
    pthread_mutex_destroy(&q->cnt_release_mutex);
    pthread_cond_destroy(&q->cnt_release_cv);
    pthread_mutex_destroy(&q->buf_release_mutex);
    pthread_cond_destroy(&q->buf_release_cv);
    DWLfree(q);
    return NULL;
  }

  (void)n_buffers;
  return q;
}

void DecreaseInputQueueCnt(InputQueue queue, struct DWLLinearMem *addr) {
  ASSERT(queue);
  u32 i;

  i = InputQueueFindBufferId(queue, addr);

  DecreaseInputQueueCntById(queue, i);
}

void IncreaseInputQueueCnt(InputQueue queue, struct DWLLinearMem *addr) {
  ASSERT(queue);
  u32 i;

  i = InputQueueFindBufferId(queue, addr);

  IncreaseInputQueueCntById(queue, i);
}


void InputQueueReset(InputQueue queue) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;
  if (!q) return;

  if (q->n_buffers) {/* Destory mutex before releasing. */
    pthread_mutex_destroy(&q->cs);
    pthread_mutex_destroy(&q->buf_release_mutex);
    pthread_cond_destroy(&q->buf_release_cv);
    pthread_mutex_destroy(&q->cnt_release_mutex);
    pthread_cond_destroy(&q->cnt_release_cv);
  }

#ifdef USE_OMXIL_BUFFER
  q->max_buffers = MAX_PIC_BUFFERS;
  q->n_buffers = 0;
  DWLmemset(q->buffers, 0, sizeof(q->buffers));
  DWLmemset(q->buffer_used, 0, sizeof(q->buffer_used));
  DWLmemset(q->ref_cnt, 0, sizeof(q->ref_cnt));
  DWLmemset(q->ref_flag, 0, sizeof(q->ref_flag));
#else
  DWLmemset(q->ref_cnt, 0, sizeof(q->ref_cnt));
  DWLmemset(q->ref_flag, 0, sizeof(q->ref_flag));
#endif
  pthread_mutex_init(&q->cs, NULL);
  pthread_mutex_init(&q->buf_release_mutex, NULL);
  pthread_cond_init(&q->buf_release_cv, NULL);
  pthread_mutex_init(&q->cnt_release_mutex, NULL);
  pthread_cond_init(&q->cnt_release_cv, NULL);
}

void InputQueueRelease(InputQueue queue) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;
#ifdef EXT_BUF_SAFE_RELEASE
  i32 i;
  for (i = 0; i < q->n_buffers; i++) {
    if (q->ref_cnt[i] && !q->buffer_used[i]) {
      q->ref_cnt[i] = 0;
      q->ref_flag[i] = 0;
    }
  }
#endif
  pthread_mutex_destroy(&q->buf_release_mutex);
  pthread_cond_destroy(&q->buf_release_cv);
  pthread_mutex_destroy(&q->cnt_release_mutex);
  pthread_cond_destroy(&q->cnt_release_cv);
  pthread_mutex_destroy(&q->cs);
  DWLfree(q);
}

static void IncreaseInputQueueCntById(InputQueue queue, u32 id) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;

  if (id >= q->n_buffers)
    return;

  pthread_mutex_lock(&q->cnt_release_mutex);
  q->ref_cnt[id]++;
  pthread_mutex_unlock(&q->cnt_release_mutex);
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

struct DWLLinearMem *InputQueueGetBuffer(InputQueue queue, u32 *wait) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  struct DWLLinearMem *buffer;
  i32 i;
  struct IQueue* q = (struct IQueue*)queue;

  pthread_mutex_lock(&q->cnt_release_mutex);
  for (i = 0; i < q->n_buffers; i++) {
    if ((!q->ref_cnt[i]) && (!q->ref_flag[i])) {
      q->ref_flag[i] = 1;
      break;
    }
  }
  if (i == q->n_buffers) {
    if (!(*wait)) {
      pthread_mutex_unlock(&q->cnt_release_mutex);
      return NULL;
    } else {
      /* wait for a buffer free */
      while(i == q->n_buffers) {
        for (i = 0; i < q->n_buffers; i++) {
          if ((!q->ref_cnt[i]) && (!q->ref_flag[i])) {
            q->ref_flag[i] = 1;
            break;
          }
        }
        pthread_cond_wait(&q->cnt_release_cv, &q->cnt_release_mutex);
      }
    }
  }
  pthread_mutex_unlock(&q->cnt_release_mutex);

  if (q->buffer_used[i] && !q->abort) {
    pthread_mutex_lock(&q->cnt_release_mutex);
    q->ref_flag[i] = 0;
    pthread_mutex_unlock(&q->cnt_release_mutex);
    return NULL;
  }

  pthread_mutex_lock(&q->buf_release_mutex);
  while(q->buffer_used[i] && !q->abort)
    pthread_cond_wait(&q->buf_release_cv, &q->buf_release_mutex);
  pthread_mutex_unlock(&q->buf_release_mutex);
  if (q->abort) return NULL;
  pthread_mutex_lock(&q->cs);
  buffer = (void *)&q->buffers[i].dec_buffer[0][0];
#ifdef DBG_MP_PLANE
    for (u32 j=0; j<DEC_MAX_PPU_COUNT; j++) {
      for (u32 k=0; k<MAX_OUTPUT_PLANE; k++) {
        if (DWL_DEVMEM_VAILD(q->buffers[i].dec_buffer[j][k])) {
          APITRACEERR("InputQueueGetBuffer: q->buffers[%d].dec_buffer[%d][%d] %p\n",
                       i, j, k, (void*)q->buffers[i].dec_buffer[j][k].bus_address);
        }
      }
    }
#endif
  pthread_mutex_unlock(&q->cs);
  IncreaseInputQueueCntById(queue, i);
  *wait = i;

#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("# %i\n", i);
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  return buffer;
}

void InputQueueSetPPOutCtrl(InputQueue queue, const struct DWLLinearMem *buffer, u32 pp_out_ctrl) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;
  u32 index = InputQueueFindBufferId(queue, (struct DWLLinearMem *)buffer);

  if (index < q->n_buffers)
    q->pp_out_ctrl[index] = pp_out_ctrl;
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

u32 InputQueueGetPPOutCtrl(InputQueue queue, const struct DWLLinearMem *buffer) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;
  u32 index = InputQueueFindBufferId(queue, (struct DWLLinearMem *)buffer);

  if (index >= q->n_buffers)
    return 0;

#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  return q->pp_out_ctrl[index];
}

u32 InputQueueAddBuffer(InputQueue queue, struct DWLLinearMem *buffer) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;

  u32 index = InputQueueUpdateBuffer(queue, buffer, q->n_buffers);

#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */

  return index;
}

u32 InputQueueUpdateBuffer(InputQueue queue, struct DWLLinearMem *buffer, u32 index) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;

  pthread_mutex_lock(&q->cs);
  ASSERT(index <= q->n_buffers);
#ifdef DEC_SAFE_OUTPUT_CHECK
  u32 i = InputQueueFindBufferId(q, buffer);
  if (i < q->n_buffers) {
    APITRACEERR("InputQueueAddBuffer: before pp slot %d has same addr, current add slot %d\n", i, q->n_buffers);
    return;
  }
#endif
  q->buffers[index] = *(struct DWLDecBuffers *)buffer;

  if (index == q->n_buffers)
    q->n_buffers++; /* Add one picture buffer among empty picture buffers. */

  pthread_mutex_unlock(&q->cs);

#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */

  return index;
}

u32 InputQueueFindBufferId(InputQueue queue, struct DWLLinearMem *addr) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;

  u32 index = SwFindBufferIdByAddr(q->buffers, q->n_buffers, addr);

  return (index);
}


#if 0
void InputQueueWaitPending(InputQueue queue) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;

  while (FifoCount(q->fifo_in) != (u32)q->n_buffers) sched_yield();

#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}
#endif

static void DecreaseInputQueueCntById(InputQueue queue, u32 id) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;

  if (id >= q->n_buffers)
    return;

  if (!q->ref_cnt[id])
    return;

  pthread_mutex_lock(&q->cnt_release_mutex);
  q->ref_cnt[id]--;
  // ASSERT(!q->ref_cnt[id]);
  if ((!q->ref_cnt[id]) && !q->ref_flag[id]) {
    pthread_cond_signal(&q->cnt_release_cv);
  }
  pthread_mutex_unlock(&q->cnt_release_mutex);
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

/* return the index of old buffer or replace old with new */
u32 InputQueueReturnBuffer(InputQueue queue, struct DWLLinearMem *returned_buffer,
                           u32 ext_flag, u32 pp_index) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  u32 i = pp_index;
  struct IQueue* q = (struct IQueue*)queue;

#if defined(DBG_MP_PLANE)
  APITRACEERR("InputQueueReturnBuffer: 1) found id=%d, q->n_buffers %d ext_flag %d\n", i, q->n_buffers, ext_flag);
#endif

  if (i >= q->n_buffers || i != pp_index) {
    ASSERT(0);
    APITRACEERR("InputQueueReturnBuffer faild with n_buffers %d, i %d, and replace pp_index %d\n",
                 q->n_buffers, i, pp_index);
    return q->n_buffers;
  }

#if defined(DBG_MP_PLANE)
  u32 a, j, k;
  const struct DWLDecBuffers *temp_buffer = (const struct DWLDecBuffers *)returned_buffer;
  for (a = 0; a < q->n_buffers; a++) {
    for (j = 0; j < DEC_MAX_PPU_COUNT; j++) {
      for (k = 0; k < MAX_OUTPUT_PLANE; k++) {
        if (DWL_DEVMEM_VAILD(q->buffers[a].dec_buffer[j][k])) {
          if (a == i) {
            APITRACEERR("\t q->buffers[%d].dec_buffer[%d][%d] %p(vir=%p) with q->buffer_used[%d] %d, replaced with returned_buffer[%d][%d] %p\n", a, j, k,
                      (void *)q->buffers[a].dec_buffer[j][k].bus_address, (void *)q->buffers[a].dec_buffer[j][k].virtual_address, a, q->buffer_used[a],
                      j, k, (void*)temp_buffer->dec_buffer[j][k].bus_address);
          }
#if 0
          else {
            APITRACEERR("\t q->buffers[%d].dec_buffer[%d][%d] %p(vir=%p) with q->buffer_used[%d] %d\n", a, j, k,
                      (void *)q->buffers[a].dec_buffer[j][k].bus_address, (void *)q->buffers[a].dec_buffer[j][k].virtual_address, a, q->buffer_used[a]);
          }
#endif
        }
      }
    }
  }
#endif

  pthread_mutex_lock(&q->cs);
  SwReplacePpOutBuf(ext_flag, (struct DWLDecBuffers *)returned_buffer, &q->buffers[i]);
  pthread_mutex_unlock(&q->cs);

  pthread_mutex_lock(&q->buf_release_mutex);
  q->buffer_used[i] = 0;
  pthread_cond_signal(&q->buf_release_cv);
  pthread_mutex_unlock(&q->buf_release_mutex);

  pthread_mutex_lock(&q->cnt_release_mutex);
  q->ref_flag[i] = 0;
  pthread_mutex_unlock(&q->cnt_release_mutex);
  DecreaseInputQueueCntById(queue, i);

#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */

  return i;
}

void InputQueueSetAbort(InputQueue queue) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  struct IQueue* q = (struct IQueue*)queue;
  if (!q) return;

  pthread_mutex_lock(&q->buf_release_mutex);
  q->abort = 1;
  pthread_cond_signal(&q->buf_release_cv);
  pthread_mutex_unlock(&q->buf_release_mutex);
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

void InputQueueClearAbort(InputQueue queue) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  struct IQueue* q = (struct IQueue*)queue;
  if (!q) return;

  pthread_mutex_lock(&q->buf_release_mutex);
  q->abort = 0;
  pthread_mutex_unlock(&q->buf_release_mutex);
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

void InputQueueReturnAllBuffer(InputQueue queue) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;
  i32 i;

  for (i = 0; i < q->n_buffers; i++) {
    pthread_mutex_lock(&q->cnt_release_mutex);
    q->ref_cnt[i] = 0;
    q->ref_flag[i] = 0;
    pthread_cond_signal(&q->cnt_release_cv);
    pthread_mutex_unlock(&q->cnt_release_mutex);

    /* Add one picture buffer among empty picture buffers. */
    pthread_mutex_lock(&q->buf_release_mutex);
    q->buffer_used[i] = 0;
    pthread_cond_signal(&q->buf_release_cv);
    pthread_mutex_unlock(&q->buf_release_mutex);
  }
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

u32 InputQueueWaitNotUsed(InputQueue queue) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  i32 i;
  struct IQueue* q = (struct IQueue*)queue;
  for(i = 0; i < q->n_buffers; i++) {
    pthread_mutex_lock(&q->buf_release_mutex);
#if 1
    while (q->buffer_used[i] && !q->abort)
      pthread_cond_wait(&q->buf_release_cv, &q->buf_release_mutex);
#else
    if (q->buffer_used[i]) {
      pthread_mutex_unlock(&q->buf_release_mutex);
      return 1;
    }
#endif
    pthread_mutex_unlock(&q->buf_release_mutex);
  }
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif
  return 0;
}

void InputQueueWaitBufNotUsed(InputQueue queue, struct DWLLinearMem *addr) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  i32 i;

  struct IQueue* q = (struct IQueue*)queue;

  i = InputQueueFindBufferId(queue, addr);
  #ifdef DBG_MP_PLANE
  APITRACEERR("InputQueueWaitBufNotUsed: found id=%d\n", i);
  #endif
  if (i >= q->n_buffers) {
    return;
  }
  pthread_mutex_lock(&q->buf_release_mutex);
  while (q->buffer_used[i] && !q->abort)
    pthread_cond_wait(&q->buf_release_cv, &q->buf_release_mutex);
  pthread_mutex_unlock(&q->buf_release_mutex);
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

void InputQueueSetBufAsUsed(InputQueue queue, struct DWLLinearMem *buffer, u32 pp_index) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  i32 i = pp_index;
  struct IQueue* q = (struct IQueue*)queue;
#ifdef DEC_SAFE_OUTPUT_CHECK
  i = InputQueueFindBufferId(queue, buffer);
#endif
#ifdef DBG_MP_PLANE
  u32 a, j, k;
  for (a = 0; a < q->n_buffers; a++) {
    for (j = 0; j < DEC_MAX_PPU_COUNT; j++) {
      for (k = 0; k < MAX_OUTPUT_PLANE; k++) {
        if (DWL_DEVMEM_VAILD(q->buffers[a].dec_buffer[j][k])) {
          if (a == i) {
            APITRACEERR("InputQueueSetBufAsUsed: q->buffers[%d].dec_buffer[%d][%d] %p\n",
                         i, j, k, (void*)q->buffers[i].dec_buffer[j][k].bus_address);
          }
        }
      }
    }
  }
#endif
  if (i >= q->n_buffers)
    return;

  pthread_mutex_lock(&q->buf_release_mutex);
  q->buffer_used[i] = 1;
  pthread_mutex_unlock(&q->buf_release_mutex);
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
}

i32 InputQueueCheckBufUsed(InputQueue queue, struct DWLLinearMem *addr, u32 used_by_app, u32 pp_index) {
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("()");
  printf("\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  ASSERT(queue);
  i32 i = pp_index;
  struct IQueue* q = (struct IQueue*)queue;
  i32 ret = 0;

#ifdef DEC_SAFE_OUTPUT_CHECK
  i = InputQueueFindBufferId(queue, addr);
  if (i != pp_index) {
    APITRACEERR("InputQueueCheckBufUsed: addr not in pp slot %d, but in slot %d\n", pp_index, i);
    ASSERT(0);
  }
#endif
  if (i < q->n_buffers) {
    pthread_mutex_lock(&q->buf_release_mutex);
    if (used_by_app) {
      if (q->buffer_used[i])
        ret = 1;
    }
    else {
      if (q->ref_cnt[i])
        ret = 1;
    }
    pthread_mutex_unlock(&q->buf_release_mutex);
  }
#ifdef DBG_MP_PLANE
  if (i < q->n_buffers) {
    APITRACEERR("InputQueueCheckBufUsed: found id=%d with q->buffer_used[%d] %d, q->ref_cnt[%d] %d with used_by_app %d\n",
                 i, i, q->buffer_used[i], i, q->ref_cnt[i], used_by_app);
  }
  else {
    APITRACEERR("InputQueueCheckBufUsed: %s\n", "not found id, error");
  }
#endif
#ifdef BUFFER_QUEUE_PRINT_STATUS
  printf(__FUNCTION__);
  printf("#\n");
#endif /* BUFFER_QUEUE_PRINT_STATUS */
  return ret;
}

u32 InputQueueGetBufferCnt(InputQueue queue) {
  struct IQueue* q = (struct IQueue*)queue;

  if (!q) return 0;
  else return q->n_buffers;
}

struct DWLDecBuffers *InputQueueGetBufByID(InputQueue queue, u32 id) {
  ASSERT(queue);
  struct IQueue* q = (struct IQueue*)queue;

  return &q->buffers[id];
}

void InputQueueBufferClear(const void *dwl, InputQueue queue) {
  struct IQueue* q = (struct IQueue*)queue;
  u32 i;

  for (i = 0; i < q->n_buffers; i++) {
    DWLLinearMemset(dwl, &q->buffers[i].dec_buffer[0][0], 0, 0, q->buffers[i].dec_buffer[0][0].size);
  }
}
