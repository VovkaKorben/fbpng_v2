#include <cstdio>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "lodepng.h"
#include<iostream>
#include<fstream>

int main(int argc, char * argv[])
{
      if (argc == 1)
      {
            printf("sprecify png file name to output\n");
            return 1;
      }

      fb_fix_screeninfo finfo;
      fb_var_screeninfo vinfo;
      int fbdev = open("/dev/fb1", O_RDWR);
      if (!fbdev) {
            printf("error while opening /dev/fb0\n");
            return 1;
      }

      if (ioctl(fbdev, FBIOGET_VSCREENINFO, &vinfo)) {
            printf("error while FBIOGET_VSCREENINFO\n");
            return 1;
      }
      if (ioctl(fbdev, FBIOGET_FSCREENINFO, &finfo)) {
            printf("error while FBIOGET_FSCREENINFO\n");
            return 1;
      }
      unsigned char * fb_mem = (unsigned char *)mmap(NULL, finfo.smem_len, PROT_WRITE | PROT_READ, MAP_SHARED, fbdev, 0);// map framebuffer to user memory 
      if ((intptr_t)fb_mem == -1) {
            printf("mmap failed\n");
            return 1;
      }
      int bpp = vinfo.bits_per_pixel / 8, x_cnt, y_cnt = vinfo.yres;

      unsigned char * rgba = new unsigned char[vinfo.xres * vinfo.yres * 4],
            * src = fb_mem + (vinfo.yoffset * vinfo.xres + vinfo.yoffset) * bpp;
      unsigned int * dst = (unsigned int *)rgba;
      unsigned int dest_color;
      // convert data from FB to memory
      while (y_cnt--)
      {
            x_cnt = vinfo.xres;
            while (x_cnt--)
            {

                  if (vinfo.bits_per_pixel == 16)
                  {
                        unsigned short src_color = *src;
                        src++;
                        src_color = src_color | (unsigned short)(*src << 8);
                        src++;
                        // rgb
                        //dest_color = ((src_color & 0xF800) << 8) |   ((src_color & 0x07E0) << 5) |    ((src_color & 0x001F) << 3);

                        // bgr
                        dest_color =
                              ((src_color & 0xF800) >> 8) |
                              ((src_color & 0x07E0) << 5) |
                              ((src_color & 0x001F) << 19) |
                              0xFF000000;
                  }
                  else   if (vinfo.bits_per_pixel == 32)
                  {
                        dest_color = *src << 16;
                        src++;
                        dest_color |= *src << 8;
                        src++;
                        dest_color |= *src;
                        src += 2;
                  }
                  else
                  {
                        printf("fb device with %d bit per pixel not supported \n", vinfo.bits_per_pixel);
                        return 1;
                  }
                  *dst = dest_color;
                  dst++;
            }
            src += (vinfo.xres_virtual - vinfo.xres) * bpp;

      }

      unsigned char * png_data;
      size_t png_size;
      unsigned error = lodepng_encode32(&png_data, &png_size, rgba, vinfo.xres, vinfo.yres);
      if (error) {
            printf("lodepng_encode32 error: %s\n", lodepng_error_text(error));
            return 1;
      }


      //std::ofstream outf("/home/pi/projects/fbpng/test.png", std::ios::out | std::ios::binary);
      std::ofstream outf(argv[1], std::ios::out | std::ios::binary);
      if (!outf) {
            printf("Can't open file for writing: %s\n", argv[1]);
            return 1;
      }
      outf.write((char *)png_data, png_size);
      outf.close();
      free(png_data);
      //unsigned error = lodepng_encode32( decode_m(&ttt, &w, &h, (puint8)inp_data, inp_isize);


      close(fbdev);
      printf("PNG file created: %s\n", argv[1]);
      return 0;
}