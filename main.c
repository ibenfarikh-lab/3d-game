#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <graph.h>
#include <draw.h>
#include <math3d.h>
#include <packet.h>

// --- 1. DATA PAPAN (Lantai Datar / Plane) ---
// Kita buat papan sederhana berukuran 10x10 menggunakan 4 titik (2 segitiga)
VECTOR papan_vertices[4] = {
    {-5.0f, 0.0f, -5.0f, 1.0f}, // 0: Kiri-Belakang
    { 5.0f, 0.0f, -5.0f, 1.0f}, // 1: Kanan-Belakang
    {-5.0f, 0.0f,  5.0f, 1.0f}, // 2: Kiri-Depan
    { 5.0f, 0.0f,  5.0f, 1.0f}  // 3: Kanan-Depan
};
color_t warna_papan = {150, 150, 150, 128, 0}; // Abu-abu

// --- 2. DATA BIDAK (Limas Segiempat) ---
VECTOR bidak_vertices[5] = {
    { 0.0f,  2.0f,  0.0f, 1.0f }, // 0: Puncak atas
    {-1.0f, -1.0f, -1.0f, 1.0f }, // 1: Kiri-Depan bawah
    { 1.0f, -1.0f, -1.0f, 1.0f }, // 2: Kanan-Depan bawah
    { 1.0f, -1.0f,  1.0f, 1.0f }, // 3: Kanan-Belakang bawah
    {-1.0f, -1.0f,  1.0f, 1.0f }  // 4: Kiri-Belakang bawah
};

color_t bidak_colors[4] = {
    {255, 0, 0, 128, 0},   // Depan: Merah
    {0, 255, 0, 128, 0},   // Kanan: Hijau
    {0, 0, 255, 128, 0},   // Belakang: Biru
    {255, 255, 0, 128, 0}  // Kiri: Kuning
};

int bidak_faces[4][3] = {
    {0, 1, 2}, // Segitiga Depan
    {0, 2, 3}, // Segitiga Kanan
    {0, 3, 4}, // Segitiga Belakang
    {0, 4, 1}  // Segitiga Kiri
};

// --- FUNGSI UTAMA ---
int main(int argc, char **argv) {
    SifInitRpc(0);
    
    // Inisialisasi Layar & Z-Buffer
    framebuffer_t frame;
    zbuffer_t z;
    graph_wait_vsync();
    graph_initialize(GRAPH_MODE_NTSC, GRAPH_MODE_INTERLACED, GRAPH_MODE_FIELD, GRAPH_PSM_32);
    
    frame.width = 640;
    frame.height = 448;
    frame.mask = 0;
    frame.psm = GS_PSM_32;
    frame.address = graph_vram_allocate(frame.width, frame.height, frame.psm, GRAPH_ALIGN_PAGE);

    z.enable = 1;
    z.mask = 0;
    z.method = ZTEST_METHOD_GREATER_EQUAL; // Penting agar objek depan menutupi yang belakang
    z.zsm = GS_ZBUF_32;
    z.address = graph_vram_allocate(frame.width, frame.height, z.zsm, GRAPH_ALIGN_PAGE);
    
    graph_set_framebuffer_filtered(frame.address, frame.width, frame.psm, 0, 0);
    graph_set_zbuffer(z.address, z.zsm);

    // Persiapan Kamera
    MATRIX local_world, world_view, view_screen, local_screen;
    VECTOR camera_position = { 0.0f, 6.0f, 15.0f, 1.0f }; // Kamera ditarik mundur & ke atas
    VECTOR camera_rotation = { -0.4f, 0.0f, 0.0f, 1.0f }; // Kamera menunduk melihat papan
    
    create_view_screen(view_screen, graph_aspect_ratio(), -3.0f, 3.0f, -3.0f, 3.0f, 1.0f, 2000.0f);
    
    // Koordinat Posisi & Rotasi
    VECTOR papan_translation = { 0.0f, 0.0f, 0.0f, 1.0f }; // Papan di titik nol (Y=0)
    VECTOR papan_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    // PERBAIKAN POSISI Y: Dinaikkan menjadi 1.0f agar titik terbawah limas (-1.0f) menyentuh papan (0.0f)
    VECTOR bidak_translation = { 0.0f, 1.0f, 0.0f, 1.0f }; 
    VECTOR bidak_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    // PERBAIKAN PAKET: Alokasi diperbesar dari 100 menjadi 5000 agar muat papan & bidak
    packet_t *packet = packet_init(5000, PACKET_NORMAL);
    qword_t *q;

    while (1) {
        // Putar bidak
        bidak_rotation[1] += 0.05f;

        // Hitung Proyeksi Kamera
        create_world_view(world_view, camera_position, camera_rotation);

        q = packet->data;
        // Latar Belakang Hitam Pekat
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 224.0f, frame.width, frame.height, 0, 0, 0);

        // ==========================================
        // 1. RENDER PAPAN DAHULU
        // ==========================================
        create_local_world(local_world, papan_translation, papan_rotation);
        create_local_screen(local_screen, local_world, world_view, view_screen);

        VECTOR p0, p1, p2, p3;
        vector_apply(p0, papan_vertices[0], local_screen);
        vector_apply(p1, papan_vertices[1], local_screen);
        vector_apply(p2, papan_vertices[2], local_screen);
        vector_apply(p3, papan_vertices[3], local_screen);

        // Gambar 2 segitiga untuk membentuk 1 persegi (papan)
        q = draw_triangle(q, warna_papan, p0[0], p0[1], p0[2], warna_papan, p1[0], p1[1], p1[2], warna_papan, p2[0], p2[1], p2[2]);
        q = draw_triangle(q, warna_papan, p2[0], p2[1], p2[2], warna_papan, p1[0], p1[1], p1[2], warna_papan, p3[0], p3[1], p3[2]);

        // ==========================================
        // 2. RENDER BIDAK DI ATAS PAPAN
        // ==========================================
        create_local_world(local_world, bidak_translation, bidak_rotation);
        create_local_screen(local_screen, local_world, world_view, view_screen);

        int i;
        for (i = 0; i < 4; i++) {
            VECTOR v0, v1, v2;
            vector_apply(v0, bidak_vertices[bidak_faces[i][0]], local_screen);
            vector_apply(v1, bidak_vertices[bidak_faces[i][1]], local_screen);
            vector_apply(v2, bidak_vertices[bidak_faces[i][2]], local_screen);

            q = draw_triangle(q, 
                bidak_colors[i], v0[0], v0[1], v0[2],
                bidak_colors[i], v1[0], v1[1], v1[2],
                bidak_colors[i], v2[0], v2[1], v2[2]
            );
        }

        // Selesai menggambar frame ini, kirim ke TV / PCSX2
        q = draw_finish(q);
        dma_channel_send_normal(DMA_CHANNEL_GIF, packet->data, q - packet->data, 0, 0);
        draw_wait_finish();
        graph_wait_vsync();
    }

    packet_free(packet);
    SleepThread();
    return 0;
}
