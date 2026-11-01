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

typedef struct { u8 r, g, b, a; } color3d_t;

// --- 1. DATA PAPAN ---
VECTOR papan_vertices[4] = {
    {-3.0f, 0.0f, -3.0f, 1.0f},
    { 3.0f, 0.0f, -3.0f, 1.0f},
    {-3.0f, 0.0f,  3.0f, 1.0f},
    { 3.0f, 0.0f,  3.0f, 1.0f}
};
color3d_t warna_papan = {100, 100, 100, 128}; // Abu-abu gelap

// --- 2. DATA BIDAK (Limas) ---
VECTOR bidak_vertices[5] = {
    { 0.0f,  1.5f,  0.0f, 1.0f }, // Puncak
    {-0.5f, 0.0f, -0.5f, 1.0f },
    { 0.5f, 0.0f, -0.5f, 1.0f },
    { 0.5f, 0.0f,  0.5f, 1.0f },
    {-0.5f, 0.0f,  0.5f, 1.0f }
};

color3d_t bidak_colors[4] = {
    {255, 0, 0, 128},   // Merah
    {0, 255, 0, 128},   // Hijau
    {0, 0, 255, 128},   // Biru
    {255, 255, 0, 128}  // Kuning
};

int bidak_faces[4][3] = {
    {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 1}
};

// --- RENDER 3D AMAN ---
qword_t *draw_triangle_3d(qword_t *q, color3d_t c0, VECTOR v0, color3d_t c1, VECTOR v1, color3d_t c2, VECTOR v2) {
    if(v0[3] != 0.0f) { v0[0] /= v0[3]; v0[1] /= v0[3]; v0[2] /= v0[3]; }
    if(v1[3] != 0.0f) { v1[0] /= v1[3]; v1[1] /= v1[3]; v1[2] /= v1[3]; }
    if(v2[3] != 0.0f) { v2[0] /= v2[3]; v2[1] /= v2[3]; v2[2] /= v2[3]; }

    int x0 = (int)(v0[0] * 16.0f) + 2048; int y0 = (int)(v0[1] * 16.0f) + 2048;
    int x1 = (int)(v1[0] * 16.0f) + 2048; int y1 = (int)(v1[1] * 16.0f) + 2048;
    int x2 = (int)(v2[0] * 16.0f) + 2048; int y2 = (int)(v2[1] * 16.0f) + 2048;

    q->dw[0] = GIF_SET_TAG(3, 1, 1, GS_SET_PRIM(GS_PRIM_TRIANGLE, 1, 0, 0, 0, 0, 0, 0, 0), GIF_FLG_PACKED, 2);
    q->dw[1] = GS_REG_RGBAQ | (GS_REG_XYZ2 << 4);
    q++;

    q->dw[0] = (u64)c0.r | ((u64)c0.g << 8) | ((u64)c0.b << 16) | ((u64)c0.a << 24); q->dw[1] = 0; q++;
    q->dw[0] = GS_SET_XYZ(x0, y0, (u32)v0[2]); q->dw[1] = 0; q++;

    q->dw[0] = (u64)c1.r | ((u64)c1.g << 8) | ((u64)c1.b << 16) | ((u64)c1.a << 24); q->dw[1] = 0; q++;
    q->dw[0] = GS_SET_XYZ(x1, y1, (u32)v1[2]); q->dw[1] = 0; q++;

    q->dw[0] = (u64)c2.r | ((u64)c2.g << 8) | ((u64)c2.b << 16) | ((u64)c2.a << 24); q->dw[1] = 0; q++;
    q->dw[0] = GS_SET_XYZ(x2, y2, (u32)v2[2]); q->dw[1] = 0; q++;

    return q;
}

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

    MATRIX local_world, world_view, view_screen, local_screen;
    VECTOR camera_position = { 0.0f, 4.0f, 10.0f, 1.0f };
    VECTOR camera_rotation = { -0.3f, 0.0f, 0.0f, 1.0f }; 
    
    create_view_screen(view_screen, graph_aspect_ratio(), -3.0f, 3.0f, -3.0f, 3.0f, 1.0f, 2000.0f);
    
    VECTOR papan_translation = { 0.0f, 0.0f, 0.0f, 1.0f };
    VECTOR papan_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
    
    VECTOR bidak_translation = { 0.0f, 0.0f, 0.0f, 1.0f }; 
    VECTOR bidak_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    while (1) {
        bidak_rotation[1] += 0.04f; // Putar bidak perlahan
        create_world_view(world_view, camera_position, camera_rotation);

        q = packet->data;
        // Bersihkan layar tetap dengan latar belakang hitam agar kontras dengan objek 3D
        q = draw_clear(q, 0, 2048.0f - 320.0f, 2048.0f - 224.0f, frame.width, frame.height, 0, 0, 0);

        // 1. RENDER PAPAN
        create_local_world(local_world, papan_translation, papan_rotation);
        create_local_screen(local_screen, local_world, world_view, view_screen);

        VECTOR p0, p1, p2, p3;
        vector_apply(p0, papan_vertices[0], local_screen);
        vector_apply(p1, papan_vertices[1], local_screen);
        vector_apply(p2, papan_vertices[2], local_screen);
        vector_apply(p3, papan_vertices[3], local_screen);

        q = draw_triangle_3d(q, warna_papan, p0, warna_papan, p1, warna_papan, p2);
        q = draw_triangle_3d(q, warna_papan, p2, warna_papan, p1, warna_papan, p3);

        // 2. RENDER BIDAK
        create_local_world(local_world, bidak_translation, bidak_rotation);
        create_local_screen(local_screen, local_world, world_view, view_screen);

        int i;
        for (i = 0; i < 4; i++) {
            VECTOR v0, v1, v2;
            vector_apply(v0, bidak_vertices[bidak_faces[i][0]], local_screen);
            vector_apply(v1, bidak_vertices[bidak_faces[i][1]], local_screen);
            vector_apply(v2, bidak_vertices[bidak_faces[i][2]], local_screen);

            q = draw_triangle_3d(q, bidak_colors[i], v0, bidak_colors[i], v1, bidak_colors[i], v2);
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
