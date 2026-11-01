#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <graph.h>
#include <draw.h>
#include <math3d.h>
#include <packet.h>
#include <dma.h>
#include <gs_psm.h>
#include <gs_gp.h>

// --- 1. DATA BIDAK (LIMAS SEGIEMPAT) ---
VECTOR bidak_vertices[5] = {
    { 0.0f,  2.0f,  0.0f, 1.0f }, // 0: Puncak atas
    {-1.0f, -1.0f, -1.0f, 1.0f }, // 1: Kiri-Depan bawah
    { 1.0f, -1.0f, -1.0f, 1.0f }, // 2: Kanan-Depan bawah
    { 1.0f, -1.0f,  1.0f, 1.0f }, // 3: Kanan-Belakang bawah
    {-1.0f, -1.0f,  1.0f, 1.0f }  // 4: Kiri-Belakang bawah
};

color_t colors[4] = {
    {255, 0, 0, 128, 0},   // Depan: Merah
    {0, 255, 0, 128, 0},   // Kanan: Hijau
    {0, 0, 255, 128, 0},   // Belakang: Biru
    {255, 255, 0, 128, 0}  // Kiri: Kuning
};

int faces[4][3] = {
    {0, 1, 2},
    {0, 2, 3},
    {0, 3, 4},
    {0, 4, 1}
};

// --- 2. FUNGSI GAMBAR SEGITIGA MANUAL (Pengganti draw_triangle yang gaib) ---
qword_t *draw_triangle_3d(qword_t *q, color_t c, float x0, float y0, float x1, float y1, float x2, float y2) {
    // Tag GIF untuk Segitiga 3D
    q->dw[0] = GIF_SET_TAG(3, 1, 1, GS_SET_PRIM(GS_PRIM_TRIANGLE, 0, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 2);
    q->dw[1] = GS_REG_RGBAQ | (GS_REG_XYZ2 << 4);
    q++;

    // Kalkulasi Offset Tengah Layar PS2 (2048) & Ubah ke Fixed-Point (dikali 16)
    // Titik 1
    q->dw[0] = (u64)c.r | ((u64)c.g << 8) | ((u64)c.b << 16) | (128ULL << 24);
    q->dw[1] = 0; q++;
    q->dw[0] = GS_SET_XYZ((int)((x0 + 2048.0f) * 16.0f), (int)((y0 + 2048.0f) * 16.0f), 0);
    q->dw[1] = 0; q++;

    // Titik 2
    q->dw[0] = (u64)c.r | ((u64)c.g << 8) | ((u64)c.b << 16) | (128ULL << 24);
    q->dw[1] = 0; q++;
    q->dw[0] = GS_SET_XYZ((int)((x1 + 2048.0f) * 16.0f), (int)((y1 + 2048.0f) * 16.0f), 0);
    q->dw[1] = 0; q++;

    // Titik 3
    q->dw[0] = (u64)c.r | ((u64)c.g << 8) | ((u64)c.b << 16) | (128ULL << 24);
    q->dw[1] = 0; q++;
    q->dw[0] = GS_SET_XYZ((int)((x2 + 2048.0f) * 16.0f), (int)((y2 + 2048.0f) * 16.0f), 0);
    q->dw[1] = 0; q++;

    return q;
}

// --- 3. FUNGSI UTAMA ---
int main(int argc, char **argv) {
    SifInitRpc(0);
    
    // Tetap gunakan mode PAL yang sudah terbukti jalan di hardware/emulator-mu
    graph_set_mode(GRAPH_MODE_INTERLACED, GRAPH_MODE_PAL, GRAPH_MODE_FIELD, GRAPH_ENABLE);
    
    framebuffer_t frame;
    zbuffer_t z;

    frame.width = 640;
    frame.height = 512;
    frame.mask = 0;
    frame.psm = GS_PSM_32;
    frame.address = graph_vram_allocate(frame.width, frame.height, frame.psm, GRAPH_ALIGN_PAGE);

    z.enable = 1;
    z.mask = 0;
    z.method = ZTEST_METHOD_GREATER_EQUAL;
    z.zsm = GS_PSMZ_16;
    z.address = graph_vram_allocate(frame.width, frame.height, z.zsm, GRAPH_ALIGN_PAGE);
    
    graph_initialize(frame.address, frame.width, frame.height, frame.psm, 0, 0);
    dma_channel_initialize(DMA_CHANNEL_GIF, NULL, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    // Persiapan Kamera 3D
    MATRIX local_world, world_view, view_screen, local_screen;
    VECTOR camera_position = { 0.0f, 2.0f, 8.0f, 1.0f }; // Kamera mundur agar bidak terlihat utuh
    VECTOR camera_rotation = { -0.2f, 0.0f, 0.0f, 1.0f }; // Menunduk sedikit
    
    create_view_screen(view_screen, 640.0f/512.0f, -3.0f, 3.0f, -3.0f, 3.0f, 1.0f, 2000.0f);
    
    VECTOR bidak_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    VECTOR bidak_translation = { 0.0f, 0.0f, 0.0f, 1.0f };

    // FIX: Besarkan ukuran paket agar tidak jebol
    packet_t *packet = packet_init(500, PACKET_NORMAL);

    while (1) {
        // FIX: Mutlak harus di-reset setiap frame baru dimulai!
        packet_reset(packet);
        qword_t *q = packet->data;

        // Putar bidak pelan-pelan
        bidak_rotation[1] += 0.05f;

        // Hitung posisi ruang 3D
        create_local_world(local_world, bidak_translation, bidak_rotation);
        create_world_view(world_view, camera_position, camera_rotation);
        create_local_screen(local_screen, local_world, world_view, view_screen);

        // Setup Layar & Background Abu-abu gelap agar bidak warna-warni kontras
        q = draw_setup_environment(q, 0, &frame, &z);
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 256.0f, frame.width, frame.height, 30, 30, 30);

        // Gambar 4 sisi Piramida
        int i;
        for (i = 0; i < 4; i++) {
            VECTOR v0, v1, v2;
            vector_apply(v0, bidak_vertices[faces[i][0]], local_screen);
            vector_apply(v1, bidak_vertices[faces[i][1]], local_screen);
            vector_apply(v2, bidak_vertices[faces[i][2]], local_screen);

            // Eksekusi fungsi manual kita
            q = draw_triangle_3d(q, colors[i], v0[0], v0[1], v1[0], v1[1], v2[0], v2[1]);
        }

        q = draw_finish(q);
        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packet);
    SleepThread();
    return 0;
}
