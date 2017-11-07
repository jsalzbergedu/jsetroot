#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <Imlib2.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "hsetroot-config.h"

typedef enum
{ Full, Fill, Center, Tile } ImageMode;

// Globals:
Display *display;
int screen;

// Adapted from fluxbox' bsetroot
int
setRootAtoms (Pixmap pixmap)
{
  Atom atom_root, atom_eroot, type;
  unsigned char *data_root, *data_eroot;
  int format;
  unsigned long length, after;

  atom_root = XInternAtom (display, "_XROOTMAP_ID", True);
  atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", True);

  // doing this to clean up after old background
  if (atom_root != None && atom_eroot != None)
    {
      XGetWindowProperty (display, RootWindow (display, screen),
			  atom_root, 0L, 1L, False, AnyPropertyType,
			  &type, &format, &length, &after, &data_root);
      if (type == XA_PIXMAP)
	{
	  XGetWindowProperty (display, RootWindow (display, screen),
			      atom_eroot, 0L, 1L, False, AnyPropertyType,
			      &type, &format, &length, &after, &data_eroot);
	  if (data_root && data_eroot && type == XA_PIXMAP &&
	      *((Pixmap *) data_root) == *((Pixmap *) data_eroot))
	    {
	      XKillClient (display, *((Pixmap *) data_root));
	    }
	}
    }

  atom_root = XInternAtom (display, "_XROOTPMAP_ID", False);
  atom_eroot = XInternAtom (display, "ESETROOT_PMAP_ID", False);

  if (atom_root == None || atom_eroot == None)
    return 0;

  // setting new background atoms
  XChangeProperty (display, RootWindow (display, screen),
		   atom_root, XA_PIXMAP, 32, PropModeReplace,
		   (unsigned char *) &pixmap, 1);
  XChangeProperty (display, RootWindow (display, screen), atom_eroot,
		   XA_PIXMAP, 32, PropModeReplace, (unsigned char *) &pixmap,
		   1);

  return 1;
}

typedef struct
{
  int r, g, b, a;
} Color, *PColor;

int
getHex (char c)
{
  switch (c)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return c - '0';
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
      return c - 'A' + 10;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
      return c - 'a' + 10;
    default:
      return 0;
    }
}

int
parse_color (char *arg, PColor c, int a)
{
  if (arg[0] != '#')
    return 0;

  if ((strlen (arg) != 7) && (strlen (arg) != 9))
    return 0;

  c->r = getHex (arg[1]) * 16 + getHex (arg[2]);
  c->g = getHex (arg[3]) * 16 + getHex (arg[4]);
  c->b = getHex (arg[5]) * 16 + getHex (arg[6]);
  c->a = a;
  if (strlen (arg) == 9)
    c->a = getHex (arg[7]) * 16 + getHex (arg[8]);

  return 1;
}

int
load_image (ImageMode mode, const char *arg, int rootW, int rootH, int alpha,
	    Imlib_Image rootimg)
{
  int imgW, imgH, o;
  Imlib_Image buffer = imlib_load_image (arg);

  if (!buffer)
    return 0;

  imlib_context_set_image (buffer);
  imgW = imlib_image_get_width (), imgH = imlib_image_get_height ();

  if (alpha < 255)
    {
      // Create alpha-override mask
      imlib_image_set_has_alpha (1);
      Imlib_Color_Modifier modifier = imlib_create_color_modifier ();
      imlib_context_set_color_modifier (modifier);

      DATA8 red[256], green[256], blue[256], alph[256];
      imlib_get_color_modifier_tables (red, green, blue, alph);
      for (o = 0; o < 256; o++)
	alph[o] = (DATA8) alpha;
      imlib_set_color_modifier_tables (red, green, blue, alph);

      imlib_apply_color_modifier ();
      imlib_free_color_modifier ();
    }

  imlib_context_set_image (rootimg);
  if (mode == Fill)
    {
      imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
				    0, 0, rootW, rootH);
    }
  else if (mode == Full)
    {
      double aspect = ((double) rootW) / imgW;
      int top, left;
      if ((int) (imgH * aspect) > rootH)
	aspect = (double) rootH / (double) imgH;
      top = (rootH - (int) (imgH * aspect)) / 2;
      left = (rootW - (int) (imgW * aspect)) / 2;
      imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
				    left, top, (int) (imgW * aspect),
				    (int) (imgH * aspect));
    }
  else
    {
      int left = (rootW - imgW) / 2, top = (rootH - imgH) / 2;

      if (mode == Tile)
	{
	  int x, y;
	  for (; left > 0; left -= imgW);
	  for (; top > 0; top -= imgH);

	  for (x = left; x < rootW; x += imgW)
	    for (y = top; y < rootH; y += imgH)
	      imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
					    x, y, imgW, imgH);
	}
      else
	{
	  imlib_blend_image_onto_image (buffer, 0, 0, 0, imgW, imgH,
					left, top, imgW, imgH);
	}
    }

  imlib_context_set_image (buffer);
  imlib_free_image ();

  imlib_context_set_image (rootimg);

  return 1;
}

void
set_background_image (const char *image_file_name, Display *_display)
{
  Visual *vis;
  Colormap cm;
  // Display *_display;
  Imlib_Context *context;
  Imlib_Image image;
  int width, height, depth, i, alpha;
  Pixmap pixmap;
  Imlib_Color_Modifier modifier = NULL;
  // _display = XOpenDisplay (NULL);

  for (screen = 0; screen < ScreenCount (_display); screen++)
    {
      display = XOpenDisplay (NULL);

      context = imlib_context_new ();
      imlib_context_push (context);

      imlib_context_set_display (display);
      vis = DefaultVisual (display, screen);
      cm = DefaultColormap (display, screen);
      width = DisplayWidth (display, screen);
      height = DisplayHeight (display, screen);
      depth = DefaultDepth (display, screen);

      pixmap =
	XCreatePixmap (display, RootWindow (display, screen), width, height,
		       depth);

      imlib_context_set_visual (vis);
      imlib_context_set_colormap (cm);
      imlib_context_set_drawable (pixmap);
      imlib_context_set_color_range (imlib_create_color_range ());

      image = imlib_create_image (width, height);
      imlib_context_set_image (image);

      imlib_context_set_color (0, 0, 0, 255);
      imlib_image_fill_rectangle (0, 0, width, height);

      imlib_context_set_dither (1);
      imlib_context_set_blend (1);

      alpha = 255;
      if (load_image (Center, image_file_name, width, height, alpha, image) ==
	  0)
	{
	  fprintf (stderr, "Bad image (%s)\n", image_file_name);
	  continue;
	}
      
      if (modifier != NULL)
	{
	  imlib_context_set_color_modifier (modifier);
	  imlib_apply_color_modifier ();
	  imlib_free_color_modifier ();
	  modifier = NULL;
	}

      imlib_render_image_on_drawable (0, 0);
      imlib_free_image ();
      imlib_free_color_range ();

      if (setRootAtoms (pixmap) == 0)
	fprintf (stderr, "Couldn't create atoms...\n");

      XKillClient (display, AllTemporary);
      XSetCloseDownMode (display, RetainTemporary);

      XSetWindowBackgroundPixmap (display, RootWindow (display, screen),
				  pixmap);
      XClearWindow (display, RootWindow (display, screen));

      XFlush (display);
      XSync (display, False);

      imlib_context_pop ();
      imlib_context_free (context);
    }

  return;
}
