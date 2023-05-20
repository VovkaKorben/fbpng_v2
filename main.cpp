#include <cstdio>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "lodepng.h"
#include<iostream>
#include<fstream>
#include <getopt.h>

struct cfg_s
{
      int dev{ 0 };
      int silent{ 0 };
      char * filename{ NULL };
      //char * devname{ NULL };

};
void do_help(const char * programname)
{
      printf("Usage: %s [options] filename, where options are:\n", programname);
      printf("\t[-f X] use specific device /dev/fbX (default /dev/fb0)\n");


}
int main(int argc, char * argv[])
{
      char devname[16], filename[1024];
      cfg_s cfg;
      int opt;
      while ((opt = getopt(argc, argv, "hf:")) != -1)
      {
            switch (opt)
            {
                  case '?':
                  case 'h':
                  default:
                        do_help(argv[0]);
                        return -1;
                  case 'f':
                  case 'F':
                        cfg.dev = atoi(optarg);
                        break;
                  case 's':
                  case 'S':
                        cfg.silent = 1;
                        break;

            }
      }

      if (optind < argc)
      {
            //printf("Non-option args: ");
            while (optind < argc)
            {
                  strcpy(filename, argv[optind]);
                  //filename = argv[optind++];
                  optind++;
            }
            //printf("%s ", argv[optind++]);
      //printf("\n");
      }


      sprintf(devname, "/dev/fb%d", cfg.dev);

      /*if (argc == 1)
{
      printf("sprecify png file name to output\n");
      return 1;
}
*/
      fb_fix_screeninfo finfo;
      fb_var_screeninfo vinfo;
      int fbdev = open(devname, O_RDWR);
      if (!fbdev) {
            printf("error while opening %s\n", devname);
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
      if (!cfg.silent)
            printf("%s: %dx%d (%d bpp)\n", devname, vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
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


      std::ofstream outf(filename, std::ios::out | std::ios::binary);
      if (!outf) {
            printf("Can't open file for writing: %s\n", filename);
            return 1;
      }
      outf.write((char *)png_data, png_size);
      outf.close();
      free(png_data);


      close(fbdev);
      if (!cfg.silent)
            printf("PNG file created: %s\n", filename);
      return 0;
}