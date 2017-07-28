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

//============================================================================
// Example:
//     play wav file from microSD card
//============================================================================

#include <M10SRAM.h>

#include <M10SD.h>

#include <M10CODEC.h>

//============================================================================
// Set the wav file name here
//============================================================================
const uint8_t wav_file_name [] = "SPIDER.WAV";

// The first 128KB samples will also be loaded into SRAM
// if PLAY_FROM_SD_CARD is not defined, samples will be played 
// from the SRAM instead of reading directly from microSD card.
#define PLAY_FROM_SD_CARD

//----------------------------------------------------------------------------
// get_u16()
//
// Parameters:
//      ptr    : pointer to BYTE
//
// Return Value:
//      (*ptr) + ((*(ptr+1)) << 8)
//
// Remarks:
//      get a word from the pointer
//----------------------------------------------------------------------------

uint16_t get_u16 (uint8_t *ptr)
{
  uint16_t t;
  t = (uint16_t)(*ptr);
  ++ptr;
  t += (uint16_t)(*ptr) << 8;

  return t;
} // get_u16()


//----------------------------------------------------------------------------
// get_u32()
//
// Parameters:
//      ptr    : pointer to BYTE
//
// Return Value:
//      (*ptr) + ((*(ptr+1)) << 8) + ((*(ptr+2)) << 16) + ((*(ptr+3)) << 24) 
//
// Remarks:
//      get a DWORD from the pointer
//----------------------------------------------------------------------------

uint32_t get_u32(uint8_t *ptr)
{
  uint16_t t;
  uint32_t ret;
  
  ret = get_u16(ptr);
  
  ptr += 2;
  
  t = get_u16(ptr);

  ret += (uint32_t)t << 16;
  
  return ret;  
} // End of get_u32()




//----------------------------------------------------------------------------
// sample buffer and scratch buffer
//----------------------------------------------------------------------------

uint8_t wav_samp_buffer [256];
uint8_t wav_samp_scratch_buffer[256];



//----------------------------------------------------------------------------
// str_cmp_case_insensitive()
//
// Parameters:
//      a    : pointer to buffer A
//      b    : pointer to buffer B
//      cnt  : buffer size to compare 
//
// Return Value:
//      return 0 if buffer A and buffer B are the same (case insensitive)
//
// Remarks:
//      compare byte buffer with case insensitive
//----------------------------------------------------------------------------

uint8_t str_cmp_case_insensitive (uint8_t *a, uint8_t *b, uint8_t cnt)
{
  uint8_t i, ta, tb;

  for (i = 0; i < cnt; ++i) {
    ta = a[i]; 
    tb = b[i];

    if ((ta >= 'a') && (ta <= 'z')) {
      ta = ta - 'a' + 'A';
    }

    if ((tb >= 'a') && (tb <= 'z')) {
      tb = tb - 'a' + 'A';
    }

    if (ta != tb) {
      return 1;
    }
  }

  return 0;
  
} // End of str_cmp_case_insensitive()


//----------------------------------------------------------------------------
// read_file()
//
// Parameters:
//      cnt  : buffer size to read
//      buf  : string to compare against (set it to 0 for no string comparison
//
// Return Value:
//      return 0 if data is read ok and is the same as the buf (case insensitive)
//
// Remarks:
//      read data into wav_samp_buffer, and compare it against buf
//----------------------------------------------------------------------------

uint8_t read_file(uint8_t cnt, uint8_t *buf)
{
  uint16_t br, btr;
  uint8_t res;

  btr = cnt; br = 0;
  res = SD.fread(wav_samp_buffer, btr, &br);
  if ((res) || (btr != br)) {
    Serial.write ("wav read error\n");
    return 0xff;
  }

  if(buf) {
      res = str_cmp_case_insensitive(wav_samp_buffer, buf, btr);
    
      if (res) {
        Serial.write ("!!!!!!!! not match the subtrunk of");
        Serial.write (buf, cnt);
        Serial.write ("\n");
        return 0xff;
      } else {
        Serial.write ("==> match ");
        Serial.write (buf, cnt);
        Serial.write ("\n");
      }
  }

  return 0;
  
} // End of read_file()


//----------------------------------------------------------------------------
// flags for wav file
//----------------------------------------------------------------------------

uint16_t block_align;
uint16_t num_of_channels;
uint32_t num_of_samples;
uint8_t wav_file_header_size;
uint8_t valid_wav_file = 0;


//----------------------------------------------------------------------------
// parse_wav_file_head()
//
// Parameters:
//      None
//
// Return Value:
//      return 0 if wav file head is parsed ok 
//
// Remarks:
//      parse wav file head, only mono 8KHz sample rate is supported.
//----------------------------------------------------------------------------

uint8_t parse_wav_file_head ()
{
  uint16_t t;
  uint8_t res;
  uint32_t subchunk_size, sample_rate;
  
  uint8_t cnt = 0;

  Serial.write ("==========================================\n");
  Serial.write (" Start parsing head for wav file\n");
  Serial.write ("==========================================\n");
  

  // chunk_ID
      read_file (4, "RIFF");
      cnt += 4;

  // chunk_size 
      read_file (4, 0);
      cnt += 4;
      
  // format 
      read_file (4, "WAVE");
      cnt += 4;
      
  // subchunk1_ID
      read_file (4, "fmt ");
      cnt += 4;
      
  // subchunk1_size
      read_file (4, 0);
      cnt += 4;
      subchunk_size = get_u32(wav_samp_buffer);
      Serial.write ("subchunk1_size ");
      Serial.println(subchunk_size);


  // audio_format
      read_file (2, 0);
      cnt += 2;
      Serial.write ("audio format ");
      Serial.println(get_u16(wav_samp_buffer));

  // num_of_channels
      read_file (2, 0);
      cnt += 2;
      num_of_channels = get_u16(wav_samp_buffer);
      Serial.write ("num_of_channels ");
      Serial.println(num_of_channels);

      if (num_of_channels != 1) {
        Serial.write ("only mono (single channel) is supported!\n");
        return 0xff;  
      }

  // sample_rate 
      read_file (4, 0);
      cnt += 4;
      sample_rate = get_u32(wav_samp_buffer);
      Serial.write ("sample_rate ");
      Serial.println(sample_rate);

      if (sample_rate != 8000) {
        Serial.write ("not 8KHz sample rate!\n");
        return 0xff;
      }

  // byte_rate
      read_file (4, 0);
      cnt += 4;
      Serial.write ("byte_rate ");
      Serial.println(get_u32(wav_samp_buffer));

  // block_align
      read_file (2, 0);
      cnt += 2;
      block_align = get_u16(wav_samp_buffer);
      Serial.write ("block_align ");
      Serial.println(block_align);


  // bits_per_sample
      read_file (2, 0);
      cnt += 2;
      Serial.write ("bits_per_sample ");
      Serial.println(get_u16(wav_samp_buffer));

  // read extra
      if ((20 + subchunk_size) > cnt) {
        t = 20 + subchunk_size - cnt;
        Serial.write ("read extra pad ");
        Serial.print(t);
        Serial.write (" bytes\n");
        read_file (t, 0);
      }
      
 // data
      t = 4;
      do { 
       res = read_file (4, "data");
       cnt += 4; 
       read_file (4, 0);
       cnt += 4;
       subchunk_size = get_u32(wav_samp_buffer);

       if (res) {
          Serial.write ("skip this subtrunk\n");
          read_file (subchunk_size, 0);
          cnt += subchunk_size;
       }

       --t;
       
      } while (res && t);

      if (!t) {
        Serial.write ("can not find data subtrunk\n");
        return 0xff; 
      }

      wav_file_header_size = cnt;

      Serial.write ("wav_file_header_size ");
      Serial.println(wav_file_header_size);
      
      num_of_samples = subchunk_size / block_align;
      Serial.write ("num_of_samples = ");
      Serial.println(num_of_samples);


  Serial.write ("==========================================\n");
  Serial.write (" End parsing the head for wav file\n");
  Serial.write ("==========================================\n");
  
  return 0;
  
} // End of parse_wav_file_head()

//----------------------------------------------------------------------------
// load_wav_into_SRAM()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      load wav file samples (The first 128KB) into SRAM
//----------------------------------------------------------------------------

void load_wav_into_SRAM()
{
  uint32_t i, max_num_of_samp, addr;
  uint16_t sample;
  uint8_t t, res;

  if (!valid_wav_file) {
    return;
  }

  if (num_of_samples > 65536) {
    max_num_of_samp = 65536;
  } else {
    max_num_of_samp = num_of_samples;
  }

  addr = 0;
  for (i = 0; i < max_num_of_samp; ++i) {
    res = read_file (block_align, 0);
    if (res) {
      break;
    }

    sample = get_u16(wav_samp_buffer);
    
    t = sample & 0xFF;
    SRAM.write(addr, t);
    
    t = (sample >> 8) & 0xFF;
    ++addr;
          
    SRAM.write(addr, t);
    ++addr;
  }
  
} // End of load_wav_into_SRAM()


//----------------------------------------------------------------------------
// read/write pointer for wav sample buffer
//----------------------------------------------------------------------------

uint8_t wav_buf_read_pointer;
uint8_t wav_buf_write_pointer;



//----------------------------------------------------------------------------
// codec_isr_handler()
//
// Parameters:
//      None
//
// Return Value:
//      None
//
// Remarks:
//      isr handler for CODEC (IRQ 6)
//----------------------------------------------------------------------------

void codec_isr_handler()
{
  uint8_t high, low;

  if (wav_buf_read_pointer == wav_buf_write_pointer) {
    return;
  }
  
  low = wav_samp_buffer[wav_buf_read_pointer++];
  high = wav_samp_buffer[wav_buf_read_pointer++];

  CODEC_WRITE_DATA_HIGH = high;
  CODEC_WRITE_DATA_LOW = low & 0xFE;

} // End of codec_isr_handler()



//============================================================================
// setup()
//============================================================================

void setup() {
    uint8_t res;

    delay(1000);
    Serial.begin(115200);
    delay(1000);
  
    attachIsrHandler(CODEC_INT_INDEX, codec_isr_handler);
  
    SRAM.begin();
    CODEC.begin();
  

    Serial.write("Example: Wav File Player\n");
    Serial.write (" File Name: ");
    Serial.write(wav_file_name);
    Serial.write("\n");

    
    res = SD.begin();

    if (res) {
      Serial.write("SD init fail\n");
    } else {
      Serial.write("SD init success\n");
    }

    res = SD.fopen(wav_file_name);

    if (res) {
      Serial.write("SD open fail\n");
    } else {
      Serial.write("SD open success\n");
    }
  
    res = parse_wav_file_head();

  
    if (res) {
      valid_wav_file = 0;
      Serial.write("\n wav file open error\n");
    } else {
      valid_wav_file = 1;
      Serial.write ("\n Playing ... \n");
    }

  
    load_wav_into_SRAM();

    wav_buf_read_pointer = 0;
    wav_buf_write_pointer = 0;
  
} // End of setup()

//============================================================================
// loop()
//============================================================================

void loop() {

  uint8_t t,res;
  
#ifdef PLAY_FROM_SD_CARD  // play directly from microSD card
  uint8_t delta;
  uint8_t step_size = 64;
  uint16_t br = 0, btr = 0;

#else // play from SRAM

  uint32_t i, max_num_of_samp, addr;
  uint16_t sample;

#endif
  
  if (!valid_wav_file) {
    return;
  }


  res = SD.fopen(wav_file_name);
  read_file (wav_file_header_size, 0);


#ifdef PLAY_FROM_SD_CARD // play directly from microSD card 


  do {

      t = wav_buf_read_pointer;

      delta = wav_buf_write_pointer - t;
      
      if (delta < step_size) {
        btr = step_size; br = 0;
        res = SD.fread(wav_samp_scratch_buffer, btr, &br);
 
        
        for (t = 0; t < br; ++t) {
 
            wav_samp_buffer[wav_buf_write_pointer] =  wav_samp_scratch_buffer[t];
            ++wav_buf_write_pointer;
        } // End of for loop
      }
 
  
  
  } while ((!res) && (br == btr));

#else  // play from SRAM


  if (num_of_samples > 65536) {
    max_num_of_samp = 65536;
  } else {
    max_num_of_samp = num_of_samples;
  }

  t = 0;
  addr = 0;
  for (i = 0; i < max_num_of_samp; ++i) {
  
   
    t = SRAM.read(addr);
    sample = t;

    ++addr;
    t = SRAM.read(addr);
    sample += (uint16_t)t << 8;

    ++addr;
    
    CODEC.sampleWrite(sample);
    
  } // End of for loop

#endif

  Serial.write (" loop over\n");

} // End of loop()

