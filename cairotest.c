#include <bcm_host.h>
#include <cairo.h>
#include <cairo-vg.h>
#include <EGL/egl.h>

int main (int argc, char *argv[]) {
    //cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 240, 80);
    //cairo_t *cr = cairo_create (surface);

    //cairo_select_font_face (cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    //cairo_set_font_size (cr, 32.0);
    //cairo_set_source_rgb (cr, 0.0, 0.0, 1.0);
    //cairo_move_to (cr, 10.0, 50.0);
    //cairo_show_text (cr, "Hello, world");

    //cairo_destroy (cr);
    //cairo_surface_write_to_png (surface, "hello.png");
    //cairo_surface_destroy (surface);

    bcm_host_init();

    // FIXME: This should be replaceable by call to something in cairo-vg-boilerplate?
    EGLDisplay disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    EGLint egl_maj, egl_min;
    eglInitialize(disp, &egl_maj, &egl_min);
    eglBindAPI(EGL_OPENVG_API);

    EGLConfig configs[1];
    EGLint attribs[32];
    int n = 0;

    // vgfont/graphics.c:gx_egl_attrib_colours()
    static const EGLint RGBA[] = {EGL_RED_SIZE, EGL_GREEN_SIZE, EGL_BLUE_SIZE, EGL_ALPHA_SIZE};
    static const EGLint RGB32A[] = {8,8,8,8};
    attribs[n++] = RGBA[0];
    attribs[n++] = RGB32A[0];
    attribs[n++] = RGBA[1];
    attribs[n++] = RGB32A[1];
    attribs[n++] = RGBA[2];
    attribs[n++] = RGB32A[2];
    attribs[n++] = RGBA[3];
    attribs[n++] = RGB32A[3];

    // vgfont/graphics.c:create_context()
    attribs[n++] = EGL_RENDERABLE_TYPE;
    attribs[n++] = EGL_OPENVG_BIT;
    //attribs[n++] = EGL_SURFACE_TYPE;
    //attribs[n++] = EGL_WINDOW_BIT;
    attribs[n] = EGL_NONE;

    EGLint nconfigs;
    EGLBoolean egl_ret = eglChooseConfig(disp, attribs, configs, countof(configs), &nconfigs);

    EGLContext eglcontext = EGL_NO_CONTEXT;
    eglcontext = eglCreateContext(disp, configs[0], eglcontext, 0);

    cairo_vg_context_t *vgcontext = cairo_vg_context_create_for_egl(disp, eglcontext);
    cairo_surface_t *vgsurface = cairo_vg_surface_create(vgcontext, CAIRO_CONTENT_COLOR_ALPHA, 240, 80);
    cairo_t *cr = cairo_create(vgsurface);

    cairo_select_font_face (cr, "serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 32.0);
    cairo_set_source_rgb (cr, 1.0, 0.0, 1.0);
    cairo_move_to (cr, 10.0, 50.0);
    cairo_show_text (cr, "Hello, world");
    sleep(3);

    cairo_destroy (cr);
    cairo_surface_destroy(vgsurface);
    cairo_vg_context_destroy(vgcontext);
    eglDestroyContext(disp, eglcontext);
    eglTerminate(disp);

    return 0;
}
