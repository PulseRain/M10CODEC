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

#include "M10CODEC.h"

//----------------------------------------------------------------------------
// Si3000_reg_read()
//
// Parameters:
//      addr : CSR address
//
// Return Value:
//      CSR register value
//
// Remarks:
//      Function to read the register of Si3000
//----------------------------------------------------------------------------

static uint16_t Si3000_reg_read (uint8_t addr) __reentrant
{
   uint8_t low, high;

   low  = CODEC_READ_DATA_LOW;
   CODEC_WRITE_DATA_HIGH = (addr & 0x1F) | 0x20 ;

   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");
   while(!(CODEC_CSR & SI3000_DATA_AVAIL_FLAG));
  
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");

   
   while(CODEC_CSR & SI3000_WR_BUSY_FLAG);
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");
   
   CODEC_WRITE_DATA_LOW = 0x1;
   
   __asm__ ("nop");
   __asm__ ("nop");
  
   low  = CODEC_READ_DATA_LOW;
 
   while(CODEC_CSR & SI3000_WR_BUSY_FLAG);
   CODEC_WRITE_DATA_LOW = 0;

   while(!(CODEC_CSR & SI3000_DATA_AVAIL_FLAG));
  
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");

   low  = CODEC_READ_DATA_LOW;
  
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");

    while(!(CODEC_CSR & SI3000_DATA_AVAIL_FLAG));
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");

   high = CODEC_READ_DATA_HIGH;
   low  = CODEC_READ_DATA_LOW;
  
   return ((uint16_t)((high << 8) + low)); 
} // End of Si3000_reg_read()

//----------------------------------------------------------------------------
// Si3000_reg_write()
//
// Parameters:
//      addr : CSR address
//      data : 16 bit data to write
//
// Return Value:
//      None
//
// Remarks:
//      Function to write the register of Si3000
//----------------------------------------------------------------------------

static void Si3000_reg_write (uint8_t addr, uint16_t data) __reentrant
{
   while(CODEC_CSR & SI3000_WR_BUSY_FLAG);
   CODEC_WRITE_DATA_HIGH = (addr & 0x1F);
   CODEC_WRITE_DATA_LOW = 0x1;
   
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");
   
   while(CODEC_CSR & SI3000_WR_BUSY_FLAG);
   CODEC_WRITE_DATA_LOW = data;
   
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");
   

   while(CODEC_CSR & SI3000_WR_BUSY_FLAG);
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");
   CODEC_WRITE_DATA_HIGH = 0;
   CODEC_WRITE_DATA_LOW = 0;
   __asm__ ("nop");
   __asm__ ("nop");
   __asm__ ("nop");
   
  while(CODEC_CSR & SI3000_WR_BUSY_FLAG);
   
  // delay (100);
    
} // End of Si3000_reg_write()

//----------------------------------------------------------------------------
// Si3000_init()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      Function to initialize Si3000
//----------------------------------------------------------------------------

static void Si3000_init() __reentrant
{
   CODEC_CSR = (1 << SI3000_RESET_N_SHIFT) | SI3000_ENABLE;
   delay (100);
   CODEC_CSR = (0 << SI3000_RESET_N_SHIFT) | SI3000_ENABLE;
   delay (100);
   CODEC_CSR = (1 << SI3000_RESET_N_SHIFT) | SI3000_ENABLE ;
   delay (100);
   CODEC_CSR = (1 << SI3000_RESET_N_SHIFT) | SI3000_ENABLE;
   
   // MCLK = 4MHz, 8kHz sample rate
   Si3000_reg_write (SI3000_PLL1_DIV_N1, 24);
   Si3000_reg_write (SI3000_PLL1_MUL_M1, 255);
    
   Si3000_reg_write (SI3000_CONTROL_1, SI3000_SPD_NORMAL);
   Si3000_reg_write (SI3000_CONTROL_2, SI3000_HPFD_ENABLE | SI3000_PLL_DIV_5 | SI3000_DL1_NORMAL | SI3000_DL2_NORMAL);
   
   //Si3000_reg_write (SI3000_RX_GAIN_CONTROL_1, SI3000_LIG_20DB | SI3000_MCM_MUTE | SI3000_HIM_MUTE | SI3000_IIR_ENABLE);
    Si3000_reg_write (SI3000_RX_GAIN_CONTROL_1, SI3000_LIM_MUTE | SI3000_MCG_30DB | SI3000_HIM_MUTE | SI3000_FIR_ENABLE);
   
   Si3000_reg_write (SI3000_ADC_VOL_CONTROL, SI3000_RXG_12DB | SI3000_LOM_MUTE | SI3000_HOM_MUTE);
   Si3000_reg_write (SI3000_DAC_VOL_CONTROL, SI3000_TXG_6DB | SI3000_SLM_ACTIVE | SI3000_SRM_ACTIVE);
   Si3000_reg_write (SI3000_ANALOG_ATTEN, SI3000_LOT_M18DB | SI3000_SOT_M18DB);
   
} // End of Si3000_init()

//----------------------------------------------------------------------------
// Si3000_volume()
//
// Parameters:
//      None
//
// Return Value:
//      value to set the volume. (from 0 to 32). 0 to mute the output. 
//
// Remarks:
//      Function to set the output volume 
//----------------------------------------------------------------------------

static void Si3000_volume(uint8_t volume) __reentrant
{
    if (volume == 0) {
        Si3000_reg_write (SI3000_DAC_VOL_CONTROL, SI3000_SLM_MUTE | SI3000_SLM_MUTE);
    } else {
        --volume;
        if (volume > 31) {
            volume = 31;
        }
        volume = volume << 2;
        Si3000_reg_write (SI3000_DAC_VOL_CONTROL, SI3000_SLM_ACTIVE | SI3000_SRM_ACTIVE | volume);
    }

} // End of Si3000_volume()


//----------------------------------------------------------------------------
// Si3000_sample_write()
//
// Parameters:
//      data : 16 bit sample to write
//
// Return Value:
//      None
//
// Remarks:
//      Function to write 16 bit 2's complementary sample to Si3000. 
//      (The LSB will be set to zero)
//----------------------------------------------------------------------------

static void Si3000_sample_write (uint16_t data) __reentrant
{
   while(CODEC_CSR & SI3000_WR_BUSY_FLAG);
   CODEC_WRITE_DATA_HIGH = ((data >> 8) & 0xFF);
   CODEC_WRITE_DATA_LOW = data & 0xFE; // LSB has to be zero for primary frame

} // End of Si3000_sample_write()

//----------------------------------------------------------------------------
// Si3000_sample_read()
//
// Parameters:
//      None
//
// Return Value:
//      16 bit sample from Si3000
//
// Remarks:
//      Function to read sample from Si3000. 
//----------------------------------------------------------------------------

static uint16_t Si3000_sample_read () __reentrant
{
   uint8_t high, low;
   
   while(!(CODEC_CSR & SI3000_DATA_AVAIL_FLAG));

   high = CODEC_READ_DATA_HIGH;
   low  = CODEC_READ_DATA_LOW;

   return ((high << 8) + low);
    
} // End of Si3000_sample_read()


//==============================================================================
// Code adapted from g711.c, by Sun Microsystems, start 
//==============================================================================
/*
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use.  Users may copy or modify this source code without
 * charge.
 *
 * SUN SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 * THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS SOFTWARE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#define BIAS    (0x84)    /* Bias for linear code. */
#define CLIP     8159

static int16_t seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF};
 
//----------------------------------------------------------------------------
// Snack_Lin2Mulaw()
//
// Parameters:
//      pcm_val : 16 bit linear sample in 2's complementary
//
// Return Value:
//      8 bit Mu Law compressed data
//
// Remarks:
//      Compress by Mu Law. 
//----------------------------------------------------------------------------

static uint8_t Snack_Lin2Mulaw(int16_t  pcm_val) __reentrant  /* 2's complement (16-bit range) */
{
  int16_t   mask;
  int16_t   seg;
  uint8_t   uval, i;

  /* Get the sign and the magnitude of the value. */
  pcm_val = pcm_val >> 2;
  if (pcm_val < 0) {
    pcm_val = -pcm_val;
    mask = 0x7F;
  } else {
    mask = 0xFF;
  }
  
  if (pcm_val > CLIP ) {
    pcm_val = CLIP;   /* clip the magnitude */
  }
  
  pcm_val += (BIAS >> 2);

  /* Convert the scaled magnitude to segment number. */
  for (i = 0; i < 8; ++i) {
     if (pcm_val < seg_uend[i]) {
        break;
     }
  } // End of for loop

  if (i > 7) {
     i = 7;
  }

  seg = i;
  
  
  /*
   * Combine the sign, segment, quantization bits;
   * and complement the code word.
   */
  if (pcm_val > seg_uend[7]) {   /* out of range, return maximum value. */
    return (uint8_t) (0x7F ^ mask);
  } else {
    uval = (uint8_t) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
    return (uval ^ mask);
  }
  
} // End of Snack_Lin2Mulaw()

//----------------------------------------------------------------------------
// Snack_Mulaw2Lin()
//
// Parameters:
//      u_val : 8 bit Mu Law compressed data
//
// Return Value:
//      16 bit linear sample in 2's complementary
//
// Remarks:
//      expand the 8 bit Mu Law data to 16 bit linear sample 
//----------------------------------------------------------------------------

static int16_t Snack_Mulaw2Lin(uint8_t u_val) __reentrant
{
  int16_t t;
  uint16_t tmp;
  
  /* Complement to obtain normal u-law value. */
  u_val = ~u_val;

  /*
   * Extract and bias the quantization bits. Then
   * shift up by the segment number and subtract out the bias.
   */
  t = ((uint16_t)(u_val & 0xF) * 8) + BIAS;
  tmp = u_val & 0x70;
  tmp /= 16;
  
  t <<= tmp;

  if (u_val & 0x80) {
    return (BIAS - t);
  } else {
    return (t - BIAS);
  }
  
} // End of Snack_Mulaw2Lin()

static int16_t seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};

//----------------------------------------------------------------------------
// Snack_Lin2Alaw()
//
// Parameters:
//      pcm_val : 16 bit linear sample in 2's complementary
//
// Return Value:
//      8 bit A Law compressed data
//
// Remarks:
//      Compress by A Law. 
//----------------------------------------------------------------------------

static uint8_t Snack_Lin2Alaw(int16_t pcm_val) __reentrant	/* 2's complement (16-bit range) */
{
    uint8_t mask;
    uint8_t seg;
    uint8_t aval;
    uint8_t i;

    pcm_val = pcm_val / 8;

    if (pcm_val >= 0) {
        mask = 0xD5;    /* sign (7th) bit = 1 */
    } else {
        mask = 0x55;    /* sign bit = 0 */
        pcm_val = -pcm_val - 1;
    }

    /* Convert the scaled magnitude to segment number. */
    for (i = 0; i < 8; ++i) {
        if (pcm_val < seg_aend[i]) {
          break;
        }
    } // End of for loop

    if (i > 7) {
      i = 7;
    }
    
    seg = i;
    
    /* Combine the sign, segment, and quantization bits. */

    if (pcm_val > seg_aend[7]) {   /* out of range, return maximum value. */
        return (0x7F ^ mask);
    } else {
        aval = seg << 4;
        if (seg < 2) {
            pcm_val >>= 1;
            aval |= pcm_val & 0xF;
        } else {
            pcm_val >>= seg;
            aval |= pcm_val & 0xF;
        }
        return (aval ^ mask);
    }
} // End of Snack_Lin2Alaw()


//----------------------------------------------------------------------------
// Snack_Alaw2Lin()
//
// Parameters:
//      a_val : 8 bit A Law compressed data
//
// Return Value:
//      16 bit linear sample in 2's complementary
//
// Remarks:
//      expand the 8 bit A Law data to 16 bit linear sample 
//----------------------------------------------------------------------------

static int16_t Snack_Alaw2Lin(uint8_t a_val) __reentrant
{
    int16_t t;
    uint8_t seg;
    
    a_val ^= 0x55;
    
    t = (a_val & 0xF) << 4;
    seg = ((a_val & 0x70) >> 4) & 0xF;
        
    if (seg == 0) {
        t = t + 8;
    } else if (seg == 1) {
        t = t + 0x108;
    } else {
        t = t + 0x108;
        t = t << (seg - 1);
    }

    if (a_val & 0x80) {
        return t;
    } else {
        return (-t);
    }    
  
} // End of Snack_Alaw2Lin()

//==============================================================================
// Code adapted from g711.c, by Sun Microsystems, end 
//==============================================================================





//----------------------------------------------------------------------------
// constant initialization
//----------------------------------------------------------------------------

const M10CODEC_STRUCT CODEC = {Si3000_init,            // begin
                            Si3000_reg_read,          // regRead,
                            Si3000_reg_write,         // regWrite
                            Si3000_sample_read,       // sampleRead
                            Si3000_sample_write,      // sampleWrite
                            Snack_Lin2Mulaw,          // sampleCompress
                            Snack_Mulaw2Lin,          // sampleExpand
                            Si3000_volume             // outputVolume                            
};
