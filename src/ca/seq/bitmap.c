/* implementation of bitmap displaying using 
 * X-Images
 *
 * (c) 1996 Peter Sanders
 * freely  distributable
 *
 * The basic XLib harness is taken from the xlk Library
 * from Pascal O. Luthi as modified by Thomas Worsch
 * refer to the docs in bitmap.h also
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "bitmap.h"

#define XBytes ((bitmapXSize + 7) >> 3)

int bitmapXSize;
int bitmapYSize;
unsigned char *bitmapBuffer;


/************ pruned version of xlk.c ********************************/


/* Xlib include files */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#define Debug(A) 

#define XLK_DISP_NAME_MAXLEN 256

static int	screen_num;
static Display    *display;
static GC               gc;
static Window          win;
static XImage    *xlkImage;

static unsigned int xlkWinWidth;   /* Width of the window    */
static unsigned int xlkWinHeight;  /* Height of the window   */

static int xlkWinSize;
static int *xlkWinData;



static void getGC(GC *gc);

/* build a b/w image covering the display */
static void initImage(Visual *visual)
{  xlkImage = XCreateImage(
                display, 
                visual,
                1, /* depth */
                XYBitmap,
                0,
                (char*)bitmapBuffer,
                xlkWinWidth,xlkWinHeight,
                8,
                0); 
}
  
static char  display_name[XLK_DISP_NAME_MAXLEN];
                                  /* Standard properties               */
static    XWMHints      *wm_hints;
static    XClassHint    *class_hints;
static    XSizeHints    *size_hints;
static    XTextProperty windowName,iconName;
static    Pixmap        icon_pixmap;

static void xlkInit(int dimx, int dimy, char *title, char *dispname)
/* Initialisation of the window: Processor 0 is calling the basic
 * X routines to open a window. Standard properties are set. 
 */
{
    int border_width = 4;         /* Border 4 pixels wide              */
    int x=0,y=0;                  /* Window position on the screen     */

                                  /* Color                             */
    Visual   *visual;

    /* allocate storage for some global data structures */
    xlkWinWidth  = dimx;
    xlkWinHeight = dimy;
    xlkWinSize = xlkWinWidth*xlkWinHeight;
    xlkWinData = (int *)malloc(xlkWinSize*sizeof(int));
      
    strcpy(display_name, dispname);

    /* Connection to a Display          */
    Debug(printf("Display name:%s\n", display_name)); 
    display    = XOpenDisplay(display_name);

    /* An important global variable     */
    screen_num = DefaultScreen(display);

    /* Create opaque Simple Window      */
    visual = DefaultVisual(display,screen_num);

    win = XCreateSimpleWindow(display, 
                              RootWindow(display,screen_num),
                              x,y,xlkWinWidth,xlkWinHeight,border_width,
                              BlackPixel(display,screen_num),
                              WhitePixel(display,screen_num));

    /* ==================== fiddling with colors ==================== */

    /* Set the Standard properties      */
    size_hints  = XAllocSizeHints();
    wm_hints    = XAllocWMHints();
    class_hints = XAllocClassHint();
    /* Set size hints for window manager; the window manager may 
     * override these settings */
    /* Note that in a real application, if size or position were set by
     * the user, the flags would be USPosition and USSize and these would
     * override the window manager's preferences for this window  */
    size_hints->flags     = PPosition | PSize | PMinSize | PMaxSize;
    size_hints->min_width = xlkWinWidth;
    size_hints->min_height= xlkWinHeight;
    size_hints->max_width = xlkWinWidth;
    size_hints->max_height= xlkWinHeight;
    size_hints->width     = xlkWinWidth;
    size_hints->height    = xlkWinHeight;

    /* set window name and icon name to title */
    XStringListToTextProperty( &title, 1, &windowName );
    XStringListToTextProperty( &title, 1, &iconName );

    wm_hints->initial_state = NormalState;
    wm_hints->input = True;
    wm_hints->icon_pixmap = icon_pixmap;
    wm_hints->flags = StateHint | IconPixmapHint | InputHint;

    class_hints->res_name  = "Pipe Stdout";
    class_hints->res_class = "SP2 display";

    XSetWMProperties( display, win, &windowName, &iconName, 0, 0,
                      size_hints, wm_hints, class_hints );

    /* Select event type wanted             */
    XSelectInput( display, win, ExposureMask | KeyPressMask);

    /* Create GC for test and drawing       */
    getGC(&gc);

    /* Display window                       */
    XMapWindow(display,win);

    initImage(visual);
}


/* draw xlkImage on win */
static void xlkDrawImage(void)
{  
   Debug(printf("XSetForeground:%p, %p\n", display, gc));
   XSetForeground(display,gc,0);
   XPutImage(display, win, gc, xlkImage, 
             0, 0, 0, 0, xlkWinWidth,xlkWinHeight);
}


static void getGC(GC *gc)
{
    unsigned long valuemask = 0; /* Ignore XGCvalues and use defaults */
    XGCValues	values;
    *gc = XCreateGC(display, win, valuemask, &values );
}


/*******************************************************************/


void bitmapInit(int x, int y, char *displayName)
{  
   bitmapXSize = x;
   bitmapYSize = y; 
   bitmapBuffer   = calloc(y * XBytes, 1);
   memset(bitmapBuffer, 255, y * XBytes);
   xlkInit(x, y, "Bitmap", displayName); 
   XFlush(display);
}


void bitmapDisplay(void)
{  xlkDrawImage(); 
   XFlush(display);
}

