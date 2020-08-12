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
    for (j=0; j<nx; j++){
      for (i=0; i<ny; i++){
	unsigned char byte = *(((unsigned char *)buff)+ROWS*i + j);
	show_char(byte);
      }
      printf("\n");
    }
    sleep(1);
  }
}


int main(void) {
  
  unsigned char characterset[CHARSET_SIZE+1];
  char textbuffer[LCOLUMNS];

  // initialise text
  memcpy(characterset, font, 1024);

  // create the display thread
  create_display_thread(thread_show_buffer_on_terminal);

  // write some text into the large buffer
  strcpy(textbuffer, "Das Pferd frisst keinen Gurkensalat        ");
  write_string(strlen(textbuffer), (unsigned char*)large_display_buffer, characterset, textbuffer);
  // make a sliding window
  int copy = COLUMNS*ROWS; // fill the full display buffer
  memcpy((unsigned char*)display_buffer, ((unsigned char*)large_display_buffer), copy);
  for (int i=0;i<strlen(textbuffer)*ROWS;i++){
    for (int ii=0;ii<8;ii++){
      for (int jj=0;jj<ROWS;jj++){
        shiftleftonce(jj, ROWS, LCOLUMNS, ((unsigned char*)large_display_buffer));
      }
       memcpy((unsigned char*)display_buffer, ((unsigned char*)large_display_buffer), copy);
      sleep(2);
    }
  }
}
