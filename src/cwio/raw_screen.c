// Include the header file to get access to the MicroPython API
#include "py/dynruntime.h"

#define INT(v) mp_obj_get_int(v)
#define PY_INT(v) mp_obj_new_int(v)
#define INT_ARG(i) INT(args[i])
#define CALL(fun, n_args, n_kw, args) mp_call_function_n_kw(fun, n_args, n_kw, args)
#define WRITE(addr, data) writeargs[0] = PY_INT(addr);\
    writeargs[1] = PY_INT(data);\
    CALL(write, 2, 0, writeargs);
#define PAINT(x, y, plane, brush, v) paintargs[0] = PY_INT(x);\
    paintargs[1] = PY_INT(y);\
    paintargs[2] = PY_INT(plane);\
    paintargs[3] = PY_INT(brush);\
    paintargs[4] = PY_INT(v);\
    CALL(paint, 5, 0, paintargs);

// helper function
static mp_int_t in_bounds_raw(mp_int_t x, mp_int_t y, mp_int_t w, mp_int_t h) {
    return x >= 0 && x < w && y >= 0 && y < h;
}

// Checks if a x, y vector is in the bounds of a screen of size w, h
static mp_obj_t in_bounds(size_t n_args, const mp_obj_t *args) {
    int x = INT_ARG(0);
    int y = INT_ARG(1);
    int w = INT_ARG(2);
    int h = INT_ARG(3);
    return mp_obj_new_bool(in_bounds_raw(x, y, w, h));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(in_bounds_obj, 4, 4, in_bounds);

// Clears the entire screen from a start to an end
static mp_obj_t clear(mp_obj_t buffer, mp_obj_t start, mp_obj_t end) {
    mp_buffer_info_t buf;
    mp_get_buffer_raise(buffer, &buf, MP_BUFFER_WRITE);
    uint8_t *data = (uint8_t *)buf.buf;
    for (int i = INT(start); i < INT(end); i++) data[i] = 0x00;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(clear_obj, clear);

// Applies the screen buffer to the calculator
static mp_obj_t apply(size_t n_args, const mp_obj_t *args) {
    mp_obj_t writeargs[2];
    mp_obj_t write              = args[0];
    int const_select            = INT_ARG(1);
    int const_buffer            = INT_ARG(2);
    int const_width_b           = INT_ARG(3);
    int const_true_width_b      = INT_ARG(4);
    int const_height            = INT_ARG(5);
    int const_header            = INT_ARG(6);
    mp_obj_t buffer             = args[7];
    mp_obj_t oldbuffer          = args[8];
    int plane                   = INT_ARG(9);
    int start_x                 = INT_ARG(10);
    int start_y                 = INT_ARG(11);
    int end_x                   = INT_ARG(12);
    int end_y                   = INT_ARG(13);
    bool oldbuf_none            = oldbuffer == mp_const_none;
    mp_buffer_info_t bufinfo;
    mp_buffer_info_t oldbufinfo;
    uint8_t *buf;
    uint8_t *oldbuf;
    mp_get_buffer_raise(buffer, &bufinfo, MP_BUFFER_RW);
    buf = (uint8_t *)bufinfo.buf;
    if (!oldbuf_none) {
        mp_get_buffer_raise(oldbuffer, &oldbufinfo, MP_BUFFER_READ);
        oldbuf = (uint8_t *)oldbufinfo.buf;
    }
    
    WRITE(const_select, plane);
    for (int y = 0; y < const_header; y++) {
        for (int x = 0; x < const_width_b; x++) {
            int i = (const_height + y) * const_width_b + x;
            if (oldbuf_none || (oldbuf[i] != buf[i])) {
                WRITE(
                    const_buffer + y * const_true_width_b + x,
                    buf[i]
                );
            }
        }
    }
    for (int y = start_y; y <= end_y; y++) {
        for (int x = start_x; x < end_x; x++) {
            int i = y * const_width_b + x;
            if (oldbuf_none || (oldbuf[i] != buf[i])) {
                WRITE(
                    const_buffer + (y+1) * const_true_width_b + x,
                    buf[i]
                );
            }
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(apply_obj, 14, 14, apply);

static mp_obj_t image(size_t n_args, const mp_obj_t *args) {
    mp_obj_t paintargs[5];
    int x               = INT_ARG(0);
    int y               = INT_ARG(1);
    int w               = INT_ARG(2);
    int h               = INT_ARG(3);
    int v               = INT_ARG(4);
    int const_bright    = INT_ARG(5);
    int const_dark      = INT_ARG(6);
    int const_width     = INT_ARG(7);
    int const_height    = INT_ARG(8);
    mp_obj_t paint      = args[9];
    mp_obj_t image      = args[10];
    mp_buffer_info_t imageinfo;
    mp_get_buffer_raise(image, &imageinfo, MP_BUFFER_READ);
    uint8_t *img = (uint8_t *)imageinfo.buf;
    bool draw = false;
    // start of previous x byte
    // int xb = x / 8; // unused
    // actual row size in bytes
    int wb = ((w+7)&~7)/8;
    for (int py = 0; py < h; py++) {
        if (!draw) draw = in_bounds_raw(0, y+py, const_width, const_height);
        if (draw) {
            if (!in_bounds_raw(0, y+py, const_width, const_height)) return mp_const_none;
            for (int bx = 0; bx < wb; bx++) {
                // x offset from last byte
                int ox = x % 8;
                // current byte's address in image data
                int r = py * wb + bx;
                // previous, current and next bytes in row
                int prevb = (bx == 0) ? 0x00 : img[r-1];
                int currb = img[r];
                int nextb = (bx == wb-1) ? 0x00 : img[r+1];
                // generate brush by merging all 3 bytes next to each other
                int brush = (prevb << 16) | (currb << 8) | nextb;
                // get the byte that gets drawn to the screen
                // by offsetting the brush by the x offset
                int byte = brush >> (8 + ox);
                // draw the byte on the screen if it contains data
                if (byte) {
                    PAINT(x + bx * 8, y + py, 0, byte, v & const_bright);
                    PAINT(x + bx * 8, y + py, 1, byte, v & const_dark);
                }
                // draw the remaining part if it's being cut off by the byte border
                byte = brush >> ox;
                if (byte) {
                    PAINT(x + (bx+1) * 8, y + py, 0, byte, v & const_bright);
                    PAINT(x + (bx+1) * 8, y + py, 1, byte, v & const_dark);
                }
            }
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(image_obj, 11, 11, image);

// This is the entry point and is called when the module is imported
mp_obj_t mpy_init(mp_obj_fun_bc_t *self, size_t n_args, size_t n_kw, mp_obj_t *args) {
    // This must be first, it sets up the globals dict and other things
    MP_DYNRUNTIME_INIT_ENTRY

    // Make the functions available in the module's namespace
    mp_store_global(MP_QSTR_in_bounds   , MP_OBJ_FROM_PTR(&in_bounds_obj));
    mp_store_global(MP_QSTR_clear       , MP_OBJ_FROM_PTR(&clear_obj    ));
    mp_store_global(MP_QSTR_apply       , MP_OBJ_FROM_PTR(&apply_obj    ));
    mp_store_global(MP_QSTR_image       , MP_OBJ_FROM_PTR(&image_obj    ));

    // This must be last, it restores the globals dict
    MP_DYNRUNTIME_INIT_EXIT
}