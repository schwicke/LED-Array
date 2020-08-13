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

#ifndef CONSUMER
#define CONSUMER "LEDband"
#endif

#define dataPin 1        // DS pin of 74HC595(Pin14)
#define columnclockPin 5 // CH_CP pin of 74HC595(Pin11)
#define latchPin 26      // ST_CP pin of 74HC595(Pin12)
#define clrbarPin 4      // clear pin
#define fePin 6          // FE pin
#define rowdataPin 27    // block select pin
#define rowclockPin 28   // block clock pin


extern unsigned char *font[];

unsigned int LOW = 0;
unsigned int HIGH = 0xffffff;

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
  gpiod_chip_close(chip);
  exit(0);
}

struct gpiod_line * setAsOutput(int pin){
  struct gpiod_line *gpio_line;
  int result;

  gpio_line = gpiod_chip_get_line(chip, pin);
  if (!gpio_line) {
    gpiod_chip_close(chip);
    perror("Get line failed\n");
    exit(1);
  }
  result = gpiod_line_request_output(gpio_line, CONSUMER, 0);
  if (result < 0) {
    perror("Request line as output failed\n");
    abort();
  }
  return(gpio_line);
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
}

void writeState(struct gpiod_line *line, unsigned int state){
  int result;
  result = gpiod_line_set_value(line, state);
  if (result < 0){
    perror("Set line output failed\n");
    abort();
  }
}

void out_byte(struct gpiod_line * dLine, struct gpiod_line * cLine, int val){
  int i;
  for (i = 0; i < 8; i++) {
    writeState(cLine, LOW);
    writeState(dLine, ((0x80 & (val << i)) == 0x80) ? HIGH : LOW);
    delayMicroseconds(10);
    writeState(cLine, HIGH);
    delayMicroseconds(10);
  }
}

void out_byte_lsb(struct gpiod_line * dLine, struct gpiod_line * cLine, int val){
  int i;
  for (i = 0; i < 8; i++) {
    writeState(cLine, LOW);
    writeState(dLine, ((0x1 & (val >> i)) == 0x1) ? HIGH : LOW);
    delayMicroseconds(10);
    writeState(cLine, HIGH);
    delayMicroseconds(10);
  }
}

void out_byte_watch(struct gpiod_line * dLine, struct gpiod_line * cLine, struct gpiod_line * lLine, int val){
  int i;
  for (i = 0; i < 8; i++) {
    writeState(cLine, LOW);
    writeState(dLine, ((0x80 & (val << i)) == 0x80) ? HIGH : LOW);
    delayMicroseconds(10);
    writeState(cLine, HIGH);
    delayMicroseconds(10);
    writeState(lLine, HIGH);
    delayMicroseconds(10);
    writeState(lLine, LOW);
    delay(500);
  }
}

void switch_off(){
  // disable demultiplexer
  writeState(fe, LOW);
  // reset shift registers
  writeState(clrbar, LOW);
  delayMicroseconds(10);
  writeState(clrbar, HIGH);
  delayMicroseconds(10);
}

void * thread_show_buffer_on_led(void *buffer){
  int             nx = ROWS;
  int             ny = COLUMNS;
  int             i, j;
  unsigned char   x;
  int             block, inside, pos;
  unsigned char  *buff = (unsigned char *) buffer;
  writeState(latch, LOW);
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
	  writeState(latch, HIGH);
	  delayMicroseconds(50);
	  writeState(latch, LOW);
	}
      }
    }
  }
}

int main(int argc, char *argv[]) {
  unsigned char   characterset[CHARSET_SIZE + 1];
  char            textbuffer[LCOLUMNS];

  // initialise the hardware
  init_hardware();

  // initialise text
  // read_character_set(&characterset[0]);
  memcpy(characterset, font, 2048);

  // create the display thread
  create_display_thread(thread_show_buffer_on_led);

  // write some text into the large buffer
  strcpy(textbuffer, "        Hello, World !!!       ");
  write_string(strlen(textbuffer), (unsigned char *) large_display_buffer, characterset, textbuffer);

  // make a sliding window
  int             copy = COLUMNS * ROWS;	// fill the full display buffer

  for (int replay = 0; replay < 3; replay++) {
    // initialize the text
    write_string(strlen(textbuffer), (unsigned char *) large_display_buffer, characterset, textbuffer);
    memcpy((unsigned char *) display_buffer, ((unsigned char *) large_display_buffer), copy);
    for (int i = 0; i < strlen(textbuffer); i++) {
      for (int ii = 0; ii < 8; ii++) {
	for (int jj = 0; jj < ROWS; jj++) {
	  shiftleftonce(jj, ROWS, LCOLUMNS, ((unsigned char *)large_display_buffer));
	}
	memcpy((unsigned char *) display_buffer, ((unsigned char *) large_display_buffer), copy);
	delayMicroseconds(50000);
      }
    }
  }
  switch_off();
  abort();
}
