#include "shadow_rga.h"
#include "../drmcon/drm_display.h"

static bo_t g_bo;
static int g_src_fd;
static int g_dst_fd;

void *shadow_rga_g_bo_ptr()
{
    return g_bo.ptr;
}

#define DIVIDE_SUM(a, b) (((a)/(b)) + ((a)%(b)))

int shadow_rga_init(int size)
{
    int ret;
    struct bo bo;
    int width, height, bpp;

    getdrmdispinfo(&bo, &width, &height);
    getdrmdispbpp(&bpp);

    ret = c_RkRgaInit();
    if (ret) {
        printf("c_RkRgaInit error : %s\n", strerror(errno));
        return ret;
    }
    height = DIVIDE_SUM(DIVIDE_SUM(size, bpp), width) * 8;
    ret = c_RkRgaGetAllocBuffer(&g_bo, width, height, bpp);
    if (ret) {
        printf("c_RkRgaGetAllocBuffer error : %s\n", strerror(errno));
        return ret;
    }
    printf("size = %d, g_bo.size = %d\n", size, g_bo.size);
    ret = c_RkRgaGetMmap(&g_bo);
    if (ret) {
        printf("c_RkRgaGetMmap error : %s\n", strerror(errno));
        return ret;
    }
    ret = c_RkRgaGetBufferFd(&g_bo, &g_src_fd);
    if (ret) {
        printf("c_RkRgaGetBufferFd error : %s\n", strerror(errno));
        return ret;
    }

    return 0;
}

void shadow_rga_exit(void)
{
    int ret;
    close(g_src_fd);
    ret = c_RkRgaUnmap(&g_bo);
    if (ret)
        printf("c_RkRgaUnmap error : %s\n", strerror(errno));
    ret = c_RkRgaFree(&g_bo);
    if (ret)
        printf("c_RkRgaFree error : %s\n", strerror(errno));
}

void shadow_rga_refresh(int src_w, int src_h, int dst_w, int dst_h, int rotate)
{
    int ret;
    int bpp;
    rga_info_t src;
    rga_info_t dst;
    int srcFormat;
    int dstFormat;
    struct bo *bo = NULL;

    bo = getdrmdisp();
    ret = c_RkRgaGetBufferFd(bo, &g_dst_fd);
    if (ret) {
        printf("c_RkRgaGetBufferFd error : %s\n", strerror(errno));
        return;
    }

    memset(&src, 0, sizeof(rga_info_t));
    src.fd = g_src_fd;
    src.mmuFlag = 1;
    memset(&dst, 0, sizeof(rga_info_t));
    dst.fd = g_dst_fd;
    dst.mmuFlag = 1;

    getdrmdispbpp(&bpp);
    if (bpp == 32) {
        srcFormat = RK_FORMAT_RGBA_8888;
        dstFormat = RK_FORMAT_RGBA_8888;
    } else {
        srcFormat = RK_FORMAT_RGB_565;
        dstFormat = RK_FORMAT_RGB_565;
    }
    rga_set_rect(&src.rect, 0, 0, src_w, src_h, src_w, src_h, srcFormat);
    rga_set_rect(&dst.rect, 0, 0, dst_w, dst_h, dst_w, dst_h, dstFormat);
    src.rotation = rotate;
    ret = c_RkRgaBlit(&src, &dst, NULL);
    if (ret)
        printf("c_RkRgaBlit error : %s\n", strerror(errno));
    close(g_dst_fd);
    setdrmdisp(bo);
}
