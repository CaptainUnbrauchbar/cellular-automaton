#ifndef BITMAP
#define BITMAP
/* (c) 1996 Peter Sanders, Thomas Worsch */
/* minimalistic module for displaying bitmaps on an X-Windows Screen.
 * uses the sequential parts of xlk
 */

#ifdef __cplusplus
#define CC extern "C"
#else
#define CC
#endif

extern int bitmapXSize;
extern int bitmapYSize;
extern unsigned char *bitmapBuffer;

/* some utitility macros */
#define bitmapClearPixel(x, y)\
  (bitmapBuffer[(y) * ((bitmapXSize + 7)/8) + ((x)>>3)] |=  (128 >> ((x) & 7)))
#define bitmapSetPixel(x, y)\
  (bitmapBuffer[(y) * ((bitmapXSize + 7)/8) + ((x)>>3)] &= ~(128 >> ((x) & 7)))


/* initialize a bitmap of size x times y.
 * display gives the name of the display to be used for
 * opening the window. E.g. "i90xxx.ira.uka.de:0.0" 
 * return a pointer to an array of y * ceil(x / 8) bytes
 * which can be written with the image data.
 * Data is stored line by line, 8 bits per byte
 */
CC void bitmapInit(int x, int y, char *display);


/* display the data which is present in the
 * buffer returned by bitmapInit
 */
CC void bitmapDisplay(void);

#endif
