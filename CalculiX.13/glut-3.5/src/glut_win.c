/* Copyright (c) Mark J. Kilgard, 1994.  */

/* This program is freely distributable without licensing fees
   and is provided without guarantee or warrantee expressed or
   implied. This program is -not- in the public domain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#if !defined(WIN32)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>  /* for XA_RGB_DEFAULT_MAP atom */
#if defined(__vms)
#include <X11/StdCmap.h>  /* for XmuLookupStandardColormap */
#else
#include <X11/Xmu/StdCmap.h>  /* for XmuLookupStandardColormap */
#endif
#endif

/* SGI optimization introduced in IRIX 6.3 to avoid X server
   round trips for interning common X atoms. */
#if defined(_SGI_EXTRA_PREDEFINES) && !defined(NO_FAST_ATOMS)
#include <X11/SGIFastAtom.h>
#else
#define XSGIFastInternAtom(dpy,string,fast_name,how) XInternAtom(dpy,string,how)
#endif

#include <GL/glut_cgx.h>
#include "glutint.h"

GLUTwindow *__glutCurrentWindow = NULL;
GLUTwindow **__glutWindowList = NULL;
int __glutWindowListSize = 0;
GLUTstale *__glutStaleWindowList = NULL;

void (*__glutFreeOverlayFunc) (GLUToverlay *);
XVisualInfo *(*__glutDetermineVisualFromString) (char *string, Bool * treatAsSingle,
  Criterion * requiredCriteria, int nRequired, int requiredMask) = NULL;

static Criterion requiredWindowCriteria[] =
{
  {LEVEL, EQ, 0},
  {TRANSPARENT, EQ, 0}
};
static int numRequiredWindowCriteria = sizeof(requiredWindowCriteria) / sizeof(Criterion);
static int requiredWindowCriteriaMask = (1 << LEVEL) | (1 << TRANSPARENT);

static void
cleanWindowWorkList(GLUTwindow * window)
{
  GLUTwindow **pEntry = &__glutWindowWorkList;
  GLUTwindow *entry = __glutWindowWorkList;

  /* Tranverse singly-linked window work list look for the
     window. */
  while (entry) {
    if (entry == window) {
      /* Found it; delete it. */
      *pEntry = entry->prevWorkWin;
      return;
    } else {
      pEntry = &entry->prevWorkWin;
      entry = *pEntry;
    }
  }
}

static void
cleanStaleWindowList(GLUTwindow * window)
{
  GLUTstale **pEntry = &__glutStaleWindowList;
  GLUTstale *entry = __glutStaleWindowList;

  /* Tranverse singly-linked stale window list look for the
     window ID. */
  while (entry) {
    if (entry->window == window) {
      /* Found it; delete it. */
      *pEntry = entry->next;
      free(entry);
      return;
    } else {
      pEntry = &entry->next;
      entry = *pEntry;
    }
  }
}

static GLUTwindow *__glutWindowCache = NULL;

GLUTwindow *
__glutGetWindow(Window win)
{
  GLUTstale *entry;
  int i;

  /* Does win belong to the last window ID looked up? */
  if (__glutWindowCache && (win == __glutWindowCache->win ||
      (__glutWindowCache->overlay && win ==
        __glutWindowCache->overlay->win))) {
    return
      __glutWindowCache;
  }
  /* Otherwise scan the window list looking for the window ID. */
  for (i = 0; i < __glutWindowListSize; i++) {
    if (__glutWindowList[i]) {
      if (win == __glutWindowList[i]->win) {
        __glutWindowCache = __glutWindowList[i];
        return __glutWindowCache;
      }
      if (__glutWindowList[i]->overlay) {
        if (win == __glutWindowList[i]->overlay->win) {
          __glutWindowCache = __glutWindowList[i];
          return __glutWindowCache;
        }
      }
    }
  }
  /* Scan through destroyed overlay window IDs for which no
     DestroyNotify has yet been received. */
  for (entry = __glutStaleWindowList; entry; entry = entry->next) {
    if (entry->win == win)
      return entry->window;
  }
  return NULL;
}

/* CENTRY */
int APIENTRY 
glutGetWindow(void)
{
  if (__glutCurrentWindow) {
    return __glutCurrentWindow->num + 1;
  } else {
    return 0;
  }
}
/* ENDCENTRY */

void
__glutSetWindow(GLUTwindow * window)
{
  /* It is tempting to try to short-circuit the call to
     glXMakeCurrent if we "know" we are going to make current
     to a window we are already current to.  In fact, this
     assumption breaks when GLUT is expected to integrated with
     other OpenGL windowing APIs that also make current to
     OpenGL contexts.  Since glXMakeCurrent short-circuits the
     "already bound" case, GLUT avoids the temptation to do so
     too. */
  __glutCurrentWindow = window;

  glXMakeCurrent(__glutDisplay, __glutCurrentWindow->renderWin,
    __glutCurrentWindow->renderCtx);

  /* We should be careful to force a finish between each
     iteration through the GLUT main loop if indirect OpenGL 
     contexts are in use; indirect contexts tend to have  much
     longer latency because lots of OpenGL extension requests
     can queue up in the X protocol stream.  We accomplish this
     by posting GLUT_FINISH_WORK to be done. */
  if (!__glutCurrentWindow->isDirect)
    __glutPutOnWorkList(__glutCurrentWindow, GLUT_FINISH_WORK);

  /* If debugging is enabled, we'll want to check this window
     for any OpenGL errors every iteration through the GLUT
     main loop.  To accomplish this, we post the
     GLUT_DEBUG_WORK to be done on this window. */
  if (__glutDebug)
    __glutPutOnWorkList(__glutCurrentWindow, GLUT_DEBUG_WORK);
}

/* CENTRY */
void APIENTRY 
glutSetWindow(int win)
{
  GLUTwindow *window;

  if (win < 1 || win > __glutWindowListSize) {
    __glutWarning("glutSetWindow attempted on bogus window.");
    return;
  }
  window = __glutWindowList[win - 1];
  if (!window) {
    __glutWarning("glutSetWindow attempted on bogus window.");
    return;
  }
  __glutSetWindow(window);
}
/* ENDCENTRY */

static int
getUnusedWindowSlot(void)
{
  int i;

  /* Look for allocated, unused slot. */
  for (i = 0; i < __glutWindowListSize; i++) {
    if (!__glutWindowList[i]) {
      return i;
    }
  }
  /* Allocate a new slot. */
  __glutWindowListSize++;
  if (__glutWindowList) {
    __glutWindowList = (GLUTwindow **)
      realloc(__glutWindowList,
      __glutWindowListSize * sizeof(GLUTwindow *));
  } else {
    /* XXX Some realloc's do not correctly perform a malloc
       when asked to perform a realloc on a NULL pointer,
       though the ANSI C library spec requires this. */
    __glutWindowList = (GLUTwindow **)
      malloc(sizeof(GLUTwindow *));
  }
  if (!__glutWindowList)
    __glutFatalError("out of memory.");
  __glutWindowList[__glutWindowListSize - 1] = NULL;
  return __glutWindowListSize - 1;
}

static XVisualInfo *
getVisualInfoCI(unsigned int mode)
{
  static int bufSizeList[] =
  {16, 12, 8, 4, 2, 1, 0};
  XVisualInfo *vi;
  int list[32];
  int i, n = 0;

  list[n++] = GLX_BUFFER_SIZE;
  list[n++] = 1;
  if (GLUT_WIND_IS_DOUBLE(mode)) {
    list[n++] = GLX_DOUBLEBUFFER;
  }
  if (GLUT_WIND_IS_STEREO(mode)) {
    list[n++] = GLX_STEREO;
  }
  if (GLUT_WIND_HAS_DEPTH(mode)) {
    list[n++] = GLX_DEPTH_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_HAS_STENCIL(mode)) {
    list[n++] = GLX_STENCIL_SIZE;
    list[n++] = 1;
  }
  list[n] = (int) None; /* terminate list */

  /* glXChooseVisual specify GLX_BUFFER_SIZE prefers the
     "smallest index buffer of at least the specified size".
     This would be reasonable if GLUT allowed the user to
     specify the required buffe size, but GLUT's display mode
     is too simplistic (easy to use?). GLUT should try to find
     the "largest".  So start with a large buffer size and
     shrink until we find a matching one that exists. */

  for (i = 0; bufSizeList[i]; i++) {
    /* XXX Assumes list[1] is where GLX_BUFFER_SIZE parameter
       is. */
    list[1] = bufSizeList[i];
    vi = glXChooseVisual(__glutDisplay,
      __glutScreen, list);
    if (vi)
      return vi;
  }
  return NULL;
}

static XVisualInfo *
getVisualInfoRGB(unsigned int mode)
{
  int list[32];
  int n = 0;

  /* XXX Would a caching mechanism to minize the calls to
     glXChooseVisual? You'd have to reference count
     XVisualInfo* pointers. */

  list[n++] = GLX_RGBA;
  list[n++] = GLX_RED_SIZE;
  list[n++] = 1;
  list[n++] = GLX_GREEN_SIZE;
  list[n++] = 1;
  list[n++] = GLX_BLUE_SIZE;
  list[n++] = 1;
  if (GLUT_WIND_HAS_ALPHA(mode)) {
    list[n++] = GLX_ALPHA_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_IS_DOUBLE(mode)) {
    list[n++] = GLX_DOUBLEBUFFER;
  }
  if (GLUT_WIND_IS_STEREO(mode)) {
    list[n++] = GLX_STEREO;
  }
  if (GLUT_WIND_HAS_DEPTH(mode)) {
    list[n++] = GLX_DEPTH_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_HAS_STENCIL(mode)) {
    list[n++] = GLX_STENCIL_SIZE;
    list[n++] = 1;
  }
  if (GLUT_WIND_HAS_ACCUM(mode)) {
    list[n++] = GLX_ACCUM_RED_SIZE;
    list[n++] = 1;
    list[n++] = GLX_ACCUM_GREEN_SIZE;
    list[n++] = 1;
    list[n++] = GLX_ACCUM_BLUE_SIZE;
    list[n++] = 1;
    if (GLUT_WIND_HAS_ALPHA(mode)) {
      list[n++] = GLX_ACCUM_ALPHA_SIZE;
      list[n++] = 1;
    }
  }
#if defined(GLX_VERSION_1_1) && defined(GLX_SGIS_multisample)
  if (GLUT_WIND_IS_MULTISAMPLE(mode)) {
    if (!__glutIsSupportedByGLX("GLX_SGIS_multisample"))
      return NULL;
    list[n++] = GLX_SAMPLES_SGIS;
    /* XXX Is 4 a reasonable minimum acceptable number of
       samples? */
    list[n++] = 4;
  }
#endif
  list[n] = (int) None; /* terminate list */

  return glXChooseVisual(__glutDisplay,
    __glutScreen, list);
}

XVisualInfo *
__glutGetVisualInfo(unsigned int mode)
{
  /* XXX GLUT_LUMINANCE not implemented for GLUT 3.0. */
  if (GLUT_WIND_IS_LUMINANCE(mode))
    return NULL;

  if (GLUT_WIND_IS_RGB(mode))
    return getVisualInfoRGB(mode);
  else
    return getVisualInfoCI(mode);
}

XVisualInfo *
__glutDetermineVisual(
  unsigned int displayMode,
  Bool * treatAsSingle,
  XVisualInfo * (getVisualInfo) (unsigned int))
{
  XVisualInfo *vis;

  *treatAsSingle = GLUT_WIND_IS_SINGLE(displayMode);
  vis = getVisualInfo(displayMode);
  if (!vis) {
    /* Fallback cases when can't get exactly what was asked
       for... */
    if (GLUT_WIND_IS_SINGLE(displayMode)) {
      /* If we can't find a single buffered visual, try looking
         for a double buffered visual.  We can treat a double
         buffered visual as a single buffer visual by changing
         the draw buffer to GL_FRONT and treating any swap
         buffers as no-ops. */
      displayMode |= GLUT_DOUBLE;
      vis = getVisualInfo(displayMode);
      *treatAsSingle = True;
    }
    if (!vis && GLUT_WIND_IS_MULTISAMPLE(displayMode)) {
      /* If we can't seem to get multisampling (ie, not Reality
         Engine class graphics!), go without multisampling.  It
         is up to the application to query how many multisamples
         were allocated (0 equals no multisampling) if the
         application is going to use multisampling for more than
         just antialiasing. */
      displayMode &= ~GLUT_MULTISAMPLE;
      vis = getVisualInfo(displayMode);
    }
  }
  return vis;
}

void
__glutSetupColormap(XVisualInfo * vi, GLUTcolormap ** colormap, Colormap * cmap, int isRGB)
{
#if defined(WIN32)
  if (vi->dwFlags & PFD_NEED_PALETTE || vi->iPixelType==PFD_TYPE_COLORINDEX) {
    *colormap = __glutAssociateColormap(vi);
    *cmap = (*colormap)->cmap;
  } else {
    *colormap = NULL;
    *cmap = 0;
  }
#else
  Status status;
  XStandardColormap *standardCmaps;
  int i, numCmaps;
  static Atom hpColorRecoveryAtom = -1;

  switch (vi->class) {
  case PseudoColor:
    if (isRGB) {
      /* Mesa might return a PseudoColor visual for RGB mode. */
      *colormap = NULL;
      if (MaxCmapsOfScreen(DefaultScreenOfDisplay(__glutDisplay)) == 1
        && vi->visual == DefaultVisual(__glutDisplay, __glutScreen)) {
        char *private = getenv("MESA_PRIVATE_CMAP");

        if (private) {
          /* User doesn't want to share colormaps. */
          *cmap = XCreateColormap(__glutDisplay, __glutRoot,
            vi->visual, AllocNone);
        } else {
          /* Share the root colormap. */
          *cmap = DefaultColormap(__glutDisplay, __glutScreen);
        }
      } else {
        /* Get our own PseudoColor colormap. */
        *cmap = XCreateColormap(__glutDisplay, __glutRoot,
          vi->visual, AllocNone);
      }
    } else {
      /* CI mode, real GLX never returns a PseudoColor visual
         for RGB mode. */
      *colormap = __glutAssociateColormap(vi);
      *cmap = (*colormap)->cmap;
    }
    break;
  case TrueColor:
  case DirectColor:
    *colormap = NULL;   /* NULL if RGBA */

    /* Hewlett-Packard supports a feature called "HP Color
       Recovery". Mesa has code to use HP Color Recovery.  For
       Mesa to use this feature, the atom
       _HP_RGB_SMOOTH_MAP_LIST must be defined on the root
       window AND the colormap obtainable by XGetRGBColormaps
       for that atom must be set on the window.  If that
       colormap is not set, the output will look stripy. */

    if (hpColorRecoveryAtom == -1) {
      char *xvendor;

#define VENDOR_HP "Hewlett-Packard"

      /* Only makes sense to make XInternAtom round-trip if we
         know that we are connected to an HP X server. */
      xvendor = ServerVendor(__glutDisplay);
      if (!strncmp(xvendor, VENDOR_HP, sizeof(VENDOR_HP) - 1)) {
        hpColorRecoveryAtom = XInternAtom(__glutDisplay, "_HP_RGB_SMOOTH_MAP_LIST", True);
      } else {
        hpColorRecoveryAtom = None;
      }
    }
    if (hpColorRecoveryAtom != None) {
      status = XGetRGBColormaps(__glutDisplay, __glutRoot,
        &standardCmaps, &numCmaps, hpColorRecoveryAtom);
      if (status == 1) {
        for (i = 0; i < numCmaps; i++) {
          if (standardCmaps[i].visualid == vi->visualid) {
            *cmap = standardCmaps[i].colormap;
            XFree(standardCmaps);
            return;
          }
        }
        XFree(standardCmaps);
      }
    }
#ifndef SOLARIS_2_4_BUG
    /* Solaris 2.4 has a bug in its XmuLookupStandardColormap
       implementation.  Please compile your Solaris 2.4 version
       of GLUT with -DSOLARIS_2_4_BUG to work around this bug.
       The symptom of the bug is that programs will get a
       BadMatch error from X_CreateWindow when creating a GLUT
       window because Solaris 2.4 creates a  corrupted
       RGB_DEFAULT_MAP property.  Note that this workaround
       prevents Colormap sharing between applications, perhaps
       leading unnecessary colormap installations or colormap
       flashing. */
    status = XmuLookupStandardColormap(__glutDisplay,
      vi->screen, vi->visualid, vi->depth, XA_RGB_DEFAULT_MAP,
      /* replace */ False, /* retain */ True);
    if (status == 1) {
      status = XGetRGBColormaps(__glutDisplay, __glutRoot,
        &standardCmaps, &numCmaps, XA_RGB_DEFAULT_MAP);
      if (status == 1) {
        for (i = 0; i < numCmaps; i++) {
          if (standardCmaps[i].visualid == vi->visualid) {
            *cmap = standardCmaps[i].colormap;
            XFree(standardCmaps);
            return;
          }
        }
        XFree(standardCmaps);
      }
    }
#endif
    /* If no standard colormap but TrueColor, just make a
       private one. */
    /* XXX Should do a better job of internal sharing for
       privately allocated TrueColor colormaps. */
    /* XXX DirectColor probably needs ramps hand initialized! */
    *cmap = XCreateColormap(__glutDisplay, __glutRoot,
      vi->visual, AllocNone);
    break;
  case StaticColor:
  case StaticGray:
  case GrayScale:
    /* Mesa supports these visuals */
    *colormap = NULL;
    *cmap = XCreateColormap(__glutDisplay, __glutRoot,
      vi->visual, AllocNone);
    break;
  default:
    __glutFatalError(
      "could not allocate colormap for visual type: %d.",
      vi->class);
  }
  return;
#endif
}

void
__glutDefaultDisplay(void)
{
  /* XXX Remove the warning after GLUT 3.0. */
  __glutWarning("The following is a new check for GLUT 3.0; update your code.");
  __glutFatalError(
    "redisplay needed for window %d, but no display callback.",
    __glutCurrentWindow->num + 1);
}

void
__glutDefaultReshape(int width, int height)
{
  GLUToverlay *overlay;

  /* Adjust the viewport of the window (and overlay if one
     exists). */
  glXMakeCurrent(__glutDisplay, __glutCurrentWindow->win,
    __glutCurrentWindow->ctx);
  glViewport(0, 0, (GLsizei) width, (GLsizei) height);
  overlay = __glutCurrentWindow->overlay;
  if (overlay) {
    glXMakeCurrent(__glutDisplay, overlay->win, overlay->ctx);
    glViewport(0, 0, (GLsizei) width, (GLsizei) height);
  }
  /* Make sure we are current to the current layer (application
     should be able to count on the current layer not changing
     unless the application explicitly calls glutUseLayer). */
  glXMakeCurrent(__glutDisplay, __glutCurrentWindow->renderWin,
    __glutCurrentWindow->renderCtx);
}

XVisualInfo *
__glutDetermineWindowVisual(Bool * treatAsSingle, Bool * visAlloced)
{
  if (__glutDisplayString) {

    /* __glutDisplayString should be NULL except if
       glutInitDisplayString has been called to register a
       different display string.  Calling glutInitDisplayString
       means using a string instead of an integer mask determine 
       the visual to use. Using the function pointer variable
       __glutDetermineVisualFromString below avoids linking in
       the code for implementing glutInitDisplayString (ie,
       glut_dstr.o) unless glutInitDisplayString gets called by
       the application. */

    assert(__glutDetermineVisualFromString);
    *visAlloced = False;
    return __glutDetermineVisualFromString(__glutDisplayString, treatAsSingle,
      requiredWindowCriteria, numRequiredWindowCriteria, requiredWindowCriteriaMask);
  } else {
    *visAlloced = True;
    return __glutDetermineVisual(__glutDisplayMode,
      treatAsSingle, __glutGetVisualInfo);
  }
}

GLUTwindow *
__glutCreateWindow(GLUTwindow * parent,
  int x, int y, int width, int height)
{
  GLUTwindow *window;
  XSetWindowAttributes wa;
  unsigned long attribMask;
  int winnum;
  int i;

#if defined(WIN32)
  WNDCLASS wc;
  if (!GetClassInfo(GetModuleHandle(NULL), "GLUT", &wc))
    __glutOpenWin32Connection(NULL);
#else
  if (!__glutDisplay)
    __glutOpenXConnection(NULL);
#endif
  winnum = getUnusedWindowSlot();
  window = (GLUTwindow *) malloc(sizeof(GLUTwindow));
  if (!window)
    __glutFatalError("out of memory.");
  window->num = winnum;

#if !defined(WIN32)
  window->vis = __glutDetermineWindowVisual(&window->treatAsSingle,
    &window->visAlloced);
  if (!window->vis) {
    __glutFatalError(
      "visual with necessary capabilities not found.");
  }
  __glutSetupColormap(window->vis, &window->colormap, &window->cmap,
    GLUT_WIND_IS_RGB(__glutDisplayMode));
#endif
  window->eventMask = StructureNotifyMask | ExposureMask;

  attribMask = CWBackPixmap | CWBorderPixel | CWColormap | CWEventMask;
  wa.background_pixmap = None;
  wa.border_pixel = 0;
  wa.colormap = window->cmap;
  wa.event_mask = window->eventMask;
  if (parent) {
    if (parent->eventMask & GLUT_HACK_STOP_PROPAGATE_MASK)
      wa.event_mask |= GLUT_HACK_STOP_PROPAGATE_MASK;
    attribMask |= CWDontPropagate;
    wa.do_not_propagate_mask = parent->eventMask & GLUT_DONT_PROPAGATE_FILTER_MASK;
  } else {
    wa.do_not_propagate_mask = 0;
  }

  /* Stash width and height before Win32's __glutAdjustCoords possibly
     overwrites the values. */
  window->width = width;
  window->height = height;
  window->forceReshape = True;

#if defined(WIN32)
  __glutAdjustCoords(parent ? parent->win : NULL, &x, &y, &width, &height);
  window->win = XCreateWindow(__glutDisplay,
    parent == NULL ? __glutRoot : parent->win,
    x, y, width, height, 0,
    0, InputOutput, 0,
    attribMask, &wa);
  window->hdc = GetDC(window->win);
  /* must set the XHDC for fake glXChooseVisual & fake
     glXCreateContext & fake XAllocColorCells. */
  XHDC = window->hdc;
  window->vis = __glutDetermineWindowVisual(&window->treatAsSingle,
    &window->visAlloced);
  if (!window->vis) {
    __glutFatalError(
      "visual with necessary capabilities not found.");
  }
  if (__glutDebug)
    glXPrintPixelFormat(ChoosePixelFormat(window->hdc, window->vis), 
			window->vis);
  if (!SetPixelFormat(window->hdc,
		      ChoosePixelFormat(window->hdc, window->vis),
		      window->vis))
    __glutFatalError("SetPixelFormat() failed in glutCreateWindow().");
  __glutSetupColormap(window->vis, &window->colormap, &window->cmap,
		      GLUT_WIND_IS_RGB(__glutDisplayMode));
  /* make sure subwindows get a windowStatus callback */
  if (parent)
    PostMessage(parent->win, WM_ACTIVATE, 0, 0);
#else
  window->win = XCreateWindow(__glutDisplay,
    parent == NULL ? __glutRoot : parent->win,
    x, y, width, height, 0,
    window->vis->depth, InputOutput, window->vis->visual,
    attribMask, &wa);
#endif
  window->renderWin = window->win;
  window->ctx = glXCreateContext(__glutDisplay, window->vis,
    None, __glutTryDirect);
  window->renderCtx = window->ctx;
  window->isDirect = glXIsDirect(__glutDisplay, window->ctx);
  if (__glutForceDirect) {
    if (!window->isDirect)
      __glutFatalError("direct rendering not possible.");
  }

  window->parent = parent;
  if (parent) {
    window->siblings = parent->children;
    parent->children = window;
  } else {
    window->siblings = NULL;
  }
  window->overlay = NULL;
  window->children = NULL;
  window->display = __glutDefaultDisplay;
  window->reshape = __glutDefaultReshape;
  window->mouse = NULL;
  window->motion = NULL;
  window->windowStatus = NULL;
  window->visibility = NULL;
  window->passive = NULL;
  window->entry = NULL;
  window->special = NULL;
  window->buttonBox = NULL;
  window->dials = NULL;
  window->spaceMotion = NULL;
  window->spaceRotate = NULL;
  window->spaceButton = NULL;
  window->tabletMotion = NULL;
  window->tabletButton = NULL;
  window->tabletPos[0] = -1;
  window->tabletPos[1] = -1;
  window->keyboard = NULL;
  window->shownState = 0;
  window->visState = -1;  /* not VisibilityUnobscured,
                             VisibilityPartiallyObscured, or
                             VisibilityFullyObscured */
  window->entryState = -1;  /* not EnterNotify or LeaveNotify */
  window->workMask = GLUT_MAP_WORK;
  window->desiredMapState = NormalState;
  window->desiredConfMask = 0;
  window->buttonUses = 0;
  window->cursor = GLUT_CURSOR_INHERIT;
  window->prevWorkWin = __glutWindowWorkList;
  __glutWindowWorkList = window;
  for (i = 0; i < GLUT_MAX_MENUS; i++) {
    window->menu[i] = 0;
  }
  __glutWindowList[winnum] = window;
  __glutSetWindow(window);

  __glutDetermineMesaSwapHackSupport();

  if (window->treatAsSingle) {
    /* We do this because either the window really is single
       buffered (in which case this is redundant, but harmless,
       because this is the initial single-buffered context
       state); or we are treating a double buffered window as a
       single-buffered window because the system does not appear
       to export any suitable single- buffered visuals (in which
       the following are necessary). */
    glDrawBuffer(GL_FRONT);
    glReadBuffer(GL_FRONT);
  }
  return window;
}

static int
findColormaps(GLUTwindow * window,
  Window * winlist, Colormap * cmaplist, int num, int max)
{
  GLUTwindow *child;
  int i;

  /* Do not allow more entries that maximum number of
     colormaps! */
  if (num >= max)
    return num;
  /* Is cmap for this window already on the list? */
  for (i = 0; i < num; i++) {
    if (cmaplist[i] == window->cmap)
      goto normalColormapAlreadyListed;
  }
  /* Not found on the list; add colormap and window. */
  winlist[num] = window->win;
  cmaplist[num] = window->cmap;
  num++;

normalColormapAlreadyListed:

  /* Repeat above but for the overlay colormap if there one. */
  if (window->overlay) {
    if (num >= max)
      return num;
    for (i = 0; i < num; i++) {
      if (cmaplist[i] == window->overlay->cmap)
        goto overlayColormapAlreadyListed;
    }
    winlist[num] = window->overlay->win;
    cmaplist[num] = window->overlay->cmap;
    num++;
  }
overlayColormapAlreadyListed:

  /* Recursively search children. */
  child = window->children;
  while (child) {
    num = findColormaps(child, winlist, cmaplist, num, max);
    child = child->siblings;
  }
  return num;
}

void
__glutEstablishColormapsProperty(GLUTwindow * window)
{
#if !defined(WIN32)
  /* this routine is strictly X.  Win32 doesn't need to do anything of
     this sort (but has to do other wacky stuff later). */
  static Atom wmColormapWindows = None;
  Window *winlist;
  Colormap *cmaplist;
  Status status;
  int maxcmaps, num;

  assert(!window->parent);
  maxcmaps = MaxCmapsOfScreen(ScreenOfDisplay(__glutDisplay,
      __glutScreen));
  /* For portability reasons we don't use alloca for winlist
     and cmaplist, but we could. */
  winlist = (Window *) malloc(maxcmaps * sizeof(Window));
  cmaplist = (Colormap *) malloc(maxcmaps * sizeof(Colormap));
  num = findColormaps(window, winlist, cmaplist, 0, maxcmaps);
  if (num < 2) {
    /* Property no longer needed; remove it. */
    wmColormapWindows = XSGIFastInternAtom(__glutDisplay,
      "WM_COLORMAP_WINDOWS", SGI_XA_WM_COLORMAP_WINDOWS, False);
    if (wmColormapWindows == None) {
      __glutWarning("Could not intern X atom for WM_COLORMAP_WINDOWS.");
      return;
    }
    XDeleteProperty(__glutDisplay, window->win, wmColormapWindows);
  } else {
    status = XSetWMColormapWindows(__glutDisplay, window->win,
      winlist, num);
    /* XSetWMColormapWindows should always work unless the
       WM_COLORMAP_WINDOWS property cannot be intern'ed.  We
       check to be safe. */
    if (status == False)
      __glutFatalError("XSetWMColormapWindows returned False.");
  }
  /* For portability reasons we don't use alloca for winlist
     and cmaplist, but we could. */
  free(winlist);
  free(cmaplist);
#endif
}

GLUTwindow *
__glutToplevelOf(GLUTwindow * window)
{
  while (window->parent) {
    window = window->parent;
  }
  return window;
}

/* CENTRY */
int APIENTRY 
glutCreateWindow(const char *title)
{
  static int firstWindow = 1;
  GLUTwindow *window;
#if !defined(WIN32)
  XWMHints *wmHints;
#endif
  Window win;
  XTextProperty textprop;

  window = __glutCreateWindow(NULL,
    __glutSizeHints.x, __glutSizeHints.y,
    __glutInitWidth, __glutInitHeight);
  win = window->win;
  /* Setup ICCCM properties. */
  textprop.value = (unsigned char *) title;
  textprop.encoding = XA_STRING;
  textprop.format = 8;
  textprop.nitems = strlen(title);
#if defined(WIN32)
  SetWindowText(win, title);
  if (__glutIconic)
    ShowWindow(win, SW_MINIMIZE);
#else
  wmHints = XAllocWMHints();
  wmHints->initial_state =
    __glutIconic ? IconicState : NormalState;
  wmHints->flags = StateHint;
  XSetWMProperties(__glutDisplay, win, &textprop, &textprop,
  /* Only put WM_COMMAND property on first window. */
    firstWindow ? __glutArgv : NULL,
    firstWindow ? __glutArgc : 0,
    &__glutSizeHints, wmHints, NULL);
  XFree(wmHints);
  XSetWMProtocols(__glutDisplay, win, &__glutWMDeleteWindow, 1);
#endif
  firstWindow = 0;
  return window->num + 1;
}

int APIENTRY 
glutCreateSubWindow(int win, int x, int y, int width, int height)
{
  GLUTwindow *window, *toplevel;

  window = __glutCreateWindow(__glutWindowList[win - 1],
    x, y, width, height);
  toplevel = __glutToplevelOf(window);
  if (toplevel->cmap != window->cmap) {
    __glutPutOnWorkList(toplevel, GLUT_COLORMAP_WORK);
  }
  return window->num + 1;
}
/* ENDCENTRY */

void
__glutDestroyWindow(GLUTwindow * window,
  GLUTwindow * initialWindow)
{
  GLUTwindow **prev, *cur, *parent, *siblings;

  /* Recursively destroy any children. */
  cur = window->children;
  while (cur) {
    siblings = cur->siblings;
    __glutDestroyWindow(cur, initialWindow);
    cur = siblings;
  }
  /* Remove from parent's children list (only necessary for
     non-initial windows and subwindows!). */
  parent = window->parent;
  if (parent && parent == initialWindow->parent) {
    prev = &parent->children;
    cur = parent->children;
    while (cur) {
      if (cur == window) {
        *prev = cur->siblings;
        break;
      }
      prev = &(cur->siblings);
      cur = cur->siblings;
    }
  }
  /* Unbind if bound to this window. */
  if (window == __glutCurrentWindow) {
    glXMakeCurrent(__glutDisplay, None, NULL);
    __glutCurrentWindow = NULL;
  }
  /* Begin tearing down window itself. */
  if (window->overlay) {
    __glutFreeOverlayFunc(window->overlay);
  }
  XDestroyWindow(__glutDisplay, window->win);
  glXDestroyContext(__glutDisplay, window->ctx);
  if (window->colormap) {
    /* Only color index windows have colormap data structure. */
    __glutFreeColormap(window->colormap);
  }
  /* NULLing the __glutWindowList helps detect is a window
     instance has been destroyed, given a window number. */
  __glutWindowList[window->num] = NULL;

  /* Cleanup data structures that might contain window. */
  cleanWindowWorkList(window);
  cleanStaleWindowList(window);
  /* Remove window from the "get window cache" if it is there. */
  if (__glutWindowCache == window)
    __glutWindowCache = NULL;

  if (window->visAlloced) {
    /* Only free XVisualInfo* gotten from glXChooseVisual. */
    XFree(window->vis);
  }
  free(window);
}

/* CENTRY */
void APIENTRY 
glutDestroyWindow(int win)
{
  GLUTwindow *window = __glutWindowList[win - 1];

  if (__glutMappedMenu && __glutMenuWindow == window) {
    __glutFatalUsage("destroying menu window not allowed while menus in use");
  }
  /* if not a toplevel window... */
  if (window->parent) {
    /* Destroying subwindows may change colormap requirements;
       recalculate toplevel window's WM_COLORMAP_WINDOWS
       property. */
    __glutPutOnWorkList(__glutToplevelOf(window->parent),
      GLUT_COLORMAP_WORK);
  }
  __glutDestroyWindow(window, window);
}

void APIENTRY 
glutSwapBuffers(void)
{
  GLUTwindow *window = __glutCurrentWindow;

  if (window->renderWin == window->win) {
    if (__glutCurrentWindow->treatAsSingle) {
      /* Pretend the double buffered window is single buffered,
         so treat glutSwapBuffers as a no-op. */
      return;
    }
  } else {
    if (__glutCurrentWindow->overlay->treatAsSingle) {
      /* Pretend the double buffered overlay is single
         buffered, so treat glutSwapBuffers as a no-op. */
      return;
    }
  }

  /* For the MESA_SWAP_HACK. */
  window->usedSwapBuffers = 1;

  glXSwapBuffers(__glutDisplay, __glutCurrentWindow->renderWin);

  /* I considered putting the window being swapped on the
     GLUT_FINISH_WORK work list because you could call
     glutSwapBuffers from an idle callback which doesn't call
     __glutSetWindow which normally adds indirect rendering
     windows to the GLUT_FINISH_WORK work list.  Not being put
     on the list could lead to the buffering up of multiple
     redisplays and buffer swaps and hamper interactivity.  I
     consider this an application bug due to not using
     glutPostRedisplay to trigger redraws.  If
     glutPostRedisplay were used, __glutSetWindow would be
     called and a glFinish to throttle buffering would occur. */
}
/* ENDCENTRY */

void
__glutChangeWindowEventMask(long eventMask, Bool add)
{
  if (add) {
    /* Add eventMask to window's event mask. */
    if ((__glutCurrentWindow->eventMask & eventMask) !=
      eventMask) {
      __glutCurrentWindow->eventMask |= eventMask;
      __glutPutOnWorkList(__glutCurrentWindow,
        GLUT_EVENT_MASK_WORK);
    }
  } else {
    /* Remove eventMask from window's event mask. */
    if (__glutCurrentWindow->eventMask & eventMask) {
      __glutCurrentWindow->eventMask &= ~eventMask;
      __glutPutOnWorkList(__glutCurrentWindow,
        GLUT_EVENT_MASK_WORK);
    }
  }
}

void APIENTRY 
glutDisplayFunc(GLUTdisplayCB displayFunc)
{
  /* XXX Remove the warning after GLUT 3.0. */
  if (!displayFunc)
    __glutFatalError("NULL display callback not allowed in GLUT 3.0; update your code.");
  __glutCurrentWindow->display = displayFunc;
}

void APIENTRY 
glutKeyboardFunc(GLUTkeyboardCB keyboardFunc)
{
  __glutChangeWindowEventMask(KeyPressMask,
    keyboardFunc != NULL || __glutCurrentWindow->special != NULL);
  __glutCurrentWindow->keyboard = keyboardFunc;
}

void APIENTRY 
glutSpecialFunc(GLUTspecialCB specialFunc)
{
  __glutChangeWindowEventMask(KeyPressMask,
    specialFunc != NULL || __glutCurrentWindow->keyboard != NULL);
  __glutCurrentWindow->special = specialFunc;
}

void APIENTRY 
glutMouseFunc(GLUTmouseCB mouseFunc)
{
  if (__glutCurrentWindow->mouse) {
    if (!mouseFunc) {
      /* Previous mouseFunc being disabled. */
      __glutCurrentWindow->buttonUses--;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask,
        __glutCurrentWindow->buttonUses > 0);
    }
  } else {
    if (mouseFunc) {
      /* Previously no mouseFunc, new one being installed. */
      __glutCurrentWindow->buttonUses++;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask, True);
    }
  }
  __glutCurrentWindow->mouse = mouseFunc;
}

void APIENTRY 
glutMotionFunc(GLUTmotionCB motionFunc)
{
  /* Hack.  Some window managers (4Dwm by default) will mask
     motion events if the client is not selecting for button
     press and release events. So we select for press and
     release events too (being careful to use reference
     counting).  */
  if (__glutCurrentWindow->motion) {
    if (!motionFunc) {
      /* previous mouseFunc being disabled */
      __glutCurrentWindow->buttonUses--;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask,
        __glutCurrentWindow->buttonUses > 0);
    }
  } else {
    if (motionFunc) {
      /* Previously no mouseFunc, new one being installed. */
      __glutCurrentWindow->buttonUses++;
      __glutChangeWindowEventMask(
        ButtonPressMask | ButtonReleaseMask, True);
    }
  }
  /* Real work of selecting for passive mouse motion.  */
  __glutChangeWindowEventMask(
    Button1MotionMask | Button2MotionMask | Button3MotionMask,
    motionFunc != NULL);
  __glutCurrentWindow->motion = motionFunc;
}

void APIENTRY 
glutPassiveMotionFunc(GLUTpassiveCB passiveMotionFunc)
{
  __glutChangeWindowEventMask(PointerMotionMask,
    passiveMotionFunc != NULL);

  /* Passive motion also requires watching enters and leaves so
     that a fake passive motion event can be generated on an
     enter. */
  __glutChangeWindowEventMask(EnterWindowMask | LeaveWindowMask,
    __glutCurrentWindow->entry != NULL || passiveMotionFunc != NULL);

  __glutCurrentWindow->passive = passiveMotionFunc;
}

void APIENTRY 
glutEntryFunc(GLUTentryCB entryFunc)
{
  __glutChangeWindowEventMask(EnterWindowMask | LeaveWindowMask,
    entryFunc != NULL || __glutCurrentWindow->passive);
  __glutCurrentWindow->entry = entryFunc;
  if (!entryFunc) {
    __glutCurrentWindow->entryState = -1;
  }
}

void APIENTRY 
glutWindowStatusFunc(GLUTwindowStatusCB windowStatusFunc)
{
  __glutChangeWindowEventMask(VisibilityChangeMask,
    windowStatusFunc != NULL);
  __glutCurrentWindow->windowStatus = windowStatusFunc;
  if (!windowStatusFunc) {
    /* Make state invalid. */
    __glutCurrentWindow->visState = -1;
  }
}

static void
visibilityHelper(int status)
{
  if (status == GLUT_HIDDEN || status == GLUT_FULLY_COVERED)
    __glutCurrentWindow->visibility(GLUT_NOT_VISIBLE);
  else
    __glutCurrentWindow->visibility(GLUT_VISIBLE);
}

void APIENTRY 
glutVisibilityFunc(GLUTvisibilityCB visibilityFunc)
{
  __glutCurrentWindow->visibility = visibilityFunc;
  if (visibilityFunc)
    glutWindowStatusFunc(visibilityHelper);
  else
    glutWindowStatusFunc(NULL);
}

void APIENTRY 
glutReshapeFunc(GLUTreshapeCB reshapeFunc)
{
  if (reshapeFunc) {
    __glutCurrentWindow->reshape = reshapeFunc;
  } else {
    __glutCurrentWindow->reshape = __glutDefaultReshape;
  }
}
