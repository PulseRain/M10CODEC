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
#include "M10MISC.h"
#include "M10SRAM.h"

//=======================================================================================
// Frame definition for communication between M10 and host PC
//=======================================================================================

#define FRAME_SYNC_2  0xBA
#define FRAME_SYNC_1  0xAB
#define FRAME_SYNC_0  0x11

#define FRAME_LENGTH 12
#define SYNC_LENGTH  3

#define EXT_FRAME_LENGTH (128 + FRAME_LENGTH)

#define FRAME_TYPE_MEM_WRITE_WORD  0x37
#define FRAME_TYPE_MEM_WRITE_BYTE  0x38
#define FRAME_TYPE_MEM_WRITE_EXT   0x39
#define FRAME_TYPE_MEM_READ_WORD   0x40
#define FRAME_TYPE_MEM_READ_EXT    0x41
#define FRAME_TYPE_CMD_PLAY        0x50
#define FRAME_TYPE_CMD_RECORD      0x52
#define FRAME_TYPE_CMD_VOLUME      0x54

#define FRAME_TYPE_ACK             0x34
#define FRAME_TYPE_ACK_EXT         0x35



//----------------------------------------------------------------------------
// Input frame buffer
//----------------------------------------------------------------------------

uint8_t input_data[EXT_FRAME_LENGTH];
uint8_t input_counter;

//----------------------------------------------------------------------------
// clear_input_data()
//
// Parameters:
//    None
//
// Remarks:
//    Function to clear input frame buffer
//----------------------------------------------------------------------------

void clear_input_data ()
{
  int i;

  for (i = 0; i < FRAME_LENGTH; ++i) {
    input_data[i] = 0;
  } // End of for loop

} // End of clear_input_data()

//----------------------------------------------------------------------------
// buffer for memory read
//----------------------------------------------------------------------------

#define READ_WRITE_BUFFER_SIZE (128 + 16)
uint8_t read_write_buffer[READ_WRITE_BUFFER_SIZE];

//----------------------------------------------------------------------------
// send_reply_back()
//
// Parameters:
//    m : 16 bit data
//    n : 32 bit data
//
// Remarks:
//    function to send (m, n) as the payload to reply
//----------------------------------------------------------------------------

void send_reply_back (uint16_t m, uint32_t n)
{
  uint8_t t;
  uint8_t* buf = read_write_buffer;
  uint16_t crc16;

  *buf++ = FRAME_SYNC_2;
  *buf++ = FRAME_SYNC_1;
  *buf++ = FRAME_SYNC_0;

  *buf++ = FRAME_TYPE_ACK;

  *buf++ = (m >> 8) & 0xFF;
  *buf++ = m & 0xFF;

  *buf++ = (n >> 24) & 0xFF;
  *buf++ = (n >> 16) & 0xFF;
  *buf++ = (n >> 8)  & 0xFF;
  *buf++ = n & 0xFF;


  Serial.write(read_write_buffer, FRAME_LENGTH - 2);

  crc16 = MISC.CRC16 (read_write_buffer, FRAME_LENGTH - 2);

  t = (uint8_t)((crc16 & 0xFF00) >> 8);
  Serial.write(&t, 1);

  t = (uint8_t)(crc16 & 0xFF);
  Serial.write(&t, 1);

} // End of send_reply_back()


uint32_t mem_addr = 0;
uint32_t play_addr = 0;
uint32_t record_addr = 0;

enum Input_FSM_State {
  INPUT_STATE_IDLE,
  INPUT_STATE_SYNC_1,
  INPUT_STATE_SYNC_0,
  INPUT_STATE_INPUT_WAIT,
  INPUT_STATE_INPUT_WAIT_EXT,
  INPUT_STATE_FRAME_TYPE
};

//----------------------------------------------------------------------------
// input_FSM()
//
// Parameters:
//    input : 8 bit data input from the UART
//    reset : 1 to reset the FSM
//
// Remarks:
//    function to decode the incoming frame from UART
//----------------------------------------------------------------------------

uint8_t input_FSM(uint8_t input, uint8_t reset)
{
    static enum Input_FSM_State state = INPUT_STATE_IDLE;

    static uint8_t frame_type = 0;
    uint8_t loop_continue;
    uint16_t crc_received;
    uint32_t start_addr;
    uint8_t data_high;
    uint8_t data_low;
    uint8_t i, t;
    uint8_t* buf;
    uint16_t crc16;
    uint8_t ret = 0;
    
    if (reset) {
      state = INPUT_STATE_IDLE;
      clear_input_data();
    } else {

      do {
        loop_continue = 0;

        switch (state) {
          case INPUT_STATE_IDLE:
            input_counter = 0;

            if (input == FRAME_SYNC_2) {
                state = INPUT_STATE_SYNC_1;
                clear_input_data();
                input_data [input_counter++] = input;
            }
  
            break;
  
          case INPUT_STATE_SYNC_1:

            if (input == FRAME_SYNC_1) {
                state = INPUT_STATE_SYNC_0;
                input_data [input_counter++] = input;
            } else {
                state = INPUT_STATE_IDLE;
            }
  
            break;
  
          case INPUT_STATE_SYNC_0:
            
            if (input == FRAME_SYNC_0) {
                state = INPUT_STATE_INPUT_WAIT;
                input_data [input_counter++] = input;
            } else {
                state = INPUT_STATE_IDLE;
            }
  
            break;
  
          case INPUT_STATE_INPUT_WAIT:

            input_data [input_counter++] = input;
            frame_type  = input_data[SYNC_LENGTH];
  
            if (input_counter == FRAME_LENGTH) {

              
              if (frame_type  == FRAME_TYPE_MEM_WRITE_EXT) {
                state = INPUT_STATE_INPUT_WAIT_EXT;
              } else {
                
                state = INPUT_STATE_FRAME_TYPE;
                loop_continue = 1;
              }
            }
  
            break;
  
          case INPUT_STATE_INPUT_WAIT_EXT:
            input_data [input_counter++] = input;
  
            if   ((input_counter == (EXT_FRAME_LENGTH - 2)) && (frame_type == FRAME_TYPE_MEM_WRITE_EXT)) {
                state = INPUT_STATE_FRAME_TYPE;
                loop_continue = 1;
            }
  
            break;
  
          case INPUT_STATE_FRAME_TYPE:
  
            if (frame_type == FRAME_TYPE_MEM_WRITE_EXT) {
              crc16 = MISC.CRC16(input_data, EXT_FRAME_LENGTH - 4);
              crc_received = (input_data [EXT_FRAME_LENGTH - 4] << 8) + input_data [EXT_FRAME_LENGTH - 3];
            } else {
              crc16 = MISC.CRC16(input_data, FRAME_LENGTH - 2);
              crc_received = (input_data [FRAME_LENGTH - 2] << 8) + input_data [FRAME_LENGTH - 1];
            }
  
            start_addr = ((uint32_t)input_data [SYNC_LENGTH + 1] << 24) +
                         ((uint32_t)input_data [SYNC_LENGTH + 2] << 16) +
                         ((uint32_t)input_data [SYNC_LENGTH + 3] << 8) +
                         ((uint32_t)input_data [SYNC_LENGTH + 4]);
  
            data_high  = input_data [SYNC_LENGTH + 5];
            data_low   = input_data [SYNC_LENGTH + 6];

            if ((frame_type  == FRAME_TYPE_CMD_VOLUME) && (crc_received == crc16)) {
                CODEC.outputVolume (data_low);
                send_reply_back (0xbeef, start_addr);
            } else if ((frame_type  == FRAME_TYPE_MEM_WRITE_BYTE) && (crc_received == crc16))  {
                SRAM.write(start_addr, data_low);
                
                send_reply_back ( data_low, start_addr);
               
            } else if ((frame_type  == FRAME_TYPE_MEM_WRITE_WORD) && (crc_received == crc16))  {
                SRAM.write(start_addr, data_low);
                SRAM.write(start_addr + 1, data_high);
                
                send_reply_back (data_high * 256 + data_low, start_addr);
               
            } else if ((frame_type  == FRAME_TYPE_MEM_READ_WORD) && (crc_received == crc16))  {
               
                data_low  = SRAM.read(start_addr);
                data_high = SRAM.read(start_addr + 1);
                send_reply_back (data_high * 256 + data_low, start_addr);
            } else if ((frame_type  == FRAME_TYPE_MEM_WRITE_EXT) && (crc_received == crc16))  {

                for (i = 0; i < (EXT_FRAME_LENGTH - FRAME_LENGTH); ++i) {
                    SRAM.write(start_addr + i, input_data [SYNC_LENGTH + 5 + i]);
                } // End of for loop

                send_reply_back (0xabcd, start_addr);
                mem_addr = start_addr;
                
            } else if ((frame_type  == FRAME_TYPE_MEM_READ_EXT) && (crc_received == crc16))  {
                                   
                buf = read_write_buffer;

                *buf++ = FRAME_SYNC_2;
                *buf++ = FRAME_SYNC_1;
                *buf++ = FRAME_SYNC_0;

                *buf++ = FRAME_TYPE_ACK_EXT;

                for (i = 0; i < 128; ++i) {
                  *buf++ = SRAM.read(start_addr++);
                } // End of for loop

                crc16 = MISC.CRC16 (read_write_buffer, 4 + 128);
  
                Serial.write(read_write_buffer, 4 + 128);
    
                t = (uint8_t)((crc16 & 0xFF00) >> 8);
                Serial.write(&t, 1);

                t = (uint8_t)(crc16 & 0xFF);
                Serial.write(&t, 1);
                    
            } else if ((frame_type  == FRAME_TYPE_CMD_PLAY) && (crc_received == crc16))  {
                send_reply_back (0xabcd, start_addr);

                ret = FRAME_TYPE_CMD_PLAY;
            } else if ((frame_type  == FRAME_TYPE_CMD_RECORD) && (crc_received == crc16))  {
                send_reply_back (0xabcd, start_addr);

                ret = FRAME_TYPE_CMD_RECORD;
            }
            
            state = INPUT_STATE_IDLE;
  
            default:
              break;
         } // End of switch
      } while (loop_continue);
   }

   return ret;
   
} // End of Input_FSM()

//----------------------------------------------------------------------------
// ascii_to_digital()
//
// Parameters:
//    ascii : 8 bit data input from the UART
//
// Remarks:
//    function to convert the incoming ascii into display-able digit
//----------------------------------------------------------------------------

uint8_t ascii_to_digital (uint8_t ascii)
{
    if (ascii >= 'A') {
        return (ascii - 'A' + 10);
    } else {
        return (ascii - '0');
    }
} // End of ascii_to_digital()


enum FSM_State {
    STATE_IDLE,

  
    STATE_INIT,
    STATE_UART,
    STATE_PLAY,
    STATE_RECORD
};

// flag to use Mu Law compression
uint8_t compressedFlag = 0;

// default output volume
uint8_t volume = 28;


//----------------------------------------------------------------------------
// FSM()
//
// Parameters:
//    None
//
// Remarks:
//    function to run the FSM for record/play etc.
//----------------------------------------------------------------------------

void FSM()
{
    static enum FSM_State state = STATE_INIT;
    uint8_t  t, ret;
    uint8_t  low, high;
    uint16_t data;
   
    switch (state) {
            
      
      
        case STATE_INIT:
            mem_addr = 0;
            input_FSM (0, 1);
            state = STATE_UART; 
           
            break;
           
        case STATE_UART:
               
            if (Serial.available()) {
                t = Serial.read();
                ret = input_FSM(t, 0);

                if (ret == FRAME_TYPE_CMD_PLAY) {
                  play_addr = 0;
                  state = STATE_PLAY;
                } else if (ret == FRAME_TYPE_CMD_RECORD) {
                  record_addr = 0;
                  state = STATE_RECORD;
                }
            } 
           
            break;

        case STATE_PLAY:

            if (!compressedFlag) {
                low =  SRAM.read (play_addr++);
                high = SRAM.read (play_addr++);
                data = (high << 8) + low;
            } else {
                t = SRAM.read (play_addr++);
                data = CODEC.sampleExpand(t);
            }
            
            if (play_addr >= mem_addr) {
              play_addr = 0;
            }
            
            
            CODEC.sampleWrite(data);

           if (Serial.available()) {
                t = Serial.read();

                if (t == ' ') {
                  state = STATE_UART;
                }

                if (t == '+') {
                  if (volume < 32) {
                    ++volume;  
                  }
                  
                  CODEC.outputVolume(volume);
                  
                }
                
                if (t == '-') {
                  if (volume > 0) {
                    --volume;  
                  }

                  CODEC.outputVolume(volume);
                  
                }
                
                t = 0;
            }
            
            break;

        case STATE_RECORD:
            
            data = CODEC.sampleRead();

            if (!compressedFlag) {
                low = data & 0xFF;
                high = (data >> 8) & 0xFF; 
                SRAM.write(record_addr++, low);
                SRAM.write(record_addr++, high);
            } else {
                t = CODEC.sampleCompress(data);
                SRAM.write(record_addr++, t);
            }

            if (!compressedFlag) {            
              if ((record_addr & 0x3FFF) == 0) {
                  Serial.write ("\n"); 
              }
            } else {
              if ((record_addr & 0x1FFF) == 0) {
                  Serial.write ("\n"); 
              } 
            }
            
            if (record_addr >= ((uint32_t)128*1024)) {
              record_addr = 131072 - 2;
            }

           
            if (Serial.available()) {
                t = Serial.read();

                if (t == ' ') {
                  mem_addr = record_addr;
                  state = STATE_UART;
                }

                t = 0;
            }
            
            break;
           
        default:
            state = STATE_UART;
            break;
    } // End of switch

} // End of FSM()





//============================================================================
// setup()
//============================================================================

void setup() {
    delay(1000);
    Serial.begin (115200);
    CODEC.begin();

    while (Serial.available()) {
      Serial.read();
    }
    
    SRAM.begin();  
    CODEC.outputVolume(volume);
 
}


//============================================================================
// loop()
//============================================================================

void loop() {
    FSM();
}

