//===========================================================
// ALPHACOMPRESS.CPP
// compress alpha data
// Copyright Zynga, Inc. 2011
//===========================================================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <memory.h>

#include "alphacompress.h"

#define ALIGN8(x) ((x + 7) & (~7))

void WriteBit(unsigned char *bitstream, unsigned long offset)
{
   unsigned char *pbitstream = &bitstream[offset >> 3];
   unsigned char bit = 1 << (offset - ((offset >> 3) << 3));
   *pbitstream |= bit;
}

void WriteBits(unsigned char *bitstream, unsigned long offset, unsigned char val, unsigned char width)
{
   for(int x=0;x<width;x++)
   {
      if((val >> x)&1)
      {
         WriteBit(bitstream, offset+x);
      }
   }
}

void WriteNibble(unsigned char *bitstream, unsigned long offset, unsigned char val)
{
   unsigned char *pbitstream = &bitstream[offset >> 3];
   unsigned char bit = (offset - ((offset >> 3) << 3));
   *pbitstream |= (val << bit);
}

unsigned char GetPaletteIndex(unsigned char val, const unsigned char *palette, unsigned int size)
{
   for(unsigned int x=0;x<size;x++)
   {
      if(val == palette[x])
      {
         return x;
      }
   }

   return 255;
}

unsigned int CompressAlpha(const unsigned char *alpha, const unsigned char *palette, unsigned int size, unsigned char **compressed)
{
   
   int stride = 4;
   int length = size / stride;
   
   int numbits = length + ((size - (length * stride)) != 0);
   
   
   int blackbits = 0;
   int whitebits = 0;
   int dataelements = 0;
   
   
   // test first
   for(int x=0;x<length;x++)
   {
      int white = alpha[x*stride] == 255;
      int black = alpha[x*stride] == 0;
      bool solid = true;
   
      if(white || black)
      {
         for(int y=0;y<(stride-1);y++)
         {
            if(alpha[x*stride+y] != alpha[x*stride+y+1])
            {
               solid = false;
               break;
            }
         }
      }
      else
      {
         solid = false;
      }
      
      
      if(solid)
      {
         blackbits += black;
         whitebits += white;
      }
      else
      {
         dataelements++;
      }
   }
   
   
   int blacktablebits = (blackbits + whitebits);
   int encodeddatabits = ((numbits - (blackbits + whitebits)) * (4 * stride + 1));
   int deadwhitebits = whitebits;
   int palettebits = 8 * 16; // 16 8-bit entries
   
   int estimatedsize = (blacktablebits + encodeddatabits + deadwhitebits + palettebits) / 8;
   
   unsigned char *bitstream = (unsigned char *)malloc(estimatedsize);
   unsigned char *indexdata = (unsigned char *)malloc(estimatedsize);
   unsigned char *palettedata = (unsigned char *)malloc(16);
   
   memset(bitstream, 0, estimatedsize);
   memset(indexdata, 0, estimatedsize);
   memcpy(palettedata, palette, 16);
   
   unsigned long bitoffset = 0;
   unsigned long indexdatabitoffset = 0;
   
   for(int x=0;x<length;x++)
   {
      int white = alpha[x*stride] == 255;
      int black = alpha[x*stride] == 0;
      bool solid = true;
   
      if(white || black)
      {
         for(int y=0;y<(stride-1);y++)
         {
            if((alpha[x*stride+y] != alpha[x*stride+y+1]))
            {
               solid = false;
               break;
            }
         }
      }
      else
      {
         solid = false;
      }
      
      if(solid)
      {
         if(white)
         {
            WriteBit(bitstream, bitoffset);
            bitoffset += 2; //
            // 1 (check next bit)
            // 0 (no data, so white)
         }
         else
         {
            bitoffset ++;
            // 0 (no next bit check, black row)
         }

      }
      else
      {
         WriteBit(bitstream, bitoffset);
         bitoffset++;
         WriteBit(bitstream, bitoffset);
         bitoffset++;
         
         // 1 (check next bit)
         // 1 (has data)
         
         // then go through each alpha value and match it to a palette entry
         for(int y=0;y<stride;y++)
         {
            unsigned char alphaval = alpha[x*stride+y];
            WriteNibble(indexdata, indexdatabitoffset, GetPaletteIndex(alphaval, palette, 16));
            indexdatabitoffset += 4;
         }
      }
      
   }
   
   unsigned char remainingpixels = size - (length * stride);
   
   if(remainingpixels) // just write the encoded data regardless of solids
   {
      for(int x=0;x<remainingpixels;x++)
      {
         unsigned char alphaval = alpha[length * stride + x];
         WriteNibble(indexdata, indexdatabitoffset, GetPaletteIndex(alphaval, palette, 16));
         indexdatabitoffset += 4;
      }
   }
   
   unsigned int numbitsaligned = ALIGN8(bitoffset);
   unsigned int bitstreamnumbytes = numbitsaligned >> 3;
   
   unsigned int numindexdatabitsaligned = ALIGN8(indexdatabitoffset);
   unsigned int indexdatanumbytes = numindexdatabitsaligned >> 3;
   
   *compressed = (unsigned char *)malloc(indexdatanumbytes + bitstreamnumbytes + 16 + 12);
   unsigned char *pcompressed = *compressed;
   
   unsigned int palettebytes = 16;
   
   memcpy(pcompressed, &bitstreamnumbytes, 4);
   memcpy(&pcompressed[4], &indexdatanumbytes, 4);
   memcpy(&pcompressed[8], &palettebytes, 4);
   memcpy(&pcompressed[12], bitstream, bitstreamnumbytes);
   memcpy(&pcompressed[12 + bitstreamnumbytes], indexdata, indexdatanumbytes);
   memcpy(&pcompressed[12 + bitstreamnumbytes + indexdatanumbytes], palette, 16);
   
   free(bitstream);
   free(indexdata);
   free(palettedata);
   
   
   // decompression test....
   #if 0
   FILE *decompressed_raw = fopen("test.raw", "wb");
   
   unsigned char decompvalue = 0;
   unsigned int decompencodeindex = 0;
   
   
   
   for(unsigned int x=0;x<bitoffset;x++)
   {
      //printf("x = %d ", x);
   
      unsigned char bitbyte = pcompressed[12 + (x >> 3)];
      unsigned char bittest = x - ((x >> 3) << 3);
      
      if(!((bitbyte >> bittest)&1))
      {
         // full run of black
         for(int y=0;y<4;y++)
         {
            decompvalue = 0;
            fwrite(&decompvalue, 1, 1, decompressed_raw);
         }
      }
      else
      {
         x++;
         
         bitbyte = pcompressed[12 + (x >> 3)];
         bittest = x - ((x >> 3) << 3);
         
         //printf("bitbyte = 0x%02x, bittest = %d\n", bitbyte, bittest);
         
         if((bitbyte >> bittest)&1)
         {
            for(int y=0;y<4;y++)
            {
               unsigned char encodebyte = pcompressed[12 + bitstreamnumbytes + (decompencodeindex >> 3)];
               bittest = decompencodeindex - ((decompencodeindex >> 3) << 3);

               unsigned int paletteindex = (encodebyte >> bittest) & 0xf;

               decompvalue = palette[paletteindex];
               
               
               printf("encodebyte: 0x%02x , bittest = %d, paletteIndex = %d, alpha = %d\n", encodebyte, bittest, paletteindex, decompvalue);
               
               fwrite(&decompvalue, 1, 1, decompressed_raw);
               decompencodeindex += 4;
            }
         }
         else
         {
            for(int y=0;y<4;y++)
            {
               decompvalue = 255;
               fwrite(&decompvalue, 1, 1, decompressed_raw);
            }
         
         }
         
         
      }
      
   }
   
   
   fclose(decompressed_raw);   
   #endif
   
   return indexdatanumbytes + bitstreamnumbytes + 16 + 12;

}




