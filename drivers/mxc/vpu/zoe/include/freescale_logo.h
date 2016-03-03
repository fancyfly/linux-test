#ifndef __FREESCALE_LOGO_H__
#define __FREESCALE_LOGO_H__



typedef struct freescale_picture
{
    unsigned int buffer_id_;
    int *x;
    int *y;
    int w;
    int h;
    int crop_left;
    int crop_right;
    int crop_top;
    int crop_bottom;

    int *delta_x;
    int *delta_y;
    bool           *in_use;
    int            sizer; // 0(full size) or 1(half size)..
    int            semiplanar;
    unsigned char* y_buffer;
    unsigned char* u_buffer;
    unsigned char* v_buffer;
} freescale_picture_t;


freescale_picture_t** freescale_create_pictures(int max_nr_pictures);
void freescale_init_picture(int* p_x, int* p_y, int* p_delta_x, int* p_delta_y, freescale_picture_t** pics, unsigned int buffer_id, int i, int sizer, int semiplanar, int width, int height, unsigned char* ybuf, unsigned char* ubuf, unsigned char* vbuf, bool* in_use_flag);
void freescale_init_pictures(int sizer, int max_nr_pictures, int width, int height, int semiplanar);
void freescale_deinit_picture(freescale_picture_t** pics, unsigned int buffer_id);
void freescale_deinit_pictures();
freescale_picture_t* freescale_next_picture(freescale_picture_t** pics);
void freescale_release_picture(freescale_picture_t*);




#if 0
#include "Freescale.h"
#include <stdio.h>

const int FREESCALE_MAX_PIC = 2;
const int FREESCALE_WIDTH   = 1280;
const int FREESCALE_HEIGHT  = 720;

int main(int argc, char* argv[])
{
    char filename[512];
    sprintf(filename, "D:/zv_root/output/free_%ux%u.yuv", FREESCALE_WIDTH, FREESCALE_HEIGHT);
    FILE* fp         = 0;
    int   semiplanar = 1;
    freescale_init_pictures(0, FREESCALE_MAX_PIC, FREESCALE_WIDTH, FREESCALE_HEIGHT, semiplanar);

    fp = fopen(filename, "wbS");
    if(fp != 0) {
        int n = 0;
        for(n = 0; n < 1000; n++) {
            freescale_picture_t* pic = freescale_next_picture();
            if(pic) {
                fwrite(&(pic->y_buffer[0]), 1, FREESCALE_WIDTH * FREESCALE_HEIGHT, fp);
                if(semiplanar) {
                    fwrite(&(pic->u_buffer[0]), 1, FREESCALE_WIDTH * FREESCALE_HEIGHT / 2, fp);
                }
                else {
                    fwrite(&(pic->u_buffer[0]), 1, FREESCALE_WIDTH * FREESCALE_HEIGHT / 4, fp);
                    fwrite(&(pic->v_buffer[0]), 1, FREESCALE_WIDTH * FREESCALE_HEIGHT / 4, fp);
                }
            }
            freescale_release_picture(pic);
        }
        fclose(fp);
    }

    return 0;
}

#endif

#endif
