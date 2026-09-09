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

#include "dwl.h"
#include "regdrv.h"
#include "sw_util.h"
#include "string.h"
#include "deccfg.h"

static const u32 reg_mask[33] = {
  0x00000000, 0x00000001, 0x00000003, 0x00000007, 0x0000000F, 0x0000001F, /* reg mask to indicate how many bits need to operate on, currently from  0 to  5 */
  0x0000003F, 0x0000007F, 0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF, /* reg mask to indicate how many bits need to operate on, currently from  6 to 11 */
  0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF, 0x0001FFFF, /* reg mask to indicate how many bits need to operate on, currently from 12 to 17 */
  0x0003FFFF, 0x0007FFFF, 0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF, /* reg mask to indicate how many bits need to operate on, currently from 18 to 23 */
  0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF, 0x1FFFFFFF, /* reg mask to indicate how many bits need to operate on, currently from 24 to 29 */
  0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF                                      /* reg mask to indicate how many bits need to operate on, currently from 30 to 32 */
};

/* { SWREG, BITS, POSITION, WRITABLE } */
const HwRegDesc hw_dec_reg_spec[HWIF_LAST_REG + 1] = {
  /* include script-generated part */
#include "8170table.h"
  /* HWIF_DEC_IRQ_STAT */ G2REG(1, 15, 11, 0),
  /* dummy entry */ G2REG(0, 0, 0, 0)
};

#ifdef LIBVA_DEBUG
FILE *regiset_ofile = NULL;
pthread_mutex_t va_oflie_mutex;
char registername[HWIF_LAST_REG][100] = {
  #include "8170enum_string.h"
};
#endif

void SetDecRegister(u32* reg_base, u32 id, u32 value) {
  u32 tmp;
  const HwRegDesc *desc;

  ASSERT(id < HWIF_LAST_REG);
  if (id > HWIF_LAST_REG)
	  return;

  desc = &hw_dec_reg_spec[id];

  if (desc->reg_id == 0)
    return; /* Try to set a undefined register -> return directly */

  tmp = reg_base[desc->reg_id]; /* get the source value of reg of swreg index which gain from the 1st attr of id */
  tmp &= ~(reg_mask[desc->bits] << desc->posi); /* clean the bits that need to be set with "and" */
  tmp |= (value & reg_mask[desc->bits]) << desc->posi; /* set the bits with "or" */
  reg_base[desc->reg_id] = tmp;

#ifdef LIBVA_DEBUG
  if(regiset_ofile == NULL) {
    pthread_mutex_init(&va_oflie_mutex, NULL);
    regiset_ofile = fopen("setReigsetValues.txt","wb");
  }
  pthread_mutex_lock(&va_oflie_mutex);
  u32 addr_value_workaround = 10000;
  if (value > addr_value_workaround)
    fprintf(regiset_ofile, "%-48s - 0x%x\n", registername[id], value);
  else
    fprintf(regiset_ofile, "%-48s - %9u\n", registername[id], value);
  fflush(regiset_ofile);
  pthread_mutex_unlock(&va_oflie_mutex);
#endif
}

u32 GetDecRegister(const u32* reg_base, u32 id) {
  u32 tmp;
  const HwRegDesc *desc;

  ASSERT(id < HWIF_LAST_REG);

  desc = &hw_dec_reg_spec[id];

  /* Try to reading a undefined register -> return 0 */
  if (desc->reg_id == 0)
    return 0;

  tmp = reg_base[desc->reg_id];
  tmp = tmp >> desc->posi;
  tmp &= reg_mask[desc->bits];
  return (tmp);
}

void SetPpuRegister(u32 *pp_reg_base, u32 id, int ppu_index, u32 value) {
  u32 tmp;
  const HwRegDesc *desc;

  ASSERT(id < HWIF_LAST_REG);

  /* hw reg spec is based on decoder so we have to adjust the register offset
   * because we are getting PP base register as parameter
   */
  desc = &hw_dec_reg_spec[id];

  tmp = pp_reg_base[desc->reg_id + PPU_REG_RANGE * ppu_index];
  tmp &= ~(reg_mask[desc->bits] << desc->posi);
  tmp |= (value & reg_mask[desc->bits]) << desc->posi;
  pp_reg_base[desc->reg_id + PPU_REG_RANGE * ppu_index] = tmp;

#ifdef LIBVA_DEBUG
  if(regiset_ofile == NULL) {
    pthread_mutex_init(&va_oflie_mutex, NULL);
    regiset_ofile = fopen("setReigsetValues.txt","wb");
  }
  pthread_mutex_lock(&va_oflie_mutex);
  u32 addr_value_workaround = 10000;
  if (value > addr_value_workaround)
    fprintf(regiset_ofile, "%-48s - 0x%x\n", registername[id], value);
  else
    fprintf(regiset_ofile, "%-48s - %9u\n", registername[id], value);
  fflush(regiset_ofile);
  pthread_mutex_unlock(&va_oflie_mutex);
#endif
}

u32 GetPpuRegister(const u32 *pp_reg_base, u32 id, int ppu_index) {
  u32 tmp;
  const HwRegDesc *desc;

  ASSERT(id < HWIF_LAST_REG);

  /* hw reg spec is based on decoder so we have to adjust the register offset
   * because we are getting PP base register as parameter
   */
  desc = &hw_dec_reg_spec[id];

  tmp = pp_reg_base[desc->reg_id + PPU_REG_RANGE * ppu_index];
  tmp = tmp >> desc->posi;
  tmp &= reg_mask[desc->bits];
  return (tmp);
}

/* FIXME: When registers are really defined, restore two these two functions.*/

#ifdef REG_ACCESS_OPT
static void GetDecRegNumbers(u32* reg_count, u32* reg_offsets,
                             u32 writable_only, u32 ppu_count) {
  u32 found;
  u32 buff[MAX_REG_COUNT] = {0};
  u32 *p = buff;
  const HwRegDesc *desc;

  /* For all registers, put swreg0 into count, so that the search for swreg0 can
     be optimized, since swreg0 is always read only. */
  if (!writable_only) {
    *p++ = 0;
  }

  /* Loop through registers (as defined in enum HwIfName enumeration). */
  for (u32 reg = 0; reg != HWIF_LAST_REG; reg++) {
    desc = &hw_dec_reg_spec[reg];
    if (!writable_only || desc->rw == 1) {
      if (desc->reg_id == 0) continue;
      /* Loop may write single reg multiple time, even if we do this simple
       * check for the same reg multiple times in a row. */
      found = 0;
      /* If there is a reg defined multiple times, reverse search will find it
         more quickly. */
      for (i32 i = reg - 1; i >= 0; i--) {
        if (desc->reg_id == hw_dec_reg_spec[i].reg_id) {
          found = 1; break;
        }
      }
      if (found) continue;
      *p++ = desc->reg_id;
    }
  }
  /* registers for extra pp units */
  for (u32 ppu = 1; ppu < ppu_count; ppu++) {
    for (u32 reg = HWIF_PPX_START; reg <= HWIF_PPX_END; reg++) {
      desc = &hw_dec_reg_spec[reg];
      if (!writable_only || desc->rw == 1) {
        if (desc->reg_id == 0) continue;
        /* Loop may write single reg multiple time, even if we do this simple
         * check for the same reg multiple times in a row. */
        found = 0;
        /* If there is a reg defined multiple times, reverse search will find it
           more quickly. */
        for (i32 i = reg - 1; i >= 0; i--) {
          if (desc->reg_id == hw_dec_reg_spec[i].reg_id) {
            found = 1; break;
          }
        }
        if (found) continue;
        *p++ = desc->reg_id + ppu * PPU_REG_RANGE;
      }
    }
  }
  *reg_count = p - buff;
  DWLmemcpy(reg_offsets, buff, *reg_count * sizeof(u32));
}

void FlushDecRegisters(const void* dwl, i32 core_id, u32* regs, u32 ppu_count) {
  static u32 reg_count = MAX_REG_COUNT + 1;
  static u32 reg_offsets[MAX_REG_COUNT] = {0};

#ifdef TRACE_START_MARKER
  /* write ID register to trigger logic analyzer */
  DWLWriteReg(dwl, dec_cont->core_id, 0x00, ~0);
#endif

  if (reg_count == MAX_REG_COUNT + 1)
    GetDecRegNumbers(&reg_count, reg_offsets, /* Writable regs only */1, ppu_count);
  for (u32 i = 0; i < reg_count; i++)
    DWLWriteReg(dwl, core_id, reg_offsets[i] * 4, regs[reg_offsets[i]]);
}

void RefreshDecRegisters(const void* dwl, i32 core_id, u32* regs, u32 ppu_count) {
  static u32 reg_count = MAX_REG_COUNT + 1;
  static u32 reg_offsets[MAX_REG_COUNT] = {0};

  if (reg_count == MAX_REG_COUNT + 1)
    GetDecRegNumbers(&reg_count, reg_offsets, /* All regs */0, ppu_count);
  for (u32 i = 0; i < reg_count; i++)
    regs[reg_offsets[i]] = DWLReadReg(dwl, core_id, reg_offsets[i] * 4);
}
#else
void FlushDecRegisters(const void* dwl, i32 core_id, u32* regs, u32 max_regs) {
  for (u32 i = 0; i <= max_regs; i++)
    DWLWriteReg(dwl, core_id, i * 4, regs[i]);
}

void RefreshDecRegisters(const void* dwl, i32 core_id, u32* regs, u32 max_regs) {
  for (u32 i = 0; i <= max_regs; i++)
    regs[i] = DWLReadReg(dwl, core_id, i * 4);
}
#endif
