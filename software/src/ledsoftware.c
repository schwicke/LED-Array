/**********************************************************************
* Filename    : LEDsoftware.c
* Description : Control LED by 74HC595
* License     : GPL2
* Author      : Ulrich Schwickerath, 2020
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <gpiod.h>
#include "common.h"

#define dataPin 18        // DS pin of 74HC595(Pin14)
#define columnclockPin 24 // CH_CP pin of 74HC595(Pin11)
#define latchPin 12      // ST_CP pin of 74HC595(Pin12)
#define clrbarPin 23      // clear pin
#define fePin 25          // FE pin
#define rowdataPin 16    // block select pin
#define rowclockPin 20   // block clock pin

extern unsigned char *font[];

unsigned int LOW = 0;
unsigned int HIGH = 1;

struct gpiod_chip *chip;
struct gpiod_line *data;
struct gpiod_line *columnclock;
struct gpiod_line *latch;
struct gpiod_line *clrbar;
struct gpiod_line *fe;
struct gpiod_line *rowdata;
struct gpiod_line *rowclock;

// size of the display: we have 7 rows and 9 columns of 8bit size

unsigned char characters[128 * 8];

void delayMicroseconds(int delay){
  usleep(delay);
}

void delay(int delay){
  usleep(1000*delay);
}

void abort(){
  gpiod_line_release(data);
  gpiod_line_release(latch);
  gpiod_line_release(columnclock);
  gpiod_line_release(clrbar);
  gpiod_line_release(fe);
  gpiod_line_release(rowdata);
  gpiod_line_release(rowclock);
  //gpiod_chip_close(chip);
  exit(0);
}

struct gpiod_line * setAsOutput(int bcm){
  struct gpiod_line *gpio_line;
  char consumer[7];
  int result;

  gpio_line = gpiod_chip_get_line(chip, bcm);
  if (!gpio_line) {
    gpiod_chip_close(chip);
    perror("Get line failed\n");
    exit(1);
  }
  (void)sprintf(consumer, "out-%-2d", bcm);
  result = gpiod_line_request_output(gpio_line, consumer, 0);
  if (result < 0) {
    perror("Request line as output failed\n");
    abort();
  }
  return(gpio_line);
}

void writeState(struct gpiod_line *line, unsigned int state){
  int result;
  result = gpiod_line_set_value(line, state);
  if (result < 0){
    perror("Set line output failed\n");
    abort();
  }
}

void init_hardware()
{
  char *chipname = "gpiochip0";
  /* open the chip */
  chip = gpiod_chip_open_by_name(chipname);
  if (!chip) {
    perror("Open chip failed\n");
    exit(1);
  }
  data = setAsOutput(dataPin);
  latch = setAsOutput(latchPin);
  columnclock = setAsOutput(columnclockPin);
  clrbar = setAsOutput(clrbarPin);
  fe = setAsOutput(fePin);
  rowdata = setAsOutput(rowdataPin);
  rowclock = setAsOutput(rowclockPin);
  // enable demultiplexer
  writeState(fe, 1);
  // reset shift registers
  writeState(clrbar, 0);
  writeState(clrbar, 1);
}

void out_byte(struct gpiod_line * dLine, struct gpiod_line * cLine, int val){
  int i;
  for (i = 0; i < 8; i++) {
    writeState(cLine, 0);
    writeState(dLine, ((0x80 & (val << i)) == 0x80) ? 1 : 0);
    writeState(cLine, 1);
  }
}

void out_byte_lsb(struct gpiod_line * dLine, struct gpiod_line * cLine, int val){
  int i;
  for (i = 0; i < 8; i++) {
    writeState(cLine, 0);
    writeState(dLine, ((0x1 & (val >> i)) == 0x1) ? 1 : 0);
    writeState(cLine, 1);
  }
}

void switch_off(){
  // disable demultiplexer
  writeState(fe, 1);
  // reset shift registers
  writeState(clrbar, 0);
  writeState(clrbar, 1);
}

void thread_show_buffer_on_led(void *buffer){
  int             nx = ROWS;
  int             ny = COLUMNS;
  int             i, j;
  unsigned char   x;
  int             block, inside, pos;
  unsigned char  *buff = (unsigned char *) buffer;
  writeState(latch, 0);
  while (1) {
    for (j = 0; j < nx; j++) {
      for (i = 0; i < ny; i++) {
	block = i / 3;
	inside = i - 3 * block;
	pos = 3 * block + 2 - inside;
	x = *(buff + ROWS * pos + j);
	out_byte_lsb(data, columnclock, x ^ 0xff);
	if (inside == 2) {
	  x = j + 8 * block;
	  out_byte(rowdata, rowclock, x);
	  writeState(latch, 1);
	  writeState(latch, 0);
	}
      }
    }
  }
}

int main(int argc, char *argv[]) {
  unsigned char   characterset[CHARSET_SIZE + 1];
  char            textbuffer[LCOLUMNS];

  if (argc<=1){
    printf("Please give the text to be displayed\n");
    exit(1);
  }
  printf("%s", argv[0]);
  printf("%s", argv[1]);
  strncpy(textbuffer, argv[1], LCOLUMNS);

  // initialise the hardware
  init_hardware();
  // initialise text
  // read_character_set(&characterset[0]);
  memcpy(characterset, font, 2048);

  // create the display thread
  create_display_thread(thread_show_buffer_on_led);

  // write some text into the large buffer
  scroll_in_text(characterset, textbuffer);
  abort();
}
