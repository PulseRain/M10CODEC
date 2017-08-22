/*
###############################################################################
# Copyright (c) 2017, PulseRain Technology LLC 
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License (LGPL) as 
# published by the Free Software Foundation, either version 3 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but 
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY 
# or FITNESS FOR A PARTICULAR PURPOSE.  
# See the GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
###############################################################################
*/

#ifndef M10CODEC_H
#define M10CODEC_H

#include "Arduino.h"

//=======================================================================
// Si3000 Library
//=======================================================================

#define SI3000_CONTROL_1  1
#define   SI3000_SR_RESET     (1 << 7)
#define   SI3000_SR_NORMAL    (0 << 7)

#define   SI3000_SPD_NORMAL   (1 << 4)
#define   SI3000_SPD_PWR_DOWN (0 << 4)

#define   SI3000_LPD_NORMAL   (1 << 3)
#define   SI3000_LPD_PWR_DOWN (0 << 3)

#define   SI3000_HPD_NORMAL   (1 << 2)
#define   SI3000_HPD_PWR_DOWN (0 << 2)

#define   SI3000_MPD_PWR_DOWN (1 << 1)
#define   SI3000_MPD_NORMAL   (0 << 1)

#define   SI3000_CPD_PWR_DOWN (1 << 0)
#define   SI3000_CPD_NORMAL   (0 << 0)

#define SI3000_CONTROL_2  2
#define   SI3000_HPFD_DISABLE  (1 << 4)
#define   SI3000_HPFD_ENABLE   (0 << 4)

#define   SI3000_PLL_DIV_10    (1 << 3)
#define   SI3000_PLL_DIV_5     (0 << 3)

#define   SI3000_DL1_EN        (1 << 2)
#define   SI3000_DL1_NORMAL    (0 << 2)

#define   SI3000_DL2_EN        (1 << 1)
#define   SI3000_DL2_NORMAL    (0 << 1)

#define SI3000_PLL1_DIV_N1  3

#define SI3000_PLL1_MUL_M1  4

#define SI3000_RX_GAIN_CONTROL_1 5
#define    SI3000_LIG_20DB  (3 << 6)
#define    SI3000_LIG_10DB  (2 << 6)
#define    SI3000_LIG_0DB   (1 << 6)

#define    SI3000_LIM_MUTE  (1 << 5)

#define    SI3000_MCG_30DB  (3 << 3)
#define    SI3000_MCG_20DB  (2 << 3)
#define    SI3000_MCG_10DB  (1 << 3)
#define    SI3000_MCG_0DB   (0 << 3)

#define    SI3000_MCM_MUTE  (1 << 2)
#define    SI3000_HIM_MUTE  (1 << 1)

#define    SI3000_IIR_ENABLE  (1 << 0)
#define    SI3000_FIR_ENABLE  (0 << 0)


#define SI3000_ADC_VOL_CONTROL 6
#define    SI3000_RXG_12DB     (0x1F << 2)
#define    SI3000_RXG_0DB      (0x17 << 2)
#define    SI3000_RXG_M34P5DB  (0x0 << 2)


#define    SI3000_LOM_MUTE    (0 << 1)
#define    SI3000_LOM_ACTIVE  (1 << 1)

#define    SI3000_HOM_MUTE    (0 << 0)
#define    SI3000_HOM_ACTIVE  (1 << 0)

#define SI3000_DAC_VOL_CONTROL 7
#define    SI3000_TXG_12DB     (0x1F << 2)
#define    SI3000_TXG_6DB      (0x1B << 2)
#define    SI3000_TXG_0DB      (0x17 << 2)
#define    SI3000_TXG_M34P5DB  (0x0 << 2)


#define    SI3000_SLM_MUTE    (0 << 1)
#define    SI3000_SLM_ACTIVE  (1 << 1)

#define    SI3000_SRM_MUTE    (0 << 0)
#define    SI3000_SRM_ACTIVE  (1 << 0)


#define SI3000_STATUS_REPORT 8
#define    SI3000_SLSC     (1 << 7)
#define    SI3000_SRSC     (1 << 6)
#define    SI3000_LOSC     (1 << 5)

#define SI3000_ANALOG_ATTEN 9

#define    SI3000_LOT_M18DB     (3 << 2)
#define    SI3000_LOT_M12DB     (2 << 2)
#define    SI3000_LOT_M6DB      (1 << 2)
#define    SI3000_LOT_0DB       (0 << 2)

#define    SI3000_SOT_M18DB     (3 << 0)
#define    SI3000_SOT_M12DB     (2 << 0)
#define    SI3000_SOT_M6DB      (1 << 0)
#define    SI3000_SOT_0DB       (0 << 0)


#define SI3000_RESET_N_SHIFT    7
#define SI3000_PRI_PULSE_SHIFT  6
#define SI3000_SEC_PULSE_SHIFT  5
#define SI3000_DONE_SHIFT       4
#define SI3000_DATA_AVAIL_SHIFT 3
#define SI3000_WR_BUSY_SHIFT    2
#define SI3000_ENABLE_SHIFT     1
#define SI3000_SYNC_RESET_SHIFT 0

#define SI3000_WR_BUSY_FLAG     (1 << SI3000_WR_BUSY_SHIFT)
#define SI3000_DATA_AVAIL_FLAG  (1 << SI3000_DATA_AVAIL_SHIFT)    
#define SI3000_ENABLE           (1 << SI3000_ENABLE_SHIFT)
#define SI3000_DONE_FLAG        (1 << SI3000_DONE_SHIFT)

typedef struct {
  void (*begin)() __reentrant;
  uint16_t (*regRead)(uint8_t addr) __reentrant;
  void (*regWrite) (uint8_t addr, uint16_t data) __reentrant;
  uint16_t (*sampleRead)() __reentrant;
  void (*sampleWrite)(uint16_t data) __reentrant;
  uint8_t (*sampleCompress)(int16_t  pcm_val) __reentrant;
  int16_t (*sampleExpand)(uint8_t u_val) __reentrant;
  void (*outputVolume)(uint8_t volume) __reentrant;
} M10CODEC_STRUCT;

extern const M10CODEC_STRUCT CODEC;

#endif

