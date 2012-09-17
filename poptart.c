/*
PopTart: toast notifications for RaspberryPi

See README.md for more information.

Copyleft 2012, Peter H. Li

Derived from the RaspberryPi hello_font example program; see Broadcom copyright
and license below.

-----

Copyright (c) 2012, Broadcom Europe Ltd
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

#include "bcm_host.h"
#include "vgfont.h"


int LAYER = 1;

void init_screen() {
   bcm_host_init();
   int s = gx_graphics_init(".");
   assert(s == 0);
}



/**
 * Not really clear why we make a window the whole screen size instead of just 
 * what's needed for the text.  We should be able to get the text extent 
 * without an existing resource if we include vgft.h
 */
GRAPHICS_RESOURCE_HANDLE make_transparent_canvas(uint32_t width, uint32_t height) {
   GRAPHICS_RESOURCE_HANDLE img;
   int s = gx_create_window(0, width, height, GRAPHICS_RESOURCE_RGBA32, &img);
   assert(s == 0);
   
   // Make it transparent
   graphics_resource_fill(img, 0, 0, width, height, GRAPHICS_RGBA32(0,0,0,0x00));

   // Show it
   graphics_display_resource(img, 0, LAYER, 0, 0, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, VC_DISPMAN_ROT0, 1);

   return img;
}



// Caller must free read_buffer
// Adapted from http://stackoverflow.com/questions/1151029/unix-linux-ipc-reading-from-a-pipe-how-to-know-length-of-data-at-runtime
char *fslurp(FILE *f) {
   const int bytes_at_a_time = 2;
   char *read_buffer = NULL;
   int buffer_size = 0;
   int buffer_offset = 0;
   int chars_io;
   while (1) {
      if (buffer_offset + bytes_at_a_time > buffer_size) {
        buffer_size = bytes_at_a_time + buffer_size * 2;
        read_buffer = realloc(read_buffer, buffer_size);
        if (!read_buffer) {
          perror("memory");
          exit(EXIT_FAILURE);
        }
      }

      chars_io = fread(read_buffer + buffer_offset, 1, bytes_at_a_time, f);
      if (chars_io <= 0) break;
      buffer_offset += chars_io;
   }

   // Terminate
   read_buffer = realloc(read_buffer, buffer_size + 1);
   read_buffer[buffer_offset] = 0;

   if (chars_io < 0) {
      perror("read");
      exit(EXIT_FAILURE);
   }

   return read_buffer;
}



char *run_command(const char *command) {
   FILE *fpipe = (FILE*) popen(command, "r");
   if (!fpipe) return NULL;
   char *output = fslurp(fpipe);
   pclose(fpipe);
   return output;
}




int32_t render_toast(GRAPHICS_RESOURCE_HANDLE img, uint32_t img_w, uint32_t img_h, const char *text, const uint32_t text_size, const uint32_t y_offset) {

   uint32_t width = 0, height = 0;
   int s = graphics_resource_text_dimensions_ext(img, text, 0, &width, &height, text_size);
   if (s != 0) return s;
   
   int32_t x_offset = ((int32_t) img_w - (int32_t) width) / 2;
   s = graphics_resource_render_text_ext(img, x_offset, y_offset - height,
                                     GRAPHICS_RESOURCE_WIDTH,
                                     GRAPHICS_RESOURCE_HEIGHT,
                                     GRAPHICS_RGBA32(0xff,0xff,0xff,0xff), /* fg */
                                     GRAPHICS_RGBA32(0,0,0,0x80), /* bg */
                                     text, 0, text_size);
   return s;
}



int main(int argc, char *argv[]) {
   // Defaults
   double seconds_duration = 2; 
   uint32_t text_size = 20;
   int loop = 0;
   char *command = NULL;
   char *text = NULL;

   // Parse command-line arguments (getopt)
   int c;
   opterr = 0;
   while ((c = getopt(argc, argv, "c:ils:t:")) != -1)
      switch (c) {
         case 'c':
            command = optarg;
            break;
         case 'i':
            text = fslurp(stdin);
            break;
         case 'l':
            loop = 1;
            break;
         case 's':
            text_size = atoi(optarg);
            break;
         case 't':
            seconds_duration = strtod(optarg, 0);
            break;
         case '?':
            if (optopt == 'c' || optopt == 's' || optopt == 't')
               fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
               fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
               return 1;
         default:
            abort ();
      }
   if (seconds_duration == 0) return 0;
   if (!text && optind < argc) text = argv[optind];
   if (!text && !command) return 1;

   init_screen();
   uint32_t width, height;
   int s = graphics_get_display_size(0, &width, &height);
   assert(s == 0);

   GRAPHICS_RESOURCE_HANDLE img = make_transparent_canvas(width, height);
   uint32_t img_w, img_h;
   graphics_get_resource_size(img, &img_w, &img_h);

   do {
      if (command) text = run_command(command);

      // Draw the toast text
      uint32_t y_offset = height - 60 + text_size/2;
      graphics_resource_fill(img, 0, 0, img_w, img_h, GRAPHICS_RGBA32(0,0,0,0x00));
      render_toast(img, img_w, img_h, text, text_size, y_offset);
      graphics_update_displayed_resource(img, 0, 0, 0, 0);

      // Sleep until time is up (negative duration means infinite)
      if (seconds_duration < 0) sleep(-1);
      else                      usleep(seconds_duration * 1000000);

      if (command) free(text);
   } while (loop);

   graphics_display_resource(img, 0, LAYER, 0, 0, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, VC_DISPMAN_ROT0, 0);
   graphics_delete_resource(img);
   return 0;
}
