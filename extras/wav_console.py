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



###############################################################################
# Remarks:
#   Python script to work with the sketch
#   To use this script, please run it under Windows Command Prompt. The
#   Windows Power Shell does not work very well with this script. 
#   In particular, the display format might get messed up under Power Shell
#
# Usage:
#   Python wav_console.py com_port_name
#   For example, to use COM6, type in
#     Python wav_console.py COM6
#  
#   After script is loaded. Type in help for available commands.
###############################################################################

import os
import sys

import serial

from time import sleep
from Console_Input import Console_Input
from CRC16_CCITT import CRC16_CCITT

class _wave_file:
    
    
    _BIAS = 0x84  # Bias for linear code. 
    _CLIP = 8159
    
    
    def linear2Alaw (self, pcm_val):
        pcm_val = pcm_val // 8
        
        if (pcm_val >= 0):
            mask = 0xD5
        else:
            mask = 0x55
            pcm_val = -pcm_val - 1
        
        #  Convert the scaled magnitude to segment number. 
        
        seg_aend = [0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF]

        for i in range(len(seg_aend)):  
            if (pcm_val < seg_aend[i]):
                break
        
        seg = i
        
        # Combine the sign, segment, and quantization bits. 
        if (pcm_val > seg_aend[7]): # out of range, return maximum value. 
            ret = 0x7F ^ mask
        else:
            aval = seg << 4
            if (seg < 2):
                aval = aval | ((pcm_val >> 1) & 0xF);
            else:
                aval = aval | ((pcm_val >> seg) & 0xF)
            
            ret = aval ^ mask
            
        return ret
    
    def Alaw2linear(self, a_val):
        a_val = a_val ^ 0x55
        
        t = (a_val & 0xF) << 4
        seg = ((a_val & 0x70) >> 4) & 0xF
        
        if (seg == 0):
            t = t + 8
        elif (seg == 1):
            t = t + 0x108
        else:
            t = t + 0x108;
            t = t << (seg - 1)
        
        if (a_val & 0x80):
            ret = t
        else:
            ret = -t
        return ret
    
    
    def linear2Mulaw (self, pcm_val):
        # Get the sign and the magnitude of the value. 
        pcm_val = pcm_val // 4
        if (pcm_val < 0):
            pcm_val = -pcm_val
            mask = 0x7F
        else:
            mask = 0xFF
        
        if ( pcm_val > _wave_file._CLIP ):
            pcm_val = _wave_file._CLIP;     # clip the magnitude
        
        pcm_val = pcm_val + (_wave_file._BIAS // 4)

        seg_uend = [0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF]

        # Convert the scaled magnitude to segment number.
        for i in range(len(seg_uend)):  
            if (pcm_val < seg_uend[i]):
                break
        
        seg = i
        
        # Combine the sign, segment, quantization bits;
        # and complement the code word.

        if (pcm_val > seg_uend[7]): # out of range, return maximum value. 
            ret = (0x7F ^ mask)
            return ret
        else:
            uval = (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF)
            return (uval ^ mask)

    
    def Mulaw2linear(self, u_val):
    
   #     print ("u_val = ", u_val)
        # Complement to obtain normal u-law value
        u_val = ~u_val

        # Extract and bias the quantization bits. Then
        # shift up by the segment number and subtract out the bias.
        
        t = ((u_val & 0xF) << 3) + _wave_file._BIAS
        t = t << ((u_val & 0x70) >> 4)

        if (u_val & 0x80):
            ret = _wave_file._BIAS - t
        else:
            ret = t - _wave_file._BIAS
   #     print ("ret = ", ret)    
        return (ret)

    
    def _bin_to_number (self, str_in):
        accum = 0
        for i in range (0, len(str_in)):
            t = str_in[i]
            accum = accum + (t << (i * 8))
        
        return accum
   
    def _data_extract(self):

        try:
            bytes = open(self.file_name, "rb").read()
        except Exception:
            print (self.file_name, "file open fail!")
            return []
            
        chunk_ID = bytes [0:4].upper()
        chunk_size = self._bin_to_number (bytes[4:8])
        format = bytes [8:12].upper()
        subchunk1_ID = bytes [12:16].lower()  
        subchunk1_size = self._bin_to_number (bytes[16:20])
        audio_format =  self._bin_to_number(bytes[20:22])
        num_of_channels = self._bin_to_number(bytes[22:24])
        sample_rate = self._bin_to_number(bytes[24:28])
        byte_rate = self._bin_to_number(bytes[28:32])
        block_align = self._bin_to_number(bytes[32:34])
        bits_per_sample = self._bin_to_number(bytes[34:36])
   
        if (chunk_ID != b'RIFF') :
            print ("NOT RIFF format!\n", chunk_ID)
            os._exit(1)

        if (format != b'WAVE') :
            print ("NOT WAVE format!\n")
            os._exit(1)


        if (subchunk1_ID != b"fmt ") :
            print ("unknown subchunk1 ID : " + subchunk1_ID)
            os._exit(1)
        else:
            print ("subchunk_ID ", subchunk1_ID)     
        
        
        print ("\nAudio Format is {0:d}, (1 = PCM)".format(audio_format))
        print ("\nnumer of channels = {0:d}".format(num_of_channels))

        if (num_of_channels > 1) :
            print ("More than one channel!")
            os._exit(1)
     
        print ("sample_rate = ", sample_rate)
        print ("bits_per_sample = ", bits_per_sample)
        print ("block_align = ", block_align)
        
        subchunk_start = 20 + subchunk1_size
        subchunk_ID = bytes [subchunk_start: subchunk_start + 4].lower()  
        subchunk_size = self._bin_to_number (bytes[subchunk_start + 4:subchunk_start + 8])
        
        print ("subchunk_ID ", subchunk_ID)
        
        while(subchunk_ID != b'data'):
            subchunk_start = subchunk_start + 8 + subchunk_size
            subchunk_ID = bytes [subchunk_start: subchunk_start + 4].lower()  
            subchunk_size = self._bin_to_number (bytes[subchunk_start + 4:subchunk_start + 8])
                    
            print ("subchunk_ID ", subchunk_ID)
        
           
        num_of_samples = int(subchunk_size / block_align)

        print ("num_of_samples = ", num_of_samples)
        print ("time span = ", num_of_samples / sample_rate, " seconds")
        
        print ("subchunk_start = ", subchunk_start)
        
        sample_list = []
        for i in range (0, num_of_samples) :
            data = self._bin_to_number(bytes[subchunk_start + 8 + i * block_align : subchunk_start + 8 + i * block_align + block_align])
            if (bits_per_sample == 8):
                None
                #print("i = {0:d}\t data = {1:d}\n".format(i, data))
            elif (bits_per_sample == 16):
                if (data > 32767) :
                    data = data - 65536
                #print("i = {0:d}\t data = {1:d}\n".format(i, data))
                
            sample_list.append(data)

        return (sample_list) 
        
    def sample_save_16bit(self, data_byte_list):
        file_data_list = [ord('R'), ord('I'), ord('F'), ord('F')]
        file_data_list = file_data_list + [0x24, 0x00, 0x2, 0]
        file_data_list = file_data_list + [ord('W'), ord('A'), ord('V'), ord('E')]
        file_data_list = file_data_list + [ord('f'), ord('m'), ord('t'), ord(' ')]
        file_data_list = file_data_list + [16, 0, 0, 0]  #Subchunk1Size, 16 for PCM
        file_data_list = file_data_list + [1, 0] # AudioFormat, PCM = 1
        file_data_list = file_data_list + [1, 0] # NumChannels      
        file_data_list = file_data_list + [0x40, 0x1F, 0, 0] # Sample Rate
        file_data_list = file_data_list + [0x80, 0x3E, 0, 0] # Byte Rate
        file_data_list = file_data_list + [2, 0] # block align
        file_data_list = file_data_list + [16, 0] # bit per sample
        
        
        # Subchunk2
        
        
        file_data_list = file_data_list + [ord('d'), ord('a'), ord('t'), ord('a')]
        file_data_list = file_data_list + [0, 0, 4, 0] # Subchunk2Size    
        
        file_data_list = file_data_list + data_byte_list
        
        if (len(data_byte_list) < (256 * 1024)):
            file_data_list = file_data_list + [0] * (256*1024 - len(data_byte_list))
        
        with open(self.file_name, "wb") as f:
            bytes_data = bytearray(file_data_list)
            f.write(bytes_data)
        
        f.close()
        

        
    def __init__ (self, file_name=""):
        self.file_name = file_name

        
        
class Wav_Console:
    
#############################################################################
# command procedures
#############################################################################
    
    _CMD_SYNC = [0xBA, 0xAB, 0x11]
    _CMD_TYPE_MEM_WRITE_WORD = 0x37
    _CMD_TYPE_MEM_WRITE_BYTE = 0x38
    _CMD_TYPE_MEM_WRITE_EXT  = 0x39
    _CMD_TYPE_MEM_READ_WORD  = 0x40
    _CMD_TYPE_MEM_READ_EXT   = 0x41
    
    _CMD_TYPE_CMD_PLAY       = 0x50
    _CMD_TYPE_CMD_RECORD     = 0x52
    _CMD_TYPE_CMD_SET_VOLUME = 0x54
    
    _FRAME_REPLY_LEN = 12
    

    #========================================================================
    # _string_to_data
    #========================================================================
    def _string_to_data (self, data_string):
                
        if (data_string.startswith('0x')):
            data = int(data_string[2:], 16)
        else:
            for i in data_string:
                if ((ord(i) < ord('0')) or (ord(i) > ord('9'))):
                    return -1
            data = int(data_string)
        
        return data

    
#############################################################################
# static variables
#############################################################################
    
    _Console_PROMPT = "\n>> "

    #========================================================================
    #  _verify_crc
    #------------------------------------------------------------------------
    #  Remarks: calculate and check CRC16_CCITT for frames 
    #========================================================================
    def _verify_crc (self, data):
        data_list = [i for i in data]
        crc_data = self._crc16_ccitt.get_crc (data_list [0 : Wav_Console._FRAME_REPLY_LEN - 2])
     
        if (crc_data == data_list [Wav_Console._FRAME_REPLY_LEN - 2 : Wav_Console._FRAME_REPLY_LEN]):
            return True
        else:
            return False
    
    #========================================================================
    # _frame_write_ext
    #========================================================================
    def _frame_write_ext (self, addr, data_list, show_crc_error = 0):
        
        addr_write_byte_0  = addr & 0xFF
        addr_write_byte_1  = (addr >> 8) & 0xFF
        addr_write_byte_2  = (addr >> 16) & 0xFF
        addr_write_byte_3  = (addr >> 24) & 0xFF
        
        
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_MEM_WRITE_EXT;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [addr_write_byte_3, addr_write_byte_2, addr_write_byte_1, addr_write_byte_0] +  data_list
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            #print ([hex(i) for i in frame])
            
            self._serial.write (frame)
              
            ret = self._serial.read (Wav_Console._FRAME_REPLY_LEN)

            condition = not self._verify_crc (ret)
            if (condition):
                if (show_crc_error):
                    print ("addr=", addr, "_frame_write_ext CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                print (zero_frame)
                self._serial.write (zero_frame)                 
    
    #========================================================================
    # _frame_write_WORD
    #========================================================================
    def _frame_write_16bit (self, addr, data, show_crc_error = 0):
        
        addr_write_byte_0  = addr & 0xFF
        addr_write_byte_1  = (addr >> 8) & 0xFF
        addr_write_byte_2  = (addr >> 16) & 0xFF
        addr_write_byte_3  = (addr >> 24) & 0xFF
        
        data_low_byte  = data & 0xFF;
        data_high_byte = (data >> 8) & 0xFF;
        
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_MEM_WRITE_WORD;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [addr_write_byte_3, addr_write_byte_2, addr_write_byte_1, addr_write_byte_0] + [data_high_byte, data_low_byte] 
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            self._serial.write (frame)
              
            ret = self._serial.read (Wav_Console._FRAME_REPLY_LEN)

            condition = not self._verify_crc (ret)
            if (condition):
                if (show_crc_error):
                    print ("addr=", addr, "_frame_write_16bit CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                print (zero_frame)
                self._serial.write (zero_frame)                 
      
    
    #========================================================================
    # _frame_set_volume
    #========================================================================
    def _frame_set_volume (self, volume):
        
        data_low_byte  = volume & 0xFF;
        data_high_byte = (volume >> 8) & 0xFF;
        
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_MEM_WRITE_WORD;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [10, 11, 12, 13] + [data_high_byte, data_low_byte] 
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            self._serial.write (frame)
              
            ret = self._serial.read (Wav_Console._FRAME_REPLY_LEN)

            condition = not self._verify_crc (ret)
            if (condition):
                if (show_crc_error):
                    print ("addr=", addr, "_frame_set_volume CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                print (zero_frame)
                self._serial.write (zero_frame)                 
      
    
    
    
    #========================================================================
    # _frame_write_byte
    #========================================================================
    def _frame_write_byte (self, addr, data, show_crc_error = 0):
        
        addr_write_byte_0  = addr & 0xFF
        addr_write_byte_1  = (addr >> 8) & 0xFF
        addr_write_byte_2  = (addr >> 16) & 0xFF
        addr_write_byte_3  = (addr >> 24) & 0xFF
        
        data_low_byte  = data & 0xFF
        data_high_byte = data_low_byte
        
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_MEM_WRITE_BYTE;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [addr_write_byte_3, addr_write_byte_2, addr_write_byte_1, addr_write_byte_0] + [data_high_byte, data_low_byte] 
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            self._serial.write (frame)
              
            ret = self._serial.read (Wav_Console._FRAME_REPLY_LEN)

            condition = not self._verify_crc (ret)
            if (condition):
                if (show_crc_error):
                    print ("addr=", addr, "_frame_write_16bit CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                print (zero_frame)
                self._serial.write (zero_frame)                 
    
    
    #========================================================================
    # _frame_read_16bit
    #========================================================================
    def _frame_read_16bit (self, addr, show_crc_error = 0):
        
        addr_write_byte_0  = addr & 0xFF
        addr_write_byte_1  = (addr >> 8) & 0xFF
        addr_write_byte_2  = (addr >> 16) & 0xFF
        addr_write_byte_3  = (addr >> 24) & 0xFF
        
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_MEM_READ_WORD;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [addr_write_byte_3, addr_write_byte_2, addr_write_byte_1, addr_write_byte_0] + [0x12, 0x34] 
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            #print ([hex(i) for i in frame])
            
            for i in frame:
                self._serial.write ([i])
              
            ret = self._serial.read (Wav_Console._FRAME_REPLY_LEN)
            condition = not self._verify_crc (ret)
            if (condition):
                if (show_crc_error):
                    print ("addr=", addr, "_frame_write_16bit CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                print (zero_frame)
                self._serial.write (zero_frame)                 
    
        data = ret [len(Wav_Console._CMD_SYNC) + 1] * 256 + ret [len(Wav_Console._CMD_SYNC) + 2] 
        
        return data
        
    
    #========================================================================
    # _frame_read_ext
    #========================================================================
    def _frame_read_ext (self, addr, show_crc_error = 0):
        
        addr_write_byte_0  = addr & 0xFF
        addr_write_byte_1  = (addr >> 8) & 0xFF
        addr_write_byte_2  = (addr >> 16) & 0xFF
        addr_write_byte_3  = (addr >> 24) & 0xFF
        
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_MEM_READ_EXT;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [addr_write_byte_3, addr_write_byte_2, addr_write_byte_1, addr_write_byte_0] + [0x12, 0x34] 
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            #print ([hex(i) for i in frame])
            
            for i in frame:
                self._serial.write ([i])
              
            ret = self._serial.read (128 + 6)
            
            crc_data = bytearray(self._crc16_ccitt.get_crc (ret[0:128+4]))
            
            if (crc_data == ret[128+4 : 128+6]):
                condition = False
            else:
                condition = True

            if (condition):
                if (show_crc_error):
                    print ("addr=", addr, "_frame_write_16bit CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                self._serial.write (zero_frame)                 
    
        data = ret [len(Wav_Console._CMD_SYNC) + 1 : len(Wav_Console._CMD_SYNC) + 129] 
        
        return [i for i in data]
        
        
    #========================================================================
    # _do_help
    #========================================================================
    def _do_help (self):
         if (len(self._args) > 1):
            if (self._args[1] in Mustang_Console._CONSOLE_CMD):
                print ("Usage:\n      ", self._args[1], Wav_Console._CONSOLE_CMD[self._args[1]][1])
                print ("Description:\n      ", Wav_Console._CONSOLE_CMD[self._args[1]][2])
                
            else:
                print ("Unknow command")        
         else:
            print ("available commands:")
            for key, value in sorted(Wav_Console._CONSOLE_CMD.items()):
                print (" ", key)

    #========================================================================
    # _do_read
    #========================================================================
    def _do_read16(self):
        
        addr = self._string_to_data(self._args[1])
        ret = self._frame_read_16bit(addr)
        
        print (hex(ret & 0xFF), " ", hex((ret >> 8) & 0xFF))
    
    #========================================================================
    # _do_write8
    #========================================================================
    def _do_write8(self):
        
        addr = self._string_to_data(self._args[1])
        data = self._string_to_data(self._args[2])
        self._frame_write_byte(addr, data)
        
        print ("write ", hex(data),"to address", hex(addr))
        
    #========================================================================
    # _do_write16
    #========================================================================
    def _do_write16(self):
        
        addr = self._string_to_data(self._args[1])
        data = self._string_to_data(self._args[2])
        self._frame_write_16bit(addr, data)
        
        print ("write ", hex(data),"to address", hex(addr))
        
        
    #========================================================================
    # _do_load
    #========================================================================
    def _do_load(self):
        
        wave_file = _wave_file(self._args[1])
        sample_list = wave_file._data_extract()
        
        #print (sample_list)
        
        num_of_ext_frames = len(sample_list) // 128
        num_of_remaining_bytes = len(sample_list) % 128
        
        addr = 0
    
        for i in range(num_of_ext_frames):
            data_list = []
            for j in range (128):
                data_byte = wave_file.linear2Mulaw(sample_list[i * 128 + j])
                data_list = data_list + [data_byte]
            
            self._frame_write_ext (addr, data_list)
            addr = addr + 128
            
            print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b %d %% completed" % int(addr * 100/ (len(sample_list))), end="")
            sys.stdout.flush()
        
        for i in range (num_of_remaining_bytes):
            data_byte = wave_file.linear2Mulaw(sample_list[num_of_ext_frames * 128 + i])
            self._frame_write_byte (addr, data_byte)
            addr = addr + 1
        
        print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b                                                  ");
        sys.stdout.flush()
        
    #========================================================================
    # _do_save
    #========================================================================
    def _do_save(self):
    
        wave_file = _wave_file(self._args[1])
  
        data_byte_list = []
        for i in range(1024):
            data_ext = self._frame_read_ext(i * 128)
            data_list_tmp = [0] * 256
            
            for j in range(128):
                tmp = wave_file.Mulaw2linear (data_ext[j])
                if (tmp < 0):
                    tmp = (65536 + tmp) & 0xFFFF
                    
                data_list_tmp[j*2] = tmp & 0xFF
                data_list_tmp[j*2 + 1] = (tmp >> 8) & 0xFF
                
            data_byte_list = data_byte_list + data_list_tmp
    
            print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b %d %% completed" % int(i * 128 * 100/ (128 * 1024)), end="")
            sys.stdout.flush()
        
        print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b                                                  ");
        sys.stdout.flush()
            
        wave_file.sample_save_16bit (data_byte_list)
        
        
    #========================================================================
    # _do_play
    #========================================================================
    def _do_play(self):
  
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_CMD_PLAY;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [0xde, 0xad, 0xbe, 0xef] + [0x99, 0x88] 
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            self._serial.write (frame)
              
            ret = self._serial.read (Wav_Console._FRAME_REPLY_LEN)

            condition = not self._verify_crc (ret)
            if (condition):
                print ("addr=", addr, "_frame_write_16bit CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                print (zero_frame)
                self._serial.write (zero_frame)                 
      
        print ("playing... Press Enter to stop")
    
    #========================================================================
    # _do_record
    #========================================================================
    def _do_record(self):
  
        condition = True
        
        while (condition):
        
            frame_type_byte = Wav_Console._CMD_TYPE_CMD_RECORD;
                       
            frame = Wav_Console._CMD_SYNC + [frame_type_byte] + [0xde, 0xad, 0xbe, 0xef] + [0x99, 0x88] 
                       
            frame = frame + self._crc16_ccitt.get_crc (frame)
            
            self._serial.write (frame)
              
            ret = self._serial.read (Wav_Console._FRAME_REPLY_LEN)

            condition = not self._verify_crc (ret)
            if (condition):
                print ("record _frame_write_16bit CRC fail")
                
                zero_frame = [0] * Wav_Console._FRAME_REPLY_LEN
                print (zero_frame)
                self._serial.write (zero_frame)                 
      
        print ("start recording...")
        
        record_time = 0
        while(record_time < 16):
            print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b %d seconds" % record_time, end="")
            sys.stdout.flush()
            ret = self._serial.read(1)
            record_time = record_time + 1
           
        print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b==> recording finished")
        self._serial.write([ord(' '), ord(' '), ord(' '), ord('\n')])
         
         
    #========================================================================
    # _do_volume_up
    #========================================================================
    def _do_volume_up(self):
        self._serial.write([ord('+'), ord('\n')])
        sys.stdout.flush()    
        print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b==> volume up")
        
    #========================================================================
    # _do_volume_down
    #========================================================================
    def _do_volume_down(self):
        self._serial.write([ord('-'), ord('\n')])
        sys.stdout.flush()    
        print ("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b==> volume down")
        
    #========================================================================
    # _dummy_exit
    #------------------------------------------------------------------------
    # Remarks:
    #    This function never gets called. But need this as a place holder 
    # for tools that can convert python script into .exe files.
    #========================================================================
    def _dummy_exit (self):
        None
        
    _CONSOLE_CMD = {
        'help'                  : (_do_help,              "[command_to_look_up]", "list command info"), 
        'load_wav'              : (_do_load,              "wav_file_name", "load wave file"),
        'save_wav'              : (_do_save,              "wav_file_name", "save wave file"),
        #'read16'                : (_do_read16,            "address", "read memory"),
        #'write8'                : (_do_write8,            "address data", "write byte memory"),
        #'write16'               : (_do_write16,           "address data", "write byte memory"),          
        'volume_up'             : (_do_volume_up,         " ", "increase output volume"),
        'volume_down'           : (_do_volume_down,       " ", "decrease output volume"),
        
        'play'                  : (_do_play,             " ", "play wav file"),
        'record'                : (_do_record,           " ", "record wav file"),
        'exit'                  : (_dummy_exit,             " ", "exit console")
    }
    
 
   
       
#############################################################################
# Methods
#############################################################################
  
    #========================================================================
    # __init__
    #========================================================================
    def __init__ (self, com_port, baud_rate=115200):
        self._serial = serial.Serial(com_port, baud_rate, timeout=6)
        if (self._serial.in_waiting):
            r = self._serial.read (self._serial.in_waiting) # clear the uart receive buffer 
            
        self._stdin = Console_Input(">> ", Wav_Console._CONSOLE_CMD.keys())
        self._stdin.uart_raw_mode_enable = 0
        
        self.volume = 32
 
        self._crc16_ccitt = CRC16_CCITT()
         
    #========================================================================
    # _execute_cmd
    #========================================================================
    def _execute_cmd (self):
        #try:
            Wav_Console._CONSOLE_CMD[self._args[0]][0](self)
        #except Exception:
         #   print ("Exception when executing", self._args[0])
            
    #========================================================================
    # _line_handle
    #========================================================================
    def _line_handle (self, line):
        
        #try:
        self._args = line.split()
        
        if (len(self._args) == 0):
            print ("empty line!");
            self._serial.write([ord(' '), ord('\n')])
        elif (self._args[0] not in Wav_Console._CONSOLE_CMD):
            print ("unknown command ", self._args[0]);
        else:
            #print ("Execute command: ", line.strip());
            self._execute_cmd ()
        #except Exception:
         #   print ("eeeeeeeeeeeeeeeeeeee\n", end="");
        
        
    #########################################################################
    # This is the main loop    
    #########################################################################
    def run (self):
        while(1):
            try:
                line = self._stdin.input()
                if (line == "exit"):
                    print ("\nGoodbye!!!")
                    return
            except Exception:
                print ("type \"exit\" to end console session \n", end="");

            self._serial.reset_input_buffer()
            self._line_handle (line)
        
        
def main():

    baud_rate = 115200
    com_port = sys.argv[1]
    raw_uart_switch = 0

    try:
        wave = Wav_Console (com_port, baud_rate)
    except:
        print ("Failed to open COM port")
        sys.exit(1)

    wave.run()
    
    #print ("wav = ",   sys.argv[1])
    #wav_file  = wave_file(sys.argv[1])  
    #wav_file._data_extract()    
      
if __name__ == "__main__":
    main()        

        