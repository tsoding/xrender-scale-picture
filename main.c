#include <stdio.h>
#include <stdbool.h>

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "nob.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

double update_scale(int window_width, int window_height, int pog_width, int pog_height, Display *display, Picture pog_picture);

int main()
{
    Display *display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "ERROR: Could not open X display\n");
        return 1;
    }

    int major, minor;
    if (!XRenderQueryVersion(display, &major, &minor)) {
        fprintf(stderr, "ERROR: Xrender is not supported\n");
        return 1;
    }
    printf("Xrender %d.%d is supported\n", major, minor);


    Window window = XCreateSimpleWindow(
                        display,
                        XDefaultRootWindow(display),
                        0, 0,
                        WINDOW_WIDTH, WINDOW_HEIGHT,
                        0,
                        0,
                        0);

    XWindowAttributes wa = {0};
    XGetWindowAttributes(display, window, &wa);

    GC gc = XCreateGC(display, window, 0, NULL);

    const char *pog_file_path = "tsodinPog.png";
    int pog_width, pog_height;
    struct { uint8_t r, g, b, a; } *pog_pixels = (void*)stbi_load(pog_file_path, &pog_width, &pog_height, NULL, 4);
    if (!pog_pixels) {
        fprintf(stderr, "ERROR: could not load %s\n", pog_file_path);
        return 1;
    }
    printf("Loaded %s %dx%d\n", pog_file_path, pog_width, pog_height);
    for (int i = 0; i < pog_width*pog_height; ++i) {
        uint8_t r = pog_pixels[i].r;
        uint8_t b = pog_pixels[i].b;
        pog_pixels[i].r = b;
        pog_pixels[i].b = r;
    }

    XImage *pog_image = XCreateImage(display, wa.visual, wa.depth, ZPixmap, 0, (void*)pog_pixels, pog_width, pog_height, 32, pog_width*sizeof(*pog_pixels));
    printf("pog_image = %p\n", pog_image);
    Pixmap pog_pixmap = XCreatePixmap(display, window, pog_width, pog_height, 24);
    XPutImage(display, pog_pixmap, gc, pog_image, 0, 0, 0, 0, pog_width, pog_height);

    XRenderPictFormat *format = XRenderFindVisualFormat(display, wa.visual);

    Picture pog_picture = XRenderCreatePicture (display, pog_pixmap, format, 0, NULL);
    Picture dst = XRenderCreatePicture (display, window, format, 0, NULL);

    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete_window, 1);

    long event_mask = KeyPressMask
                    | KeyReleaseMask
                    | StructureNotifyMask
                    | PointerMotionMask
                    | ButtonPressMask
                    | ButtonReleaseMask;
    XSelectInput(display, window, event_mask);

    XStoreName(display, window, "XRender Scale Picture");

    XMapWindow(display, window);

    int window_width  = wa.width;
    int window_height = wa.height;
    double scale = update_scale(window_width, window_height, pog_width, pog_height, display, pog_picture);

    bool quit = false;
    while (!quit) {
        while (XPending(display) > 0) {
            XEvent event = {0};
            XNextEvent(display, &event);
            switch (event.type) {
            case ClientMessage: {
                if ((Atom) event.xclient.data.l[0] == wm_delete_window) {
                    quit = true;
                }
            } break;
            case ConfigureNotify: {
                if (window_width != event.xconfigure.width || window_height != event.xconfigure.height) {
                    window_width  = event.xconfigure.width;
                    window_height = event.xconfigure.height;
                    scale         = update_scale(window_width, window_height, pog_width, pog_height, display, pog_picture);
                    XClearWindow(display, window);
                }
            } break;
            }
        }

        // XRenderColor color = { .red = 0xFFFF, .alpha = 0xFFFF };
        // XRenderFillRectangle(display, PictOpSrc, dst, &color, 0, 0, 100, 100);

        int scaled_pog_width  = (1/scale)*pog_width;
        int scaled_pog_height = (1/scale)*pog_height;
        XRenderComposite(display, PictOpSrc, pog_picture, None, dst, 0, 0, 0, 0, window_width/2 - scaled_pog_width/2, window_height/2 - scaled_pog_height/2, scaled_pog_width, scaled_pog_height);
        usleep(16000);
    }

    return 0;
}

double update_scale(int window_width, int window_height, int pog_width, int pog_height, Display *display, Picture pog_picture)
{
    double scale;
    if (window_height < window_width) {
        scale = (double)pog_height/window_height;
    } else {
        scale = (double)pog_width/window_width;
    }
    XTransform transform = {{
        {XDoubleToFixed(scale), XDoubleToFixed(0),     XDoubleToFixed(0)  },
        {XDoubleToFixed(0),     XDoubleToFixed(scale), XDoubleToFixed(0)  },
        {XDoubleToFixed(0),     XDoubleToFixed(0),     XDoubleToFixed(1.0)},
    }};
    XRenderSetPictureTransform(display, pog_picture, &transform);
    return scale;
}
