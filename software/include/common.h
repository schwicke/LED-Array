#ifndef COMMON
#define COMMON
#define CHARSET_SIZE  8 * 512
#define ROWS  7
#define COLUMNS 9
#define LCOLUMNS 2048
/* large display buffer */
unsigned char large_display_buffer[LCOLUMNS*ROWS];
/* the display buffer sliding on the above*/
unsigned char display_buffer[COLUMNS*ROWS];

/* Read character set from file */
void read_character_set(unsigned char * target);

/* write an ASCII string to the buffer */
void write_string(int columns, unsigned char *buf, unsigned char * charset, char * text);

/* create the thread to display the text */
void create_display_thread (void* display_function);

/* shift left once: row to shift, rows=number of rows, columns is number of columns */
void shiftleftonce(int row, int rows, int columns, unsigned char *buf);

#endif
