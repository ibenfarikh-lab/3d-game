#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <graph.h>
#include <draw.h>
#include <packet.h>
#include <dma.h>
#include <gs_psm.h>
#include <draw2d.h>

static framebuffer_t frame;
static zbuffer_t zbuf;

int main(int argc, char *argv[]) {
    SifInitRpc(0);

    // Inisialisasi mode video PAL sesuai spesifikasi hardware dan BIOS
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

    // Alokasi ukuran packet
    packet_t *packet = packet_init(50, PACKET_NORMAL);

    while (1) {
        packet_reset(packet, 0);
        qword_t *q = packet->data;

        // Setup lingkungan render per frame
        q = draw_setup_environment(q, 0, &frame, &zbuf);

        // Bersihkan layar dengan warna latar belakang Biru
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 256.0f, frame.width, frame.height, 20, 40, 100);

        // 1. Gambar Papan (Kotak Abu-abu menggunakan rect_t)
        rect_t papan_rect;
        papan_rect.xpos = 160;
        papan_rect.ypos = 100;
        papan_rect.width = 320;
        papan_rect.height = 312;
        papan_rect.color.r = 160;
        papan_rect.color.g = 160;
        papan_rect.color.b = 160;
        papan_rect.color.a = 0;
        papan_rect.color.q = 0;

        q = draw_rect_filled(q, 0, &papan_rect);

        // 2. Gambar Bidak (Kotak Kuning menggunakan rect_t)
        rect_t bidak_rect;
        bidak_rect.xpos = 300;
        bidak_rect.ypos = 230;
        bidak_rect.width = 40;
        bidak_rect.height = 40;
        bidak_rect.color.r = 255;
        bidak_rect.color.g = 255;
        bidak_rect.color.b = 0;
        bidak_rect.color.a = 0;
        bidak_rect.color.q = 0;

        q = draw_rect_filled(q, 0, &bidak_rect);

        q = draw_finish(q);

        // Kirim data via DMA channel GIF ke GS
        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packet);
    SleepThread();
    return 0;
}
