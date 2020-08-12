#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "common.h"
extern unsigned char *font1[];
extern unsigned char *font2[];


// create the display thread
void create_display_thread(void *display_function) {
  pthread_t displaythread;
  int rc = pthread_create(&displaythread, NULL, display_function, (void *) &display_buffer[0]);
  if (rc) {
    printf("Cannot create the display thread\n");
    exit(-1);
  }
  sleep(1);
}

void read_character_set(unsigned char *target) {
  printf("read character rom\n");
  FILE *fp = fopen("chargen", "rb");
  size_t rc = fread(target, CHARSET_SIZE, 1, fp);
  if (rc <= 0) {
    printf("Failed to open character set file");
    exit(-1);
  }
  fclose(fp);
}

void write_ascii(unsigned char *buf, unsigned char *source, unsigned int asciicode){
  memcpy(buf, source + 8 * asciicode, 7);
}

void write_string(int columns, unsigned char *buf, unsigned char *charset, char *text){
  // for C64 fonts @ is at 0
  // int offset = (int)'@';
  int offset = 0;
  int flag = 0;
  for (int i = 0; i < columns; i++) {
    int num = (int) *(text + i);
    if (num == 0 || flag) {
      num = 32;
      flag = 1;
    } else {
      num = num - offset;
    }
    write_ascii(buf + ROWS * i, charset, num);
  }
}

void shiftleftonce(int row, int rows, int columns, unsigned char *buf){
  int i, j;
  int ov[columns];
  
  for (j = 0; j < columns; j++) {
    /* create the overflow */
    i = row + j * rows;
    if ((buf[i] & 0x80) == 0) {
      ov[j] = 0;
    } else {
      ov[j] = 1;
    }
  }
  for (j = 0; j < columns; j++) {
    i = row + j * rows;
    if (j < columns - 1) {
      buf[i] = (buf[i] << 1) | ov[j + 1];
    } else {
      buf[i] = buf[i] << 1;
    }
  }
}
