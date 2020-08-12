/*
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 |   IN | 1 |  3 || 4  |   |      | 5v      |     |     |
 |   3 |   9 |   SCL.1 |   IN | 1 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 1 | ALT5 | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | ALT5 | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |   IN | 0 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |   IN | 0 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |   IN | 0 | 15 || 16 | 0 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI |   IN | 0 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO |   IN | 0 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK |   IN | 0 | 23 || 24 | 1 | IN   | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | IN   | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 0 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
*/
/**********************************************************************
* Filename    : FirstLight.c
* Description : Control LED by 74HC595
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
#define CONSUMER        "Consumer"
#endif

#define dataPin 1        // DS pin of 74HC595(Pin14)
#define columnclockPin 5 // CH_CP pin of 74HC595(Pin11)
#define latchPin 26      // ST_CP pin of 74HC595(Pin12)
#define clrbarPin 4      // clear pin
#define fePin 6          // FE pin
#define rowdataPin 27    // block select pin
#define rowclockPin 28   // block clock pin


extern unsigned char *font1[];
extern unsigned char *font2[];

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

unsigned char characters[256 * 8];

void delayMicroseconds(int delay){
  sleep((float)delay/1e6);
}

void delay(int delay){
  sleep((float)delay/1e3);
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

void out_byte(struct gpiod_line * dLine, struct gpiod_line * cLine, int val)
{
  int i;
  for (i = 0; i < 8; i++) {
    writeState(cLine, LOW);
    writeState(dLine, ((0x80 & (val << i)) == 0x80) ? HIGH : LOW);
    delayMicroseconds(10);
    writeState(cLine, HIGH);
    delayMicroseconds(10);
  }
}

void out_byte_lsb(struct gpiod_line * dLine, struct gpiod_line * cLine, int val)
{
  int i;
  for (i = 0; i < 8; i++) {
    writeState(cLine, LOW);
    writeState(dLine, ((0x1 & (val >> i)) == 0x1) ? HIGH : LOW);
    delayMicroseconds(10);
    writeState(cLine, HIGH);
    delayMicroseconds(10);
  }
}

void out_byte_watch(struct gpiod_line * dLine, struct gpiod_line * cLine, struct gpiod_line * lLine, int val)
{
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

void switch_off()
{
  // disable demultiplexer
  writeState(fe, LOW);
  // reset shift registers
  writeState(clrbar, LOW);
  delayMicroseconds(10);
  writeState(clrbar, HIGH);
  delayMicroseconds(10);
}

//void * thread_show_buffer_on_leds(int nx, int ny, unsigned char * buff){
void           *
thread_show_buffer_on_led(void *buffer)
{
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
	// printf("i, j (%d, %d) block %d, inside %d\n", i,
	// j, block, inside);
	x = *(buff + ROWS * pos + j);
	out_byte_lsb(data, columnclock, x ^ 0xff);
	if (inside == 2) {
	  x = j + 8 * block;
	  // printf("0x%x\n", x);
	  out_byte(rowdata, rowclock, x);
	  writeState(latch, HIGH);
	  delayMicroseconds(50);
	  writeState(latch, LOW);
	}
      }
    }
  }
}

int
main(int argc, char *argv[])
{
  unsigned char   characterset[CHARSET_SIZE + 1];

  char            textbuffer[LCOLUMNS];

  // initialise the hardware
  init_hardware();

  // initialise text
  // read_character_set(&characterset[0]);
  memcpy(characterset, font1, 2048);

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
	memcpy((unsigned char *) display_buffer,
	       ((unsigned char *) large_display_buffer),
	       copy);
	delayMicroseconds(50000);
      }
    }
  }
  switch_off();
}
