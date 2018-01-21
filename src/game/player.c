/// Player (source)
/// (c) 2018 Jani Nykänen

#include "player.h"

#include "../engine/graphics.h"
#include "../engine/transform.h"
#include "../engine/mathext.h"

#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "time.h"

#include "../vpad.h"

// Fish mesh
static MESH* mFish;
// Fish texture
static BITMAP* bmpFish;


// Limit player area
static void pl_limit(PLAYER* pl)
{
    const float UPPER_LIMIT = 9.0f;
    const float LOWER_LIMIT = -4.0f;

    if(pl->speed.y < 0.0f && pl->pos.y < -UPPER_LIMIT)
    {
        pl->pos.y = -UPPER_LIMIT;
        pl->angleTarget.x = 0.0f;
        pl->speed.y = 0.0f;
    }
    else if(pl->speed.y > 0.0f && pl->pos.y > -LOWER_LIMIT)
    {
        pl->pos.y = -LOWER_LIMIT;
        pl->angleTarget.x = 0.0f;
        pl->speed.y = 0.0f;
    }

}


// Speed delta
static void pl_speed_delta(float* speed, float target, float acc, float tm)
{
    if(target > *speed)
    {
        *speed += acc * tm;
        if(*speed > target)
            *speed = target;
    }
    else if(target < *speed)
    {
        *speed -= acc * tm;
        if(*speed < target)
            *speed = target;
    }
}


// Limit angle
static void pl_limit_angle(PLAYER* pl)
{
    if(pl->angle.x > M_PI/4)
    {
        pl->angle.x = M_PI/4;
        pl->angleSpeed.x = 0.0f;
    }
    else if(pl->angle.x < -M_PI/4)
    {
        pl->angle.x = -M_PI/4;
        pl->angleSpeed.x = 0.0f;
    }

    if(pl->angle.z > M_PI/4)
    {
        pl->angle.z = M_PI/4;
        pl->angleSpeed.z = 0.0f;
    }
    else if(pl->angle.z < -M_PI/4)
    {
        pl->angle.z = -M_PI/4;
        pl->angleSpeed.z = 0.0f;
    }
}


// Player controls
static void pl_control(PLAYER* pl)
{
    pl->angleTarget = vec3(0,0,0);
    pl->target.y = 0.0f;

    VEC2 stick = vpad_get_stick();
    if(fabs(stick.x)  > 0.1f)
    {
        pl->angleTarget.y = stick.x * pl->angleMax.y;
        pl->angleTarget.z = stick.x * pl->angleMax.z;
    }
    else if(fabs(pl->angle.z) > 0.001f)
    {
        pl->angleTarget.z = -2* (pl->angle.z / (M_PI/2))* pl->angleMax.z;
    }
    
    if(fabs(stick.y)  > 0.1f)
    {
        pl->angleTarget.x = stick.y * pl->angleMax.x;
        pl->target.y = pl->maxSpeed.y * stick.y;
    }
    else if(fabs(pl->angle.x) > 0.001f)
    {
        pl->angleTarget.x = -2* (pl->angle.x / (M_PI/2))* pl->angleMax.x;
    }

    pl_limit_angle(pl);

    pl->target.x = sin(pl->angle.y) * pl->maxSpeed.x;
    pl->target.z = cos(pl->angle.y) * pl->maxSpeed.z;
}


// Rotate
static void pl_rotate(PLAYER* pl, float tm)
{
    pl_speed_delta(&pl->angleSpeed.x,pl->angleTarget.x,pl->angleAcc.x,tm);
    pl_speed_delta(&pl->angleSpeed.y,pl->angleTarget.y,pl->angleAcc.y,tm);
    pl_speed_delta(&pl->angleSpeed.z,pl->angleTarget.z,pl->angleAcc.z,tm);

    pl->angle.x += pl->angleSpeed.x * tm;
    pl->angle.y += pl->angleSpeed.y * tm;
    pl->angle.z += pl->angleSpeed.z * tm;
}


// Player movement
static void pl_move(PLAYER* pl, float tm)
{
    pl_speed_delta(&pl->speed.x,pl->target.x,pl->acc.x,tm);
    pl_speed_delta(&pl->speed.y,pl->target.y,pl->acc.y,tm);
    pl_speed_delta(&pl->speed.z,pl->target.z,pl->acc.z,tm);

    pl->pos.x += pl->speed.x * tm;
    pl->pos.y += pl->speed.y * tm;
    pl->pos.z += pl->speed.z * tm;
}


// Player-triangle collision
static void pl_triangle_collision(PLAYER* pl, VEC3 A, VEC3 B, VEC3 C, VEC3 N)
{
    if(!(inside_triangle(pl->pos.x,pl->pos.y,A.x,A.y,B.x,B.y,C.x,C.y)
    || inside_triangle(pl->pos.x,pl->pos.z,A.x,A.z,B.x,B.z,C.x,C.z)
    || inside_triangle(pl->pos.y,pl->pos.z,A.y,A.z,B.y,B.z,C.y,C.z)))
    {
        return;
    }

    float d = -(N.x * A.x + N.y * A.y + N.z * A.z);
    float dist = fabs(pl->pos.x * N.x + pl->pos.y * N.y + pl->pos.z * N.z + d) 
            / sqrt( N.x*N.x + N.y*N.y + N.z*N.z);

    if(dist < pl->radius)
    {
        pl->pos.x -= N.x * (dist-pl->radius);
        pl->pos.y -= N.y * (dist-pl->radius);
        pl->pos.z -= N.z * (dist-pl->radius);
    }
}


// Initialize
void init_player(ASSET_PACK* ass)
{
    bmpFish = get_asset(ass,"fish_tex");
    mFish = get_asset(ass,"fish");
}


// Create
PLAYER pl_create(VEC3 pos)
{
    const float MAX_SPEED = 0.1f;
    const float MAX_ANGLE_Y = 0.025f;
    const float MAX_ANGLE_XZ = 0.05f;
    const float ACC = 0.01f;
    const float ANGLE_ACC = 0.05f;

    PLAYER pl;
    pl.pos = pos;
    pl.speed = vec3(0,0,0);
    pl.target = pl.speed;

    pl.radius = 0.5f;

    pl.maxSpeed = vec3(MAX_SPEED,MAX_SPEED,MAX_SPEED);
    pl.angleMax = vec3(MAX_ANGLE_XZ,MAX_ANGLE_Y,MAX_ANGLE_XZ);

    pl.angleAcc = vec3(ANGLE_ACC,ANGLE_ACC,ANGLE_ACC);
    pl.acc = vec3(ACC,ACC,ACC);

    pl.angle = vec3(0,0,0);
    pl.angleSpeed = vec3(0,0,0);
    pl.angleTarget = vec3(0,0,0);

    return pl;
}


// Update
void pl_update(PLAYER* pl, float tm)
{
    pl_control(pl);
    pl_limit(pl);
    pl_move(pl,tm);
    pl_rotate(pl,tm);
}


// Draw
void pl_draw(PLAYER* pl)
{
    tr_translate_model(pl->pos.x,pl->pos.y,pl->pos.z);
    tr_rotate_model(-M_PI/2+pl->angle.x,M_PI+pl->angle.y,M_PI/2); // - pl->angle.z);
    tr_scale_model(1.0f,1.0f,1.0f);

    bind_texture(bmpFish);
    draw_mesh(mFish);
}


// Player-mesh collision
void pl_mesh_collision(PLAYER* pl, MESH* m, VEC3 tr, VEC3 sc)
{
    if(pl == NULL || m == NULL) return;

    VEC3 A;
    VEC3 B;
    VEC3 C;
    VEC3 N;

    int i = 0;
    for(; i < m->elementCount; i += 3)
    {
        A = vec3(m->vertices[m->indices[i]*3],m->vertices[m->indices[i]*3 +1],m->vertices[m->indices[i]*3+2]);
        B = vec3(m->vertices[m->indices[i]*3 +3],m->vertices[m->indices[i]*3 +4],m->vertices[m->indices[i]*3+5]);
        C = vec3(m->vertices[m->indices[i]*3 +6],m->vertices[m->indices[i]*3 +7],m->vertices[m->indices[i]*3+8]);

        N = vec3(m->normals[m->indices[i]*3],m->normals[m->indices[i]*3 +1],m->normals[m->indices[i]*3+2]);

        A.x *= sc.x; A.x += tr.x;
        A.y *= sc.y; A.y += tr.y;
        A.z *= sc.z; A.z += tr.z;

        B.x *= sc.x; B.x += tr.x;
        B.y *= sc.y; B.y += tr.y;
        B.z *= sc.z; B.z += tr.z;

        C.x *= sc.x; C.x += tr.x;
        C.y *= sc.y; C.y += tr.y;
        C.z *= sc.z; C.z += tr.z;

        pl_triangle_collision(pl,A,B,C,N);
    }
}