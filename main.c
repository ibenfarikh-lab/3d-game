#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <graph.h>
#include <draw.h>
#include <packet.h>
#include <dma.h>
#include <gs_psm.h>
#include <gs_gp.h>

// Offset mutlak layar PS2 (2048 - setengah resolusi)
#define OFFSET_X 1728  // 2048 - 320
#define OFFSET_Y 1792  // 2048 - 256

// Fungsi gambar kotak antigagal (langsung kirim sinyal ke GPU PS2)
qword_t *draw_box(qword_t *q, int r, int g, int b, int x, int y, int w, int h) {
    // Tambahkan offset agar kotak tepat berada di dalam layar TV
    int x1 = OFFSET_X + x;
    int y1 = OFFSET_Y + y;
    int x2 = x1 + w;
    int y2 = y1 + h;

    q->dw[0] = GIF_SET_TAG(2, 1, 1, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 2);
    q->dw[1] = GS_REG_RGBAQ | (GS_REG_XYZ2 << 4);
    q++;

    q->dw[0] = (u64)r | ((u64)g << 8) | ((u64)b << 16) | (128ULL << 24);
    q->dw[1] = 0;
    q++;
    q->dw[0] = GS_SET_XYZ(x1 << 4, y1 << 4, 0); // Dikali 16 sesuai format koordinat PS2
    q->dw[1] = 0;
    q++;

    q->dw[0] = (u64)r | ((u64)g << 8) | ((u64)b << 16) | (128ULL << 24);
    q->dw[1] = 0;
    q++;
    q->dw[0] = GS_SET_XYZ(x2 << 4, y2 << 4, 0);
    q->dw[1] = 0;
    q++;

    return q;
}

static framebuffer_t frame;
static zbuffer_t zbuf;

int main(int argc, char *argv[]) {
    SifInitRpc(0);

    graph_set_mode(GRAPH_MODE_INTERLACED, GRAPH_MODE_PAL, GRAPH_MODE_FIELD, GRAPH_ENABLE);

    frame.width = 640;
    frame.height = 512;
    frame.mask = 0;
    frame.psm = GS_PSM_32;
    frame.address = graph_vram_allocate(frame.width, frame.height, frame.psm, GRAPH_ALIGN_PAGE);

    zbuf.enable = 1;
    zbuf.mask = 0;
    zbuf.method = ZTEST_METHOD_GREATER_EQUAL;
    zbuf.zsm = GS_PSMZ_16;
    zbuf.address = graph_vram_allocate(frame.width, frame.height, zbuf.zsm, GRAPH_ALIGN_PAGE);

    graph_initialize(frame.address, frame.width, frame.height, frame.psm, 0, 0);

    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    packet_t *packet = packet_init(100, PACKET_NORMAL);

    while (1) {
        packet_reset(packet); // <-- FIXED: Cukup 1 argumen sesuai log error terbaru
        qword_t *q = packet->data;

        q = draw_setup_environment(q, 0, &frame, &zbuf);

        // 1. Latar Belakang Biru Tua (Koordinatnya otomatis pas di layar)
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 256.0f, frame.width, frame.height, 20, 40, 100);

        // 2. Gambar Papan (Kotak Abu-abu, Posisi X:160 Y:100, Lebar:320 Tinggi:312)
        q = draw_box(q, 160, 160, 160, 160, 100, 320, 312);

        // 3. Gambar Bidak (Kotak Kuning, Posisi X:300 Y:230, Lebar:40 Tinggi:40)
        q = draw_box(q, 255, 255, 0, 300, 230, 40, 40);

        q = draw_finish(q);

        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packet);
    SleepThread();
    return 0;
}
