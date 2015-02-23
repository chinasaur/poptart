/*
 * PopTart: toast notifications for RaspberryPi
 * 
 * See README.md for more information.
 * 
 * Copyleft 2012, Peter H. Li
 * 
 * Derived from the RaspberryPi hello_font example program; see Broadcom
 * copyright and license below.
 * 
 * -----
 * 
 * Copyright (c) 2012, Broadcom Europe Ltd
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    * Neither the name of the copyright holder nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#include "bcm_host.h"
#include "vgfont.h"

const char *FONTDIR = "/opt/vc/src/hello_pi/hello_font"; // Path to Vera.ttf
int LAYER = 1;

void init_screen() {
   bcm_host_init();
   int s = gx_graphics_init(FONTDIR);
   assert(s == 0);
}

/**
 * Not really clear why we make a window the whole screen size instead of just 
 * what's needed for the text.  We should be able to get the text extent 
 * without an existing resource if we include vgft.h, or if we pass a dummy
 * res to the existing get_text_extents, or if we copy the logic over.
 */
GRAPHICS_RESOURCE_HANDLE make_transparent_canvas(uint32_t width, uint32_t height) {
   GRAPHICS_RESOURCE_HANDLE img;
   int s;

   s = gx_create_window(0, width, height, GRAPHICS_RESOURCE_RGBA32, &img);
   assert(s == 0);
   
   // Make it transparent
   graphics_resource_fill(img, 0, 0, width, height, GRAPHICS_RGBA32(0, 0, 0, 0));

   // Show it
   graphics_display_resource(img, 0, LAYER, 0, 0, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, VC_DISPMAN_ROT0, 1);

   return img;
}

// Caller must free read_buffer
// Adapted from http://stackoverflow.com/questions/1151029/unix-linux-ipc-reading-from-a-pipe-how-to-know-length-of-data-at-runtime
char *slurp(int filedes) {
   const int bytes_at_a_time = 2;
   char *read_buffer = NULL;
   int buffer_size = 0;
   int buffer_offset = 0;
   int chars_io;
   do {
      if (buffer_offset + bytes_at_a_time > buffer_size) {
        buffer_size = bytes_at_a_time + buffer_size * 2;
        read_buffer = realloc(read_buffer, buffer_size);
        if (!read_buffer) {
          perror("realloc");
          exit(EXIT_FAILURE);
        }
      }

      // This will block if FILEDES is still open but no bytes are available, 
      // unless FILEDES is marked O_NONBLOCK
      chars_io = read(filedes, read_buffer + buffer_offset, bytes_at_a_time);

      // If < 0, there is an error
      if (chars_io < 0) {

        // If FILEDES is marked O_NONBLOCK, then if FILEDES is still open
        // but no chars are available READ will return with -1 and set
        // errno this way.
        if (errno == EAGAIN) break;

        perror("read");
        exit(EXIT_FAILURE);
      }

      buffer_offset += chars_io;
   } while (chars_io > 0); // If == 0, there was nothing left

   // Don't forget to null-terminate!
   read_buffer = realloc(read_buffer, buffer_offset + 1);
   read_buffer[buffer_offset] = 0;

   return read_buffer;
}

// Caller must free read_buffer
char *fslurp(FILE *f) {
  int filedes = fileno(f);
  return slurp(filedes);
}

char *run_command(const char *command) {
   FILE *fpipe = (FILE*) popen(command, "r");
   if (!fpipe) {
     perror("popen");
     return NULL;
   }
   char *output = fslurp(fpipe);
   pclose(fpipe);
   return output;
}

char *run_commandi(const char *command, int i) {
  int n = snprintf(NULL, 0, command, i) + 1;
  char *commandi = malloc(n * sizeof(char));
  if (!commandi) {
    perror("malloc");
    return NULL;
  }
  snprintf(commandi, n, command, i);

  char *ret = run_command(commandi);
  free(commandi);
  return ret;
}

int32_t render_toast(
    const GRAPHICS_RESOURCE_HANDLE img, const uint32_t img_w,
    const uint32_t img_h, const int32_t x_offset, const int32_t y_offset,
    const char *text, const uint32_t text_size,
    const uint32_t fg_rgba, const uint32_t bg_rgba) {
  // Clear existing.
  int32_t s = graphics_resource_fill(
      img, 0, 0, img_w, img_h, GRAPHICS_RGBA32(0, 0, 0, 0));

  s = graphics_resource_render_text_ext(
      img, x_offset, y_offset,
      GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT,
      fg_rgba, bg_rgba,
      text, 0, text_size);
  if (s != 0) return s;
 
  graphics_update_displayed_resource(img, 0, 0, 0, 0);
  return s;
}

double elapsed(struct timeval *init) {
   struct timeval now;
   int32_t esec;
   int32_t eusec;
   double e;

   gettimeofday(&now, NULL);
   esec  = now.tv_sec  - init->tv_sec;
   eusec = now.tv_usec - init->tv_usec;
   e = esec + (eusec / 1000000.0);
   return e;
}

int32_t render_toast_scroll(
    const GRAPHICS_RESOURCE_HANDLE img, 
    const uint32_t img_w, const uint32_t img_h, 
    const char *text, const uint32_t text_size, const uint32_t user_y_offset,
    const uint32_t fg_rgba, const uint32_t bg_rgba,
    const double scroll_update, const int32_t scroll_step,
    const double seconds_duration) {
  uint32_t width, height;
  int s = graphics_resource_text_dimensions_ext(
      img, text, 0, &width, &height, text_size);
  if (s != 0) return s;
  int32_t swidth = width;
  int32_t x_offset = img_w;
  const int32_t y_offset = img_h - height - user_y_offset;

  struct timeval init;
  gettimeofday(&init, NULL);
  double e, next_scroll;
  for (e = 0.0, next_scroll = 0.0; 
       seconds_duration < 0.0 || e < seconds_duration;
       e = elapsed(&init)) {

    // Is it time to update?
    if (e < next_scroll) {
      usleep((next_scroll - e) * 1000000);
      continue;
    }
      
    s = render_toast(
        img, img_w, img_h, x_offset, y_offset, text, text_size, fg_rgba, bg_rgba);
    if (s != 0) return s;
      
    x_offset -= scroll_step;
    if (x_offset + swidth < 0) x_offset = img_w;

    next_scroll += scroll_update;
  }

  return s;
}

int32_t render_toast_static(
    const GRAPHICS_RESOURCE_HANDLE img, 
    const uint32_t img_w, const uint32_t img_h, 
    const char *text, const uint32_t text_size, const uint32_t user_y_offset,
    const uint32_t fg_rgba, const uint32_t bg_rgba,
    const double seconds_duration) {
  uint32_t width, height;
  int s = graphics_resource_text_dimensions_ext(img, text, 0, &width, &height, text_size);
  if (s != 0) return s;
  const int32_t x_offset = ((int32_t) img_w - (int32_t) width) / 2;
  const uint32_t y_offset = img_h - height - user_y_offset;

  s = render_toast(img, img_w, img_h, x_offset, y_offset, text, text_size, fg_rgba, bg_rgba);
  if (s != 0) return s;

  // Sleep until time is up (negative duration means infinite)
  if (seconds_duration < 0) sleep(-1);
  else usleep(seconds_duration * 1000000);
  return s;
}

void print_help(void) {
  printf(""
"Usage: poptart [OPTIONS] [MESSAGE]\n"
"\n"
"poptart can be used in several different modes.  The standard mode is with a\n"
"MESSAGE string passed as an argument; MESSAGE is displayed.  The string to\n"
"display can also be piped; use the -i flag to instruct poptart to read from\n"
"STDIN for its message.  The last mode is command mode; use the -c flag\n"
"followed by a command string; poptart will run this command in the shell\n"
"and use the command's STDOUT output as the display string.\n"
"\n"
"Command mode is most useful when combined with the -l flag, which causes\n"
"poptart to loop and repeatedly call the command string.\n"
"\n"
"Other recognized OPTION flags:\n"
"  -h         Show this help\n"
"  -s SIZE    Set text font size\n"
"  -t SEC     Set duration for string to be displayed\n"
"  -m PIX     Set the scroll step size.  By default, MESSAGE is displayed\n"
"             centered and static.  If PIX is > 0, then MESSAGE scrolls right\n"
"             to left by PIX pixels each scroll update.\n"
"  -n SEC     Set the scroll update frequency.  Anything greater than 0.03\n"
"             (30 ms) will tend to be noticeable.  If set to 0 the updates \n"
"             will be as fast as possible, which on my Pi seems to be about\n"
"             every 10 ms with no other graphics load.\n"
"  -y OFFSET  Set the y-axis offset.  The bottom of the text will be this many\n"
"             pixels above the bottom of the screen.\n"
  );
}

int main(int argc, char *argv[]) {

   // Defaults
   double seconds_duration = 2; 
   uint32_t text_size = 20;
   int loop = 0;
   double scroll_update = 0.03;
   uint32_t scroll_step = 0; // 0 means render_toast_static, otherwise 
                             // render_toast_scroll

   // Parse command-line arguments (getopt)
   unsigned fg_r = 255, fg_g = 255, fg_b = 255, fg_a = 255;
   unsigned bg_r = 0, bg_g = 0, bg_b = 0, bg_a = 0;
   char *command = NULL;
   char *text = NULL;
   int in;
   int c;
   opterr = 0;
   uint32_t user_y_offset = 60;
   while ((c = getopt(argc, argv, "b:c:f:hilm:n:s:t:y:")) != -1)
      switch (c) {
         case 'b':
            if (sscanf(optarg, "%u,%u,%u,%u", &bg_r, &bg_g, &bg_b, &bg_a) != 4) {
               fprintf(stderr, "Failed to parse background RGBA.\n");
               return EXIT_FAILURE;
            }
            break;
         case 'c':
            command = optarg;
            break;
         case 'f':
            if (sscanf(optarg, "%u,%u,%u,%u", &fg_r, &fg_g, &fg_b, &fg_a) != 4) {
               fprintf(stderr, "Failed to parse foreground RGBA.\n");
               return EXIT_FAILURE;
            }
            break;
         case 'i':
            in = open("/dev/stdin", O_RDONLY | O_NONBLOCK);
            text = slurp(in);
            break;
         case 'h':
            print_help();
            return EXIT_SUCCESS;
         case 'l':
            loop = 1;
            break;
         case 'm':
            scroll_step = atoi(optarg);
            break;
         case 'n':
            scroll_update = strtod(optarg, 0);
            break;
         case 's':
            text_size = atoi(optarg);
            break;
         case 't':
            seconds_duration = strtod(optarg, 0);
            break;
	 case 'y':
	    user_y_offset = atoi(optarg);
	    break;
         case '?':
            if (optopt == 'c' || optopt == 's' || optopt == 't' || optopt == 'y')
               fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
               fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
               fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
            print_help();
            return EXIT_FAILURE;
         default:
            abort();
      }
   if (seconds_duration == 0) return 0;
   if (!text && optind < argc) text = argv[optind];
   if (!text && !command) return 1;

   // Convert color args to float.
   // Upstream Raspi vgfont.h library currently parses these as BGRA...
   uint32_t fg_rgba = GRAPHICS_RGBA32(fg_b, fg_g, fg_r, fg_a);
   uint32_t bg_rgba = GRAPHICS_RGBA32(bg_b, bg_g, bg_r, bg_a);

   // Init, get display width & height
   init_screen();
   uint32_t width, height;
   int s = graphics_get_display_size(0, &width, &height);
   assert(s == 0);

   // Create and display canvas (initialized as transparent)
   GRAPHICS_RESOURCE_HANDLE img = make_transparent_canvas(width, height);
   uint32_t img_w, img_h;
   graphics_get_resource_size(img, &img_w, &img_h);
   

   // Only actually loop if requested from user, otherwise runs once
   unsigned int loopcount = 0;
   do {
      if (command) text = run_commandi(command, loopcount);
      
      // perror should already have been called for report
      if (!text) {
        fprintf(stderr, "Error running command: %s", command);
        exit(EXIT_FAILURE); 
      }
      
      // Draw the toast text
      if (!scroll_step) {
         render_toast_static(img, img_w, img_h, text, text_size, user_y_offset,
            fg_rgba, bg_rgba, seconds_duration);
      } else {
         render_toast_scroll(img, img_w, img_h, text, text_size, user_y_offset,
            fg_rgba, bg_rgba, scroll_update, scroll_step, seconds_duration);
      }
      
      if (command) free(text);
      ++loopcount;
   } while (loop);

   graphics_delete_resource(img);
   return EXIT_SUCCESS;
}
