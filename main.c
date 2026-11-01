#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <graph.h>
#include <draw.h>
#include <packet.h>
#include <dma.h>
#include <gs_psm.h>
#include <gs_gp.h>

int main(int argc, char *argv[]) {
    SifInitRpc(0);

    // Set PAL mode agar sinkron dengan BIOS Eropa
    graph_set_mode(GRAPH_MODE_INTERLACED, GRAPH_MODE_PAL, GRAPH_MODE_FIELD, GRAPH_ENABLE);
    
    framebuffer_t frame;
    zbuffer_t z;

    frame.width = 640;
    frame.height = 512;
    frame.mask = 0;
    frame.psm = GS_PSM_32;
    frame.address = graph_vram_allocate(frame.width, frame.height, frame.psm, GRAPH_ALIGN_PAGE);

    z.enable = 0;
    z.mask = 0;
    z.method = ZTEST_METHOD_GREATER_EQUAL;
    z.zsm = GS_PSMZ_16;
    z.address = graph_vram_allocate(frame.width, frame.height, z.zsm, GRAPH_ALIGN_PAGE);

    graph_initialize(frame.address, frame.width, frame.height, frame.psm, 0, 0);

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

        // 1. Bersihkan layar dengan latar belakang Biru Tua
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 256.0f, frame.width, frame.height, 20, 40, 100);

        // 2. Definisi warna objek
        color_t warna_papan = {180, 180, 180, 128, 0};
        color_t warna_bidak = {255, 255, 0, 128, 0};

        // 3. Menggunakan draw_rect bawaan PS2SDK (Papan dan Bidak)
        q = draw_rect(q, &warna_papan, 140, 100, 360, 312); // Papan (x, y, width, height)
        q = draw_rect(q, &warna_bidak, posX - 20, posY - 20, 40, 40); // Bidak Kuning

        q = draw_finish(q);
        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packet);
    SleepThread();
    return 0;
}
