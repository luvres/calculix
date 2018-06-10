#ifndef __win32_x11_h__
#define __win32_x11_h__

/* Copyright (c) Nate Robins, 1997. */

/* This program is freely distributable without licensing fees 
   and is provided without guarantee or warrantee expressed or 
   implied. This program is -not- in the public domain. */


#include <windows.h>


/* Type definitions (conversions) */
typedef int Visual;			/* Win32 equivalent of X11 type */
typedef HWND Window;
typedef HPALETTE Colormap;
typedef PIXELFORMATDESCRIPTOR XVisualInfo;
typedef BOOL Bool;
typedef MSG XEvent;
typedef HDC Display;
typedef HCURSOR Cursor;

typedef int Atom;			/* dummies */
typedef int XDevice;
typedef int Status;

#define True  TRUE			/* Win32 equivalents of X11 booleans */
#define False FALSE


#define None                 0L	/* universal null resource or null atom */

/* Input Event Masks. Used as event-mask window attribute and as arguments
   to Grab requests.  Not to be confused with event names.  */

#define NoEventMask			0L
#define KeyPressMask			(1L<<0)  
#define KeyReleaseMask			(1L<<1)  
#define ButtonPressMask			(1L<<2)  
#define ButtonReleaseMask		(1L<<3)  
#define EnterWindowMask			(1L<<4)  
#define LeaveWindowMask			(1L<<5)  
#define PointerMotionMask		(1L<<6)  
#define PointerMotionHintMask		(1L<<7)  
#define Button1MotionMask		(1L<<8)  
#define Button2MotionMask		(1L<<9)  
#define Button3MotionMask		(1L<<10) 
#define Button4MotionMask		(1L<<11) 
#define Button5MotionMask		(1L<<12) 
#define ButtonMotionMask		(1L<<13) 
#define KeymapStateMask			(1L<<14)
#define ExposureMask			(1L<<15) 
#define VisibilityChangeMask		(1L<<16) 
#define StructureNotifyMask		(1L<<17) 
#define ResizeRedirectMask		(1L<<18) 
#define SubstructureNotifyMask		(1L<<19) 
#define SubstructureRedirectMask	(1L<<20) 
#define FocusChangeMask			(1L<<21) 
#define PropertyChangeMask		(1L<<22) 
#define ColormapChangeMask		(1L<<23) 
#define OwnerGrabButtonMask		(1L<<24) 

/* Key masks. Used as modifiers to GrabButton and GrabKey, results of
   QueryPointer, state in various key-, mouse-, and button-related
   events. */

#define ShiftMask		(1<<0)
#define LockMask		(1<<1)
#define ControlMask		(1<<2)
#define Mod1Mask		(1<<3)
#define Mod2Mask		(1<<4)
#define Mod3Mask		(1<<5)
#define Mod4Mask		(1<<6)
#define Mod5Mask		(1<<7)

/* Window classes used by CreateWindow */
/* Note that CopyFromParent is already defined as 0 above */

#define InputOutput		1
#define InputOnly		2

/* Window attributes for CreateWindow and ChangeWindowAttributes */

#define CWBackPixmap		(1L<<0)
#define CWBackPixel		(1L<<1)
#define CWBorderPixmap		(1L<<2)
#define CWBorderPixel           (1L<<3)
#define CWBitGravity		(1L<<4)
#define CWWinGravity		(1L<<5)
#define CWBackingStore          (1L<<6)
#define CWBackingPlanes	        (1L<<7)
#define CWBackingPixel	        (1L<<8)
#define CWOverrideRedirect	(1L<<9)
#define CWSaveUnder		(1L<<10)
#define CWEventMask		(1L<<11)
#define CWDontPropagate	        (1L<<12)
#define CWColormap		(1L<<13)
#define CWCursor	        (1L<<14)

/* ConfigureWindow structure */

#define CWX			(1<<0)
#define CWY			(1<<1)
#define CWWidth			(1<<2)
#define CWHeight		(1<<3)
#define CWBorderWidth		(1<<4)
#define CWSibling		(1<<5)
#define CWStackMode		(1<<6)


/* Used in GetWindowAttributes reply */

#define IsUnmapped		0
#define IsUnviewable		1
#define IsViewable		2

/* Window stacking method (in configureWindow) */

#define Above                   0
#define Below                   1
#define TopIf                   2
#define BottomIf                3
#define Opposite                4

/* For CreateColormap */

#define AllocNone		0	/* create map with no entries */
#define AllocAll		1	/* allocate entire map writeable */


/* Flags used in StoreNamedColor, StoreColors */

#define DoRed			(1<<0)
#define DoGreen			(1<<1)
#define DoBlue			(1<<2)

/* 
 * Bitmask returned by XParseGeometry().  Each bit tells if the corresponding
 * value (x, y, width, height) was found in the parsed string.
 */
#define NoValue		0x0000
#define XValue  	0x0001
#define YValue		0x0002
#define WidthValue  	0x0004
#define HeightValue  	0x0008
#define AllValues 	0x000F
#define XNegative 	0x0010
#define YNegative 	0x0020

/* flags argument in size hints */
#define USPosition	(1L << 0) /* user specified x, y */
#define USSize		(1L << 1) /* user specified width, height */

/* definitions for initial window state */
#define WithdrawnState 0	/* for windows that are not mapped */
#define NormalState 1	/* most applications want to start this way */
#define IconicState 3	/* application wants to start as an icon */


/* Type definitions */

typedef struct {
    unsigned int background_pixmap;	/* background pixmap */
    unsigned long background_pixel;	/* background pixel */
    unsigned long border_pixel;	/* border pixel value */
    long event_mask;		/* set of events that should be saved */
    long do_not_propagate_mask;	/* set of events that should not propagate */
    Bool override_redirect;	   /* boolean value for override-redirect */
    Colormap colormap;		   /* color map to be associated with window */
} XSetWindowAttributes;

typedef struct {
  unsigned long pixel;
  unsigned short red, green, blue;
  char flags;  /* do_red, do_green, do_blue */
} XColor;

typedef struct {
  unsigned char *value;	   /* same as Property routines */
  Atom encoding;	   /* prop type */
  int format;		   /* prop data format: 8, 16, or 32 */
  unsigned long nitems;	   /* number of data items in value */
} XTextProperty;

typedef struct {
  long flags;	        /* marks which fields in this structure are defined */
  int x, y;		/* obsolete for new window mgrs, but clients */
  int width, height;	/* should set so old wm's don't mess up */
} XSizeHints;


/* Function prototypes */

Window
XCreateWindow(Display* display, Window parent, int x, int y,
	      unsigned int width, unsigned int height, unsigned int border,
	      int depth, unsigned int class, Visual* visual, 
	      unsigned long valuemask, XSetWindowAttributes* attributes);

XVisualInfo*
XGetVisualInfo(Display* display, long mask, XVisualInfo* template, int*nitems);

Colormap
XCreateColormap(Display* display, Window root, Visual* visual, int alloc);

void
XAllocColorCells(Display* display, Colormap colormap, Bool contig, 
		 unsigned long plane_masks_return[], unsigned int nplanes,
		 unsigned long pixels_return[], unsigned int npixels);

void
XStoreColor(Display* display, Colormap colormap, XColor* color);

void
XSetWindowColormap(Display* display, Window window, Colormap colormap);

void
XFreeColormap(Display* display, Colormap colormap);

Cursor
XCreateFontCursor(Display* display, char* shape);

void
XDefineCursor(Display* display, Window window, Cursor cursor);

void
XFlush(Display* display);

Bool
XTranslateCoordinates(Display *display, Window src, Window dst, 
		      int src_x, int src_y, 
		      int* dest_x_return, int* dest_y_return,
		      Window* child_return);

Status
XGetGeometry(Display* display, Window window, Window* root_return, 
	     int* x_return, int* y_return, 
	     unsigned int* width_return, unsigned int* height_return,
	     unsigned int *border_width_return, unsigned int* depth_return);

int
DisplayWidth(Display* display, int screen);

int
DisplayHeight(Display* display, int screen);

int
DisplayWidthMM(Display* display, int screen);

int
DisplayHeightMM(Display* display, int screen);

void
XMapWindow(Display* display, Window window);

void
XUnmapWindow(Display* display, Window window);

void
XIconifyWindow(Display* display, Window window, int screen);

void
XWithdrawWindow(Display* display, Window window, int screen);

void
XLowerWindow(Display* display, Window window);

void
XWarpPointer(Display* display, Window src, Window dst, 
	     int src_x, int src_y, int src_width, int src_height, 
	     int dst_x, int dst_y);

void
XSetWMName(Display* display, Window window, XTextProperty *tp);

void
XSetWMIconName(Display* display, Window window, XTextProperty *tp);

int
XPending(Display* display);

void
XDestroyWindow(Display* display, Window window);

void
XFree(void* data);

void
XUngrabPointer(Display* display, int time);

int
XParseGeometry(char* string, int* x, int* y, 
	       unsigned int* width, unsigned int* height);

#endif /* __win32_x11_h__ */
