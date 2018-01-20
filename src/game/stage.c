/// Stage (source)
/// (c) 2018 Jani Nykänen

#include "stage.h"

#include "decoration.h"

#include "../engine/graphics.h"
#include "../engine/transform.h"

// Grass texture
static BITMAP* bmpGrass;
// Road texture
static BITMAP* bmpRoad;
// Forest texture
static BITMAP* bmpForest;
// Mountains texture
static BITMAP* bmpMountains;
// Moon texture
static BITMAP* bmpMoon;
// Fence texture
static BITMAP* bmpFence;
// House texture
static BITMAP* bmpHouse;
// Fir texture
static BITMAP* bmpFir;

// House model
static MESH* mHouse;
// Fir model
static MESH* mFir;

// Maximum amount of decorations
#define DEC_MAX 32
// Decorations
static DECORATION decorations[DEC_MAX];
// Decoration count
static int decCount;


// Draw a floor tile (or piece of it)
static void draw_floor_tile_small(float x, float y, float z, float w, float h, float u, float v, float uw, float vh)
{

    draw_triangle_3d(vec3(x,y,z),vec3(x+w,y,z),vec3(x+w,y,z+h),
        vec2(u,v),vec2(u+uw,v),vec2(u+uw,v+vh),vec3(0,1,0));

    draw_triangle_3d(vec3(x+w,y,z+h),vec3(x,y,z+h),vec3(x,y,z),
       vec2(u+uw,v+vh),vec2(u,v+vh),vec2(u,v),vec3(0,1,0));

}

// Draw a floor tile
static void draw_floor_tile(float x, float y, float z, float w, float h, int subdivide)
{
    int dx = 0, dz = 0;
    float stepw = w / subdivide;
    float steph = h / subdivide;
    float stepuv = 1.0 / subdivide;

    for(dz = 0; dz < subdivide; ++ dz)
    {
        for(dx = 0; dx < subdivide; ++ dx)
        {
             draw_floor_tile_small(x + stepw*dx, y, z + steph*dz,stepw,steph,stepuv * dx,stepuv*dz,stepuv,stepuv);
        }
    }

}


// Draw floor
static void draw_floor(CAMERA* cam, float y)
{
    bind_texture(bmpGrass);

    const float TILE_SIZE = 10.0f;
    const int TILE_COUNT = 8;

    int x = (int)floor(cam->pos.x / TILE_SIZE);
    int z = (int)floor(cam->pos.z / TILE_SIZE);

    int sx = x - TILE_COUNT / 2;
    int sz = z - TILE_COUNT / 2;
    int ex = sx + TILE_COUNT;
    int ez = sz + TILE_COUNT;

    int stepx = 1;
    int stepz = 1;
    int subdivide = 1;

    int tcount = 0;

    int dx, dz;
    for(dz = sz; dz <= ez; dz += stepz)
    {
        for(dx = sx; dx <= ex; dx += stepx)
        {
            bind_texture(dx == 0 ? bmpRoad : bmpGrass);

            subdivide = 1;

            if(dx >= sx + TILE_COUNT/2 - 2 && dx <= ex - TILE_COUNT/2 + 2
            && dz >= sz + TILE_COUNT/2 - 2 && dz <= ez - TILE_COUNT/2 + 2) 
                subdivide = 2;

            if(dx >= sx + TILE_COUNT/2 - 1 && dx <= ex - TILE_COUNT/2 + 1
            && dz >= sz + TILE_COUNT/2 - 1 && dz <= ez - TILE_COUNT/2 + 1) 
                subdivide = 4;

            draw_floor_tile(dx * TILE_SIZE,y, dz * TILE_SIZE, TILE_SIZE, TILE_SIZE,subdivide);

            tcount += subdivide*subdivide;
        }
    }
}


// Draw background
static void draw_background(CAMERA* cam)
{
    clear_frame(0);
    int w = bmpForest->w;

    int y = 80 - (int)(cam->pos.y*3);
    
    float angle = cam->angle.y;
    while(angle >= M_PI*2) angle -= M_PI*2;
    while(angle < 0) angle += M_PI*2;

    int posx = -(int)(angle/(2*M_PI) * 1024);

    int i = 0;

    // Mountains
    for(; i < 3; ++ i)
    {
        draw_bitmap(bmpMountains,posx + i*bmpMountains->w,0,0);
    }

    // Forest
    for(i = 0; i < 6; ++ i)
    {
        draw_bitmap(bmpForest,posx + i*w,y,0);
    }

    // Draw moon
    if(posx > -512)
    {
        draw_bitmap(bmpMoon,480+posx,10,0);
    }

}


// Draw horizontal fence plane
static void draw_fence_plane_h(CAMERA* cam, float x,float y,float z, float w, float h)
{
    float dist = hypot(cam->vpos.x-(x+w/2),cam->vpos.z-z);

    int subdivide = 1;
    if(dist < 4*w) subdivide = 2;
    if(dist < 2*w) subdivide = 4;

    int dx = 0, dy = 0;
    float stepw = w / subdivide;
    float steph = h / subdivide;
    float stepuv = 1.0 / subdivide;

    float tx, ty, tz;
    float u,v;

    for(dx = 0; dx < subdivide; ++ dx)
    {
        for(dy = 0; dy < subdivide; ++ dy)
        {
            tx = x + stepw*dx;
            ty = y + steph*dy;
            tz = z;
                
            u = stepuv*dx;
            v = stepuv*dy;

            draw_triangle_3d(vec3(tx,ty,tz),vec3(tx+stepw,ty,tz),vec3(tx+stepw,ty+steph,tz),
                vec2(u,v),vec2(u+stepuv,v),vec2(u+stepuv,v+stepuv),vec3(0,0,1));

            draw_triangle_3d(vec3(tx+stepw,ty+steph,tz),vec3(tx,ty+steph,tz),vec3(tx,ty,tz),
                vec2(u+stepuv,v+stepuv),vec2(u,v+stepuv),vec2(u,v),vec3(0,0,1));
        }
    }
}


// Draw depth-direction fence plane
static void draw_fence_plane_d(CAMERA* cam, float x,float y,float z, float w, float h)
{

    float dist = hypot(cam->vpos.x-x,cam->vpos.z-(z+w/2));

    int subdivide = 1;
    if(dist < 4*w) subdivide = 2;
    if(dist < 2*w) subdivide = 4;

    int dx = 0, dy = 0;
    float stepw = w / subdivide;
    float steph = h / subdivide;

    float stepuv = 1.0 / subdivide;

    float tx, ty, tz;
    float u,v;

    for(dx = 0; dx < subdivide; ++ dx)
    {
        for(dy = 0; dy < subdivide; ++ dy)
        {
            tx = x;
            ty = y+steph*dy;
            tz = z+stepw*dx;
                
            u = stepuv*dx;
            v = stepuv*dy;

            draw_triangle_3d(vec3(tx,ty,tz),vec3(tx,ty,tz+stepw),vec3(tx,ty+steph,tz+stepw),
                vec2(u,v),vec2(u+stepuv,v),vec2(u+stepuv,v+stepuv),vec3(0,0,1));

            draw_triangle_3d(vec3(tx,ty+steph,tz+stepw),vec3(tx,ty+steph,tz),vec3(tx,ty,tz),
                vec2(u+stepuv,v+stepuv),vec2(u,v+stepuv),vec2(u,v),vec3(0,0,1));
        }
    }
}



// Draw fence
static void draw_fence(CAMERA* cam, float x, float y, float z, float w, float h, float d, int repeat)
{
    bind_texture(bmpFence);
    int i = 0;

    for(; i < repeat; ++ i)
    {
        if(i != 5 && i != 6)
        {
            draw_fence_plane_h(cam,x + i*w,y-h,z,w,h);
            draw_fence_plane_h(cam,x + i*w,y-h,z + d * repeat,w,h);
        }

        draw_fence_plane_d(cam,x,y-h,z + i*d,d,h);
        draw_fence_plane_d(cam,x + w*repeat,y-h,z + i*d,d,h);
    }
}


// Draw a fir
static void draw_fir(float x, float z, float h)
{
    tr_scale_model(4,h,4);
    tr_translate_model(x,4.9f,z);

    bind_texture(bmpFir);
    draw_mesh(mFir);
}


// Draw various models
static void draw_models()
{
    tr_scale_model(5,6,5);
    tr_translate_model(17.5,3.3f,4.0f);

    bind_texture(bmpHouse);
    draw_mesh(mHouse);

    draw_fir(15,-12,6);
    draw_fir(-4,14,5.5f);
}


// Initialize stage
void init_stage(ASSET_PACK* ass)
{
    bmpGrass = (BITMAP*)get_asset(ass,"grass");
    bmpRoad = (BITMAP*)get_asset(ass,"road");
    bmpForest = (BITMAP*)get_asset(ass,"forest");
    bmpMountains = (BITMAP*)get_asset(ass,"mountains");
    bmpMoon = (BITMAP*)get_asset(ass,"moon");
    bmpFence = (BITMAP*)get_asset(ass,"fence");
    bmpHouse = (BITMAP*)get_asset(ass,"house_tex");
    bmpFir = (BITMAP*)get_asset(ass,"fir_tex");

    mHouse = (MESH*)get_asset(ass,"house");
    mFir = (MESH*)get_asset(ass,"fir");
}


// Update stage
void update_stage(PLAYER* pl, float tm)
{

}


// Draw the stage
void draw_stage(CAMERA* cam)
{
    draw_background(cam);

    toggle_darkness(true);
    set_darkness(10.0f,35.0f);

    draw_floor(cam,5);

    draw_triangle_buffer();
    clear_triangle_buffer();

    toggle_darkness(true);
    set_darkness(10.0f,35.0f);

    draw_fence(cam,-25,5,-25,5.0f,5.0f,5.0f,10);

    draw_models();
}