#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <stdio.h>
#include <stdbool.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

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

    const char *file_path = "tsodinPog.png";
    int width, height;
    struct {
        uint8_t r, g, b, a;
    } *pixels = (void*)stbi_load(file_path, &width, &height, NULL, 4);
    if (!pixels) {
        fprintf(stderr, "ERROR: could not load %s\n", file_path);
        return 1;
    }
    printf("Loaded %s %dx%d\n", file_path, width, height);
    for (int i = 0; i < width*height; ++i) {
        uint8_t t = pixels[i].r;
        pixels[i].r = pixels[i].b;
        pixels[i].b = t;
    }

    XImage *pog_image = XCreateImage(display, wa.visual, wa.depth, ZPixmap, 0, (void*)pixels, width, height, 32, width*sizeof(*pixels));
    printf("pog_image = %p\n", pog_image);
    Pixmap pog_pixmap = XCreatePixmap(display, window, width, height, 24);
    XPutImage(display, pog_pixmap, gc, pog_image, 0, 0, 0, 0, width, height);

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

    XStoreName(display, window, "XRender Test");

    XMapWindow(display, window);

    XTransform double_scale = {{
        {XDoubleToFixed(0.5), XDoubleToFixed(0), XDoubleToFixed(0)},
        {XDoubleToFixed(0), XDoubleToFixed(0.5), XDoubleToFixed(0)},
        {XDoubleToFixed(0), XDoubleToFixed(0), XDoubleToFixed(1.0)}
    }};
    XRenderSetPictureTransform(display, pog_picture, &double_scale);

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
            }
        }

        // XRenderColor color = { .red = 0xFFFF, .alpha = 0xFFFF };
        // XRenderFillRectangle(display, PictOpSrc, dst, &color, 0, 0, 100, 100);

        XRenderComposite (display, PictOpSrc, pog_picture, None, dst, 0, 0, 0, 0, 0, 0, 2*width, 2*height);
    }

    printf("OK\n");
    return 0;
}
