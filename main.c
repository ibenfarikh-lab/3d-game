#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <graph.h>
#include <draw.h>
#include <packet.h>
#include <dma.h>
#include <gs_psm.h>
#include <gs_gp.h>

// Fungsi sprite mandiri yang aman dan stabil
qword_t *draw_box(qword_t *q, int r, int g, int b, int x1, int y1, int x2, int y2) {
    q->dw[0] = GIF_SET_TAG(2, 1, 1, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 2);
    q->dw[1] = GS_REG_RGBAQ | (GS_REG_XYZ2 << 4);
    q++;

    q->dw[0] = (u64)r | ((u64)g << 8) | ((u64)b << 16) | ((u64)128 << 24);
    q->dw[1] = 0; 
    q++;
    q->dw[0] = GS_SET_XYZ((x1 + 2048) << 4, (y1 + 2048) << 4, 0); 
    q->dw[1] = 0; 
    q++;

    q->dw[0] = (u64)r | ((u64)g << 8) | ((u64)b << 16) | ((u64)128 << 24);
    q->dw[1] = 0; 
    q++;
    q->dw[0] = GS_SET_XYZ((x2 + 2048) << 4, (y2 + 2048) << 4, 0); 
    q->dw[1] = 0; 
    q++;

    return q;
}

int main(int argc, char *argv[]) {
    SifInitRpc(0);

    // Inisialisasi Jalur Grafis persis seperti uji coba hijau yang berhasil
    graph_set_mode(GRAPH_MODE_INTERLACED, GRAPH_MODE_PAL, GRAPH_MODE_FIELD, GRAPH_ENABLE);
    
    framebuffer_t frame;
    zbuffer_t z;

    frame.width = 640;
    frame.height = 512; // Resolusi standar PAL yang cocok dengan BIOS Eropa kamu
    frame.mask = 0;
    frame.psm = GS_PSM_32;
    frame.address = graph_vram_allocate(frame.width, frame.height, frame.psm, GRAPH_ALIGN_PAGE);

    z.enable = 0;
    z.mask = 0;
    z.method = ZTEST_METHOD_GREATER_EQUAL;
    z.zsm = GS_PSMZ_16;
    z.address = graph_vram_allocate(frame.width, frame.height, z.zsm, GRAPH_ALIGN_PAGE);

    graph_initialize(frame.address, frame.width, frame.height, frame.psm, 0, 0);

    // Inisialisasi DMA GIF
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    packet_t *packet = packet_init(5000, PACKET_NORMAL);
    qword_t *q = packet->data;
    q = draw_setup_environment(q, 0, &frame, &z);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    draw_wait_finish();

    int posX = 320;
    int posY = 256;

    while (1) {
        q = packet->data;

        // Bersihkan layar dengan warna latar belakang Biru Tua agar kontras
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 256.0f, frame.width, frame.height, 20, 40, 100);

        // Gambar Papan (Kotak Abu-abu)
        q = draw_box(q, 150, 150, 150, 140, 100, 500, 412);

        // Gambar Bidak (Kotak Kuning)
        q = draw_box(q, 255, 255, 0, posX - 20, posY - 20, posX + 20, posY + 20);

        q = draw_finish(q);
        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packet);
    SleepThread();
    return 0;
}
