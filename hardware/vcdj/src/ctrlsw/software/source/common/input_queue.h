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

#ifndef _INPUT_QUEUE_H_
#define _INPUT_QUEUE_H_

#include "basetype.h"
#include "dwl.h"

/* InputQueue is picture queue indexing module, which manages the
 * buffer references on client's behalf. Module will maintain an index of
 * available and used buffers. When free  buffer is not available module
 * will block the thread asking for a buffer index. */

typedef void* InputQueue; /* Opaque instance variable. */


/* Functions to initialize and release the InputQueue. */
InputQueue InputQueueInit(i32 n_buffers);
void InputQueueRelease(InputQueue queue);

/* Function to get free buffers from the queue. Returns negative value if the
   free buffer was not found and the buffer count is below the limit. Otherwise
   blocks until the requested buffer is available. */
struct DWLLinearMem *InputQueueGetBuffer(InputQueue queue, u32 *wait);
void InputQueueSetPPOutCtrl(InputQueue queue, const struct DWLLinearMem *buffer, u32 pp_out_ctrl);
u32 InputQueueGetPPOutCtrl(InputQueue queue, const struct DWLLinearMem *buffer);

/* Function to wait until all buffers are in available status. */
u32 InputQueueWaitNotUsed(InputQueue queue);

void InputQueueWaitBufNotUsed(InputQueue queue, struct DWLLinearMem *addr);

void InputQueueSetBufAsUsed(InputQueue queue, struct DWLLinearMem *buffer, u32 pp_index);

u32 InputQueueAddBuffer(InputQueue queue, struct DWLLinearMem *buffer);

u32 InputQueueUpdateBuffer(InputQueue queue, struct DWLLinearMem *buffer, u32 index);

/* Return buffer to input queue so that it can be used or replaced freely. */
u32 InputQueueReturnBuffer(InputQueue queue, struct DWLLinearMem *returned_buffer,
                           u32 ext_flag, u32 pp_index);

u32 InputQueueFindBufferId(InputQueue queue, struct DWLLinearMem *addr);

void DecreaseInputQueueCnt(InputQueue queue, struct DWLLinearMem *addr);
void IncreaseInputQueueCnt(InputQueue queue, struct DWLLinearMem *addr);
u32 InputQueueGetBufferCnt(InputQueue queue);
#if 0
void InputQueueWaitPending(InputQueue queue);
#endif
i32 InputQueueCheckBufUsed(InputQueue queue, struct DWLLinearMem *addr, u32 used_by_app, u32 pp_index);
void InputQueueSetAbort(InputQueue queue);
void InputQueueClearAbort(InputQueue queue);
void InputQueueReturnAllBuffer(InputQueue queue);
void InputQueueReset(InputQueue queue);

struct DWLDecBuffers *InputQueueGetBufByID(InputQueue queue, u32 id);
u32 InputQueueGetBufferCnt(InputQueue queue);
void InputQueueBufferClear(const void *dwl, InputQueue queue);
#endif /* _INPUT_QUEUE_H_ */
