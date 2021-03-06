/// Stage (source)
/// (c) 2018 Jani Nykänen

#include "stage.h"

#include "decoration.h"

#include "../engine/graphics.h"
#include "../engine/transform.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"

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

// Maximum amount of decorations
#define DEC_MAX 32
// Decorations
static DECORATION decorations[DEC_MAX];
// Decoration count
static int decCount;

// Has the apocalypse begun
static bool apocalypse;
// Sky darkening timer
static float skyDarkTimer;
// Fence height
static float fenceHeight;

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

    // Darken the sky
    if(skyDarkTimer > 0.0f)
    {
        if(skyDarkTimer < 120.0f)
        {
            int amount = (int)floor(skyDarkTimer / 120.0f * MAX_DARKNESS_VALUE);
            darken_frame(amount);
        }
        else
        {
            clear_frame(0);
        }
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
    if(h < 0.0f) return;

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


// Draw various models
static void draw_models()
{
    int i = 0;
    for(; i < decCount; ++ i)
    {
        draw_decoration(&decorations[i]);
    }
}


// Initialize stage
int init_stage(ASSET_PACK* ass)
{
    bmpGrass = (BITMAP*)get_asset(ass,"grass");
    bmpRoad = (BITMAP*)get_asset(ass,"road");
    bmpForest = (BITMAP*)get_asset(ass,"forest");
    bmpMountains = (BITMAP*)get_asset(ass,"mountains");
    bmpMoon = (BITMAP*)get_asset(ass,"moon");
    bmpFence = (BITMAP*)get_asset(ass,"fence");

    // Read layout file
    WORDDATA* layout = parse_file("assets/layout.txt");
    if(layout == NULL)
        return 1;

    decCount = read_decoration_from_layout(ass,layout,decorations);

    apocalypse = false;
    fenceHeight = 5.0f;

    return 0;
}


// Stage-player collision
void stage_player_collision(PLAYER* pl, float tm)
{
    if(apocalypse) return;

    player_decoration_collision(pl,decorations,decCount);
    pl_fence_collision(pl,-1,-25,-25,25,25,0.0f,10.0f,tm);
}


// Update stage
void update_stage(float tm)
{
    const float ULIMIT = -20.0f;

    if(!apocalypse) return;

    if(decCount == 0)
    {
        if(skyDarkTimer < 120.0f)
        {
            skyDarkTimer += 1.0f * tm;
        }
        else if(fenceHeight > 0.0f)
        {
            fenceHeight -= 0.02f *tm;
        }

        return;
    }

    float speed = 0.0f;
    int i = 0;
    int above = 0;
    for(; i < decCount; ++ i)
    {
        speed = 0.1f + 0.01f * i;
        decorations[i].pos.y -= speed * tm;
        if(decorations[i].pos.y < ULIMIT)
        {
            ++ above;
        }
    }
    if(above >= decCount)
    {
        decCount = 0;
    }
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

    draw_fence(cam,-25,5,-25,5.0f,fenceHeight,5.0f,10);

    draw_models();
}


// Start the apocalypse 
void end_stage()
{
    apocalypse = true;
}

// Has the stage ended & are the decorations gone
bool world_ended()
{
    return decCount == 0 && apocalypse;
}