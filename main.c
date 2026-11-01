#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <graph.h>
#include <draw.h>
#include <packet.h>
#include <dma.h>
#include <gs_psm.h>
#include <gs_gp.h>

int main(int argc, char **argv) {
    SifInitRpc(0);
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);
    
    framebuffer_t frame;
    zbuffer_t z;
    
    frame.width = 640;
    frame.height = 448;
    frame.mask = 0;
    frame.psm = GS_PSM_32;
    frame.address = graph_vram_allocate(frame.width, frame.height, frame.psm, GRAPH_ALIGN_PAGE);

    z.enable = 0;
    z.mask = 0;
    z.method = ZTEST_METHOD_GREATER_EQUAL;
    z.zsm = GS_PSMZ_32;
    z.address = graph_vram_allocate(frame.width, frame.height, z.zsm, GRAPH_ALIGN_PAGE);
    
    graph_initialize(frame.address, frame.width, frame.height, frame.psm, 0, 0);

    packet_t *packet = packet_init(5000, PACKET_NORMAL);
    qword_t *q = packet->data;
    q = draw_setup_environment(q, 0, &frame, &z);
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
    draw_wait_finish();

    // Posisi bidak
    int posX = 320;
    int posY = 224;

    while (1) {
        q = packet->data;
        
        // 1. Bersihkan layar (Background Hitam)
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 224.0f, frame.width, frame.height, 0, 0, 0);

        // 2. Definisi Warna Aman (Tanpa Excess Elements Warning)
        color_t warna_papan;
        color_t warna_bidak;
        
        color_set(&warna_papan, 100, 100, 100, 128);
        color_set(&warna_bidak, 255, 0, 0, 128);

        // 3. Gambar Papan & Bidak (Menggunakan pointer q langsung sesuai standar fungsi draw modern)
        q = draw_quad(q, &warna_papan, 120, 100, 520, 100, 520, 380, 120, 380);
        q = draw_quad(q, &warna_bidak, posX - 15, posY - 15, posX + 15, posY - 15, posX + 15, posY + 15, posX - 15, posY + 15);

        q = draw_finish(q);
        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packet);
    SleepThread();
    return 0;
}
