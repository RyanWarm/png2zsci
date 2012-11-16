//===========================================================
// PNG2ZSCI.CPP
// png to zsci format converter
// Copyright Zynga, Inc. 2011
//===========================================================
#include "png.h"
#include "jpeglib.h"
#include "zlib.h"
#include "stdlib.h"
#include "memory.h"
#include "alphacompress.h"

#define PATH_LENGTH_MAX 1024

int g_ColorCompressionQuality = 80;
int g_AlphaCompressionQuality = 50;
bool g_OutputRecombinedImage = false;
bool g_VerboseMode = false;

char g_InputFile[PATH_LENGTH_MAX];
char g_InputFileNoExt[PATH_LENGTH_MAX];


void PrintDefaultMessage()
{
   printf("png2zsci (c) Copyright 2011 Zynga Inc.\n");
   printf("\tauthor: Eric Christensen echristensen@zynga.com\n");
   printf("usage [options] [png]\n");
   printf("\toptions:\n");
   printf("\t -c color quality percentage [0 - 100]\n");
   printf("\t -o output recombined image (*.recombined.png)\n");
   printf("\t -v verbose mode\n");
   printf("\t output is (*.zsci)\n");
}

bool ParseArgs(int argc, char *argv[])
{
   bool good_params = true;
   
   int x = 1;
   
   for(;x<argc;x++)
   {
      if(strstr(argv[x], "-"))
      {
         if(!strcmp("-c", argv[x]))
         {
            g_ColorCompressionQuality = atoi(argv[++x]);
            printf("Setting Color Compression Quality to %d%%\n", g_ColorCompressionQuality);
         }
         
         else if(!strcmp("-o",argv[x]))
         {
            g_OutputRecombinedImage = true;
            printf("Outputting recombined image for testing\n");
         }
         
         else if(!strcmp("-v", argv[x]))
         {
            g_VerboseMode = true;
            printf("Verbose Mode Enabled\n");
         }
         
      }
      else break;
   }
   
   if(x >= argc)
   {
      good_params = false;
   }
   else
   {
      strcpy(g_InputFile, argv[x]);
      strcpy(g_InputFileNoExt, argv[x]);
      char *pifne = strstr(g_InputFileNoExt, ".");
      *pifne = '\0';
   }
  
  
   return good_params;
   
}


int ZLIBDeflate(FILE *infile, FILE *outfile, unsigned int chunksize)
{
   int flush;
   int ret;
   unsigned have;
   z_stream strm;
   
   unsigned char *in = (unsigned char *)malloc(chunksize);
   unsigned char *out = (unsigned char *)malloc(chunksize);
      
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   ret = deflateInit(&strm, 9);
   
   if(ret != Z_OK)
   {
      free(in);
      free(out);
      return ret;
   }

   do 
   {
      strm.avail_in = fread(in, 1, chunksize, infile);
      
      if(ferror(infile))
      {
         deflateEnd(&strm);
         free(in);
         free(out);
         return Z_ERRNO;
      }
      
      flush = feof(infile) ? Z_FINISH : Z_NO_FLUSH;
      strm.next_in = in;
            
      do 
      {
         strm.avail_out = chunksize;
         strm.next_out = out;
         ret = deflate(&strm, flush);
         have = chunksize - strm.avail_out;
         if(fwrite(out, 1, have, outfile) != have || ferror(outfile))
         {
            deflateEnd(&strm);
            free(in);
            free(out);
            return Z_ERRNO;
         }
      } while(strm.avail_out == 0);
            
   } while(flush != Z_FINISH);
   
   deflateEnd(&strm);
   
   free(in);
   free(out);
   
   return Z_OK;
         
         

}

int ZLIBInflate(FILE *infile, FILE *outfile, unsigned int chunksize)
{
   int ret;
   unsigned have;
   z_stream strm;
   
   unsigned char *in = (unsigned char *)malloc(chunksize);
   unsigned char *out = (unsigned char *)malloc(chunksize);
   
   strm.zalloc = Z_NULL;
   strm.zfree = Z_NULL;
   strm.opaque = Z_NULL;
   strm.avail_in = 0;
   strm.next_in = Z_NULL;
   ret = inflateInit(&strm);
   
   if(ret != Z_OK)
   {
      free(in);
      free(out);
      return ret;
   }  
   
   do
   {
      strm.avail_in = fread(in, 1, chunksize, infile);
      
      if(ferror(infile))
      {
         inflateEnd(&strm);
         free(in);
         free(out);
         return Z_ERRNO;
      }
      
      if(strm.avail_in == 0)
      {
         break;
      }
      
      strm.next_in = in;
      
      do
      {
         strm.avail_out = chunksize;
         strm.next_out = out;
         
         ret = inflate(&strm, Z_NO_FLUSH);
         switch(ret)
         {
            case Z_NEED_DICT:
               ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
               inflateEnd(&strm);
               free(in);
               free(out);
               return ret;
         }
         
         have = chunksize - strm.avail_out;
         if(fwrite(out, 1, have, outfile) != have || ferror(outfile))
         {
            inflateEnd(&strm);
            free(in);
            free(out);
            return Z_ERRNO;
         }
      } while(strm.avail_out == 0);
   } while(ret != Z_STREAM_END);
   
   inflateEnd(&strm);
   free(in);
   free(out);
   
   return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

void WriteZSCI(const char *colorjpgfname, const char *alphajpgfname, unsigned int width, unsigned int height)
{
   FILE *colorfile = fopen(colorjpgfname, "rb");
   fseek(colorfile, 0, SEEK_END);
   unsigned int colorjpgsize = ftell(colorfile);
   fclose(colorfile);
   
   FILE *alphafile = fopen(alphajpgfname, "rb");
   fseek(alphafile, 0, SEEK_END);
   unsigned int alphajpgsize = ftell(alphafile);
   fclose(alphafile);
   
   char *colorbuf = (char *)malloc(colorjpgsize);
   char *alphabuf = (char *)malloc(alphajpgsize);
   
   colorfile = fopen(colorjpgfname, "rb");
   fread(colorbuf, colorjpgsize, 1, colorfile);
   fclose(colorfile);
   
   alphafile = fopen(alphajpgfname, "rb");
   fread(alphabuf, alphajpgsize, 1, alphafile);
   fclose(alphafile);
   
   // open the zsci file
   char zscifilename[PATH_LENGTH_MAX];
   sprintf(zscifilename, "%s.zsci", g_InputFileNoExt);
   
   FILE *zsci = fopen(zscifilename, "wb");
   
   char zsciheader[] = "ZSCI";
   
   fwrite(zsciheader, 4, 1, zsci);
   fwrite(&width, 4, 1, zsci);
   fwrite(&height, 4, 1, zsci);
   fwrite(&colorjpgsize, sizeof(int), 1, zsci);
   fwrite(&alphajpgsize, sizeof(int), 1, zsci);
   fwrite(colorbuf, colorjpgsize, 1, zsci);
   fwrite(alphabuf, alphajpgsize, 1, zsci);
	
   fclose(zsci);
   
   free(colorbuf);
   free(alphabuf);
   
   unlink(colorjpgfname);
   unlink(alphajpgfname);
}


int main(int argc, char *argv[])
{

   if(!ParseArgs(argc, argv))
   {
      PrintDefaultMessage();
      return 0;
   }

   png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
   if (!png_ptr)
      return (0);

   png_infop info_ptr = png_create_info_struct(png_ptr);
   if (!info_ptr)
   {
      png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
      return (0);
   }
   png_infop end_info = png_create_info_struct(png_ptr);
   if (!end_info)
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
      return (0);
   }
   
   FILE *pngfp = fopen(g_InputFile, "rb");
   
   if(!pngfp)
   {
      printf("Could not open %s\n", g_InputFile);
      return 0;
   }
   
   fread(png_ptr, 1, 8, pngfp);
   
   bool is_png = !png_sig_cmp((png_byte *)png_ptr, 0, 8);
   
   if(!is_png)
   {
      printf("%s is not a png\n", g_InputFile);
      return 0;
   }
   
   png_set_sig_bytes(png_ptr, 8);
   
   png_init_io(png_ptr, pngfp);
   
   png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
   
   
   fclose(pngfp);
   
   int width = png_get_image_width(png_ptr, info_ptr);
   int height = png_get_image_height(png_ptr, info_ptr);
   int bitdepth = png_get_bit_depth(png_ptr, info_ptr);
   int colortype = png_get_color_type(png_ptr, info_ptr);

   if(g_VerboseMode)
   {
      printf("\nwidth: %d, height: %d bitdepth: %d colortype: %d\n", width, height, bitdepth, colortype);
   }
   
   int is_only_rgb = (colortype == 2);
   
   
   png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);

   
   unsigned char **color_rows = (unsigned char **)malloc(sizeof(unsigned char **) * height);
   unsigned char **alpha_rows = (unsigned char **)malloc(sizeof(unsigned char **) * height);
   
   unsigned char gmap[256];
   unsigned int gmapcount = 0;
   
   for(int y=0;y<height;y++)
   {
      int col_count = 0;
      int alpha_count = 0;
      color_rows[y] = (unsigned char *)malloc(sizeof(unsigned char) * width * 3);
      alpha_rows[y] = (unsigned char *)malloc(sizeof(unsigned char) * width);
      for(int x=0;x<(width * 4); x+=4)
      {
         color_rows[y][col_count++] = row_pointers[y][x + 3] ? row_pointers[y][x + 0] : 0x0;
         color_rows[y][col_count++] = row_pointers[y][x + 3] ? row_pointers[y][x + 1] : 0x0;
         color_rows[y][col_count++] = row_pointers[y][x + 3] ? row_pointers[y][x + 2] : 0x0;
         
         if(is_only_rgb)
         {
            alpha_rows[y][alpha_count++] = 255;
         }
         else
         {
            if(row_pointers[y][x + 3] != 255 && row_pointers[y][x+3] != 0)
            {
               alpha_rows[y][alpha_count++] = (row_pointers[y][x+3] / 17) * 17;
            }
            else
            {
               alpha_rows[y][alpha_count++] = row_pointers[y][x+3];
            }
         }
         
         
         bool found = false;
         
         for(unsigned int z=0;z<gmapcount;z++)
         {
            if(gmap[z] == alpha_rows[y][alpha_count-1])
            {
               found = true;
               break;
            }
         }
         
         if(!found)
         {
            gmap[gmapcount++] = alpha_rows[y][alpha_count-1];
         }
         
      }
   }
   
   if(g_VerboseMode)
   {
      printf("\n\nfound %d unique alpha colors\n", gmapcount);
   
      for(unsigned int y=0;y<gmapcount;y++)
      {
         printf("%d ", gmap[y]);
      }  
   
      printf("\n\n");
   
   }
   
   
   // write the alpha channel out as a special file
   
   // allocate a buffer for the alpha channel
   unsigned char *rawalpha = (unsigned char *)malloc(width * height);
   unsigned char *prawalpha = rawalpha;
   
   for(int y=0;y<height;y++)
   {
      int alpha_count = 0;
      for(int x=0;x<width;x++)
      {
         *prawalpha = alpha_rows[y][alpha_count++];
         prawalpha++;
      }
   }
   
   unsigned char *compressedalpha = NULL;
   
   char outputcompressedalphaname[PATH_LENGTH_MAX];
   sprintf(outputcompressedalphaname, "%s.ecac", g_InputFileNoExt);
   char zlibalphafilename[PATH_LENGTH_MAX];
   sprintf(zlibalphafilename, "%s.ecacz", g_InputFileNoExt);
   
   
   // now that we have the raw alpha channel, lets compress it
   unsigned int compressedsize = CompressAlpha(rawalpha, gmap, width * height, &compressedalpha);
   if(compressedsize)
   {
      // alpha compression succeeded
      
      FILE *compressedalphafile = fopen(outputcompressedalphaname, "wb");
      fwrite(compressedalpha, compressedsize, 1, compressedalphafile);
      fclose(compressedalphafile);
   
   
      free(compressedalpha);
      
      
      // now zlib compress the sucker
      compressedalphafile = fopen(outputcompressedalphaname, "rb");
      FILE *zlibalphafile = fopen(zlibalphafilename, "wb");
      
      
      ZLIBDeflate(compressedalphafile, zlibalphafile, 16384);
      
      fclose(compressedalphafile);
      fclose(zlibalphafile);
      
      unlink(outputcompressedalphaname); 
      
   }
   
   
   
   free(rawalpha);
   
   
   char outputcolorjpgfname[PATH_LENGTH_MAX];
   char outputalphajpgfname[PATH_LENGTH_MAX];
   
   sprintf(outputcolorjpgfname, "%s.color.jpg", g_InputFileNoExt);
   sprintf(outputalphajpgfname, "%s.alpha.jpg", g_InputFileNoExt);
   
   
   jpeg_compress_struct cinfo;
   jpeg_error_mgr jerr;

   // write the color data
   cinfo.err = jpeg_std_error(&jerr);
   jpeg_create_compress(&cinfo);
   FILE *outjpg = fopen(outputcolorjpgfname, "wb");
   jpeg_stdio_dest(&cinfo, outjpg);
   cinfo.image_width = width;
   cinfo.image_height = height;
   cinfo.input_components = 3;
   cinfo.in_color_space = JCS_RGB;
   jpeg_set_defaults(&cinfo);
   jpeg_set_quality(&cinfo, g_ColorCompressionQuality, true);
   jpeg_start_compress(&cinfo, true);
   JSAMPROW data_row[1];
   for(int y=0;y<height;y++)
   {
      data_row[0] = color_rows[y];
      jpeg_write_scanlines(&cinfo, data_row, 1);
   }
   jpeg_finish_compress(&cinfo);
   jpeg_destroy_compress(&cinfo);
   fclose(outjpg);
   
   // now read the color jpeg back in.
   jpeg_decompress_struct dcinfo;
   dcinfo.err = jpeg_std_error(&jerr);
   jpeg_create_decompress(&dcinfo);
   FILE *injpg = fopen(outputcolorjpgfname,"rb");
   jpeg_stdio_src(&dcinfo, injpg);
   jpeg_read_header(&dcinfo, true);
   jpeg_start_decompress(&dcinfo);
   
   if(g_VerboseMode)
   {
      printf("width: %d height: %d OCC: %d OC: %d\n", dcinfo.image_width, dcinfo.image_height, dcinfo.out_color_components, dcinfo.output_components);
   }
   
   for(int y=0;y<height;y++)
   {
      data_row[0] = color_rows[y];
      jpeg_read_scanlines(&dcinfo, data_row, 1);
   }
   
   jpeg_finish_decompress(&dcinfo);
   jpeg_destroy_decompress(&dcinfo);
   fclose(injpg);
   
   if(g_OutputRecombinedImage)
   {

      // recombine the channels
      for(int y=0;y<height;y++)
      {
         int col_count = 0;
         int alpha_count = 0;
         
         if(colortype == 6)
         {
            for(int x=0;x<(width * 4); x+=4)
            {
               row_pointers[y][x + 0] = color_rows[y][col_count++];
               row_pointers[y][x + 1] = color_rows[y][col_count++];
               row_pointers[y][x + 2] = color_rows[y][col_count++];
               row_pointers[y][x + 3] = alpha_rows[y][alpha_count++];
            }
         
         }
         else
         {
            for(int x=0;x<(width * 3); x+=4)
            {
               row_pointers[y][x + 0] = color_rows[y][col_count++];
               row_pointers[y][x + 1] = color_rows[y][col_count++];
               row_pointers[y][x + 2] = color_rows[y][col_count++];
            }
            
         }
         
      }
      
   
   
      png_structp png_wptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL);
      if (!png_ptr)
         return (0);
   
      png_infop info_wptr = png_create_info_struct(png_wptr);
      if (!info_wptr)
      {
         png_destroy_write_struct(&png_wptr, (png_infopp)NULL);
         return (0);
      }
      png_infop end_info = png_create_info_struct(png_wptr);
      if (!end_info)
      {
         png_destroy_write_struct(&png_wptr, &info_wptr);
         return (0);
      }   
   
      char recombinedfname[PATH_LENGTH_MAX];
      
      sprintf(recombinedfname, "%s.recombined.png", g_InputFileNoExt);
      
      pngfp = fopen(recombinedfname, "wb");
      png_init_io(png_wptr, pngfp);
      
      png_set_IHDR(png_wptr, info_wptr, width, height, 8, colortype, PNG_INTERLACE_NONE,
                  PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
      
      
      png_write_info(png_wptr, info_wptr);
      
      
      png_write_image(png_wptr, row_pointers);
      
      
      png_write_end(png_wptr, NULL);

      fclose(pngfp);

      if(g_VerboseMode)
      {
         printf("\nPlease look at %s.recombined.png for quality check\n", g_InputFileNoExt);
      }
   }
   


   // free the color rows and the alpha rows
   for(int y=0;y<height;y++)
   {
      free(color_rows[y]);
      free(alpha_rows[y]);
   }
   
   free(color_rows);
   free(alpha_rows);

   if(g_VerboseMode)
   {
      printf("\nWriting %s.zsci ... ", g_InputFileNoExt);
   }
  
   
   WriteZSCI(outputcolorjpgfname, zlibalphafilename, width, height);

   if(g_VerboseMode)
   {
      printf("Done.\n");
   }

	return 1;
}
