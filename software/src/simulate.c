/**********************************************************************
* Filename    : simulate.c
* Description : simulate the output on the terminal
* License     : GPL2
* Author      : Ulrich Schwickerath, 2020
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

extern unsigned char* font[];

void show_char(unsigned char x){
  int i;
  unsigned char mask = 0x80;
  for (i=0; i<8; i++){
    if ( x & mask >> i ){
      printf("*");
    } else {
      printf(" ");
    }
  }
}

void * thread_show_buffer_on_terminal(void * buff){
  int i, j;
  int nx = ROWS;
  int ny = COLUMNS;
  while (1){
    printf("\e[1;1H\e[2J");
    for (j=0; j<nx; j++){
      for (i=0; i<ny; i++){
	unsigned char byte = *(((unsigned char *)buff)+ROWS*i + j);
	show_char(byte);
      }
      printf("\n");
    }
    usleep(100000);
  }
}


int main(void) {
  
  unsigned char characterset[CHARSET_SIZE+1];
  char textbuffer[LCOLUMNS];
  int ret;
  // initialise text
  memcpy(characterset, font, 1024);

  // create the display thread
  create_display_thread(thread_show_buffer_on_terminal);

  // write some text into the large buffer
  strcpy(textbuffer, "Das Pferd frisst keinen Gurkensalat");
  ret = system("/usr/bin/clear");
  if (ret == 0){
    scroll_in_text(characterset, textbuffer);
  }
}
