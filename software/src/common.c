/**********************************************************************
* Filename    : common.c
* Description : Common methods
* License     : GPL2
* Author      : Ulrich Schwickerath, 2020
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

unsigned char mirror_byte(unsigned char byte){
  /* returns a mirrored version of byte */
  byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
  byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
  byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
  return byte;
}

void create_display_thread(void *display_function) {
  /* create the display thread */
  pthread_t displaythread;
  int rc = pthread_create(&displaythread, NULL, display_function, (void *) &display_buffer[0]);
  if (rc) {
    printf("Cannot create the display thread\n");
    exit(-1);
  }
  sleep(1);
}

void write_ascii(unsigned char *buf, unsigned char *source, unsigned int asciicode){
  /* render a mirrored character based on its ASCII code */
  for (int i=0; i<8; i++){
    unsigned char byte = mirror_byte(*(source + 8 * asciicode + i));
    *(buf+i) = byte;
  }
}

void write_string(int columns, unsigned char *buf, unsigned char *charset, char *text){
  /* write a string into a buffer 
  Note:  int offset = (int)'@'; */
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
  /* scroll the text left by one bit */
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
