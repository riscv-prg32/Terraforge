#include "prg32.h"
#include "../assets/include/terraforge_assets.h"
#include "../assets/include/terraforge_sound.h"
#include "../assets/include/terraforge_music.h"

#define WORLD_W PRG32_PLAYFIELD_COLS
#define WORLD_H PRG32_PLAYFIELD_ROWS
#define TILE_AIR 0
#define TILE_SKY 1
#define TILE_CLOUD 2
#define TILE_GRASS 3
#define TILE_DIRT 4
#define TILE_STONE 5
#define TILE_ORE 6
#define TILE_WOOD 7
#define TILE_LEAF 8
#define TILE_SAND 9
#define TILE_WATER 10
#define TILE_LAVA 11
#define TILE_CRYSTAL 12
#define ANGLES 32
#define RAYS 80
#define COL_W 4
#define MAX_BOTS 6
#define MAX_PARTICLES 32
#define PLAYER_R 42

typedef struct { int x,y,vx,vy,life; uint16_t color; } particle_t;
typedef struct { int x,y,dir,alive,phase; } bot_t;
typedef struct { int dist,tx,ty,last_tx,last_ty; uint8_t tile,side; } rayhit_t;

static uint8_t world[WORLD_H][WORLD_W];
static uint8_t floor_tile[WORLD_H][WORLD_W];
static uint16_t inv[13];
static uint8_t selected;
static uint16_t health, relics;
static uint32_t frame_no, last_input;
static uint32_t game_state;
static uint16_t day_q8;
static uint16_t music_step, music_timer;
static int player_x, player_y, player_angle;
static particle_t particles[MAX_PARTICLES];
static bot_t bots[MAX_BOTS];
static rayhit_t center_hit;
static uint32_t rng_state = 0x32c0ffeeu;

/* 32 directions, fixed point: cos/sin scaled by 256.  Angle 0 is east. */
static const int16_t cos32[ANGLES] = {256,251,237,213,181,142,98,50,0,-50,-98,-142,-181,-213,-237,-251,-256,-251,-237,-213,-181,-142,-98,-50,0,50,98,142,181,213,237,251};
static const int16_t sin32[ANGLES] = {0,50,98,142,181,213,237,251,256,251,237,213,181,142,98,50,0,-50,-98,-142,-181,-213,-237,-251,-256,-251,-237,-213,-181,-142,-98,-50};

static uint32_t rnd(void){ rng_state = rng_state * 1664525u + 1013904223u; return rng_state; }
static int clamp_i(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }
static int abs_i(int v){ return v<0 ? -v : v; }
static int wrap_angle(int a){ while(a<0) a+=ANGLES; while(a>=ANGLES) a-=ANGLES; return a; }
static int cell_solid(int tx,int ty){ if(tx<0||ty<0||tx>=WORLD_W||ty>=WORLD_H) return 1; return world[ty][tx]!=TILE_AIR; }
static int passable_q8(int x,int y){
    int tx=x>>8, ty=y>>8;
    return !cell_solid((x-PLAYER_R)>>8,(y-PLAYER_R)>>8) && !cell_solid((x+PLAYER_R)>>8,(y-PLAYER_R)>>8) && !cell_solid((x-PLAYER_R)>>8,(y+PLAYER_R)>>8) && !cell_solid((x+PLAYER_R)>>8,(y+PLAYER_R)>>8) && tx>=1 && ty>=1 && tx<WORLD_W-1 && ty<WORLD_H-1;
}

static uint16_t shade_for(uint8_t tile,uint8_t side,int dist){
    uint16_t near = tf_tile_fg[tile < TF_TILE_COUNT ? tile : 0];
    uint16_t far = tf_tile_bg[tile < TF_TILE_COUNT ? tile : 0];
    if(tile==TILE_AIR) return TF_COLOR_BLACK;
    if(tile==TILE_GRASS && side) return TF_COLOR_DIRT;
    if(tile==TILE_ORE && dist<900) return (frame_no&8)?TF_COLOR_ORE:TF_COLOR_STONE;
    if(tile==TILE_CRYSTAL && dist<1100) return (frame_no&8)?TF_COLOR_CRYSTAL:TF_COLOR_PURPLE_DARK;
    if(tile==TILE_LAVA) return (frame_no&4)?TF_COLOR_LAVA:TF_COLOR_YELLOW;
    if(dist>1700) return far;
    if(dist>900 || side) return far;
    return near;
}


static void music_restart(void){ music_step=0; music_timer=1; }
static void music_tick(void){
    if(game_state==2) return;
    if(music_timer>0){ music_timer--; return; }
    const tf_stereo_step_t *m=&tf_stereo_score[music_step];
    /* Minimal-runtime playback: alternate the notated stereo voices so the
       score remains audible on PRG32 mono builds.  The separated WAV master is
       the true stereo asset for PRG32 Audio Plus/AUDIO-block builds. */
    uint16_t hz=(m->pan_hint<128)?m->left_hz:m->right_hz;
    if((frame_no&32u) && m->right_hz) hz=m->right_hz;
    if(hz) prg32_audio_beep(hz, 18);
    music_timer=m->frames;
    music_step++; if(music_step>=TF_STEREO_SCORE_LEN) music_step=0;
}

static void define_assets(void){
    for(uint8_t i=0;i<TF_TILE_COUNT;i++) prg32_tile_define(i, tf_tiles[i], tf_tile_fg[i], tf_tile_bg[i]);
}

static int hashed_wall(int x,int y){
    uint32_t h=(uint32_t)(x*7349u) ^ (uint32_t)(y*9151u) ^ (uint32_t)(x*y*37u);
    h ^= h>>7; h *= 1103515245u; h ^= h>>16;
    return (int)(h&15u) < 5;
}
static void carve_rect(int x0,int y0,int x1,int y1){
    for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) if(x>0&&y>0&&x<WORLD_W-1&&y<WORLD_H-1) world[y][x]=TILE_AIR;
}
static void generate_world(void){
    for(int y=0;y<WORLD_H;y++) for(int x=0;x<WORLD_W;x++){
        floor_tile[y][x]=TILE_DIRT;
        if(x==0||y==0||x==WORLD_W-1||y==WORLD_H-1) world[y][x]=TILE_STONE;
        else if(hashed_wall(x,y)) world[y][x]=((x*7+y*11)&31)==0?TILE_ORE:TILE_STONE;
        else world[y][x]=TILE_AIR;
    }
    /* Hand-carved teaching-friendly spaces: hub, shafts, loops, and four crystal rooms. */
    carve_rect(2,2,12,8); carve_rect(10,5,27,7); carve_rect(24,3,30,14); carve_rect(8,13,34,16);
    carve_rect(32,8,47,11); carve_rect(45,4,57,8); carve_rect(39,18,60,21); carve_rect(14,23,49,27);
    for(int y=2;y<29;y+=5) carve_rect(5,y,7,y+3);
    for(int x=8;x<59;x+=8) carve_rect(x,18,x+4,20);
    for(int x=15;x<23;x++) floor_tile[15][x]=TILE_WATER;
    for(int x=42;x<50;x++) floor_tile[20][x]=TILE_LAVA;
    int cx[4]={26,55,46,17}; int cy[4]={4,6,25,25};
    for(int i=0;i<4;i++){ carve_rect(cx[i]-2,cy[i]-2,cx[i]+2,cy[i]+2); world[cy[i]][cx[i]]=TILE_CRYSTAL; }
    for(int y=1;y<WORLD_H-1;y++) for(int x=1;x<WORLD_W-1;x++){
        if(world[y][x]==TILE_STONE && ((x*5+y*13)&63)==0) world[y][x]=TILE_WOOD;
        if(world[y][x]==TILE_STONE && ((x*3+y*19)&47)==0) world[y][x]=TILE_SAND;
    }
}

static void reset_bots(void){
    int bx[MAX_BOTS]={23,37,52,17,49,31}; int by[MAX_BOTS]={14,10,20,25,6,24};
    for(int i=0;i<MAX_BOTS;i++){ bots[i].x=bx[i]<<8; bots[i].y=by[i]<<8; bots[i].dir=(i*5)&31; bots[i].alive=1; bots[i].phase=i*13; }
}
static void reset_player(void){ player_x=5<<8; player_y=5<<8; player_angle=0; }

static void burst_cell(int tx,int ty,uint8_t tile){
    uint16_t c=tf_tile_fg[tile<TF_TILE_COUNT?tile:0];
    int px=(tx<<8)+128, py=(ty<<8)+128;
    for(int i=0;i<8;i++) for(int p=0;p<MAX_PARTICLES;p++) if(particles[p].life<=0){
        particles[p].x=px; particles[p].y=py; particles[p].vx=(int)(rnd()%65)-32; particles[p].vy=(int)(rnd()%65)-32; particles[p].life=12+(int)(rnd()%18); particles[p].color=c; break;
    }
}

static rayhit_t cast_ray(int angle){
    rayhit_t h; h.dist=4096; h.tx=-1; h.ty=-1; h.last_tx=player_x>>8; h.last_ty=player_y>>8; h.tile=TILE_AIR; h.side=0;
    angle=wrap_angle(angle);
    int x=player_x, y=player_y;
    int last_tx=x>>8, last_ty=y>>8;
    int dx=cos32[angle]>>4; int dy=sin32[angle]>>4;
    if(dx==0 && dy==0) dx=1;
    for(int dist=16; dist<4096; dist+=16){
        x+=dx; y+=dy;
        int tx=x>>8, ty=y>>8;
        if(tx!=last_tx || ty!=last_ty){
            if(cell_solid(tx,ty)){
                h.dist=dist; h.tx=tx; h.ty=ty; h.last_tx=last_tx; h.last_ty=last_ty; h.tile=world[ty][tx]; h.side=(tx!=last_tx)?1:0; return h;
            }
            last_tx=tx; last_ty=ty;
        }
    }
    return h;
}

static void break_target(void){
    rayhit_t h=cast_ray(player_angle);
    if(h.tile==TILE_AIR || h.dist>1536) return;
    uint8_t t=h.tile;
    inv[t]++;
    if(t==TILE_CRYSTAL){ relics++; prg32_audio_play_notes(tf_melody_relic, sizeof(tf_melody_relic)/sizeof(tf_melody_relic[0])); }
    world[h.ty][h.tx]=TILE_AIR;
    burst_cell(h.tx,h.ty,t);
    prg32_audio_beep(t==TILE_CRYSTAL?1100:(t==TILE_ORE?880:220), t==TILE_STONE?45:30);
}
static void place_target(void){
    rayhit_t h=cast_ray(player_angle);
    int tx=h.last_tx, ty=h.last_ty;
    if(h.dist>1536){ tx=(player_x + cos32[player_angle]*3)>>8; ty=(player_y + sin32[player_angle]*3)>>8; }
    if(selected==TILE_AIR || selected==TILE_WATER || selected==TILE_LAVA || inv[selected]==0) return;
    if(tx<1||ty<1||tx>=WORLD_W-1||ty>=WORLD_H-1) return;
    if(cell_solid(tx,ty)) return;
    if(((player_x>>8)==tx) && ((player_y>>8)==ty)) return;
    world[ty][tx]=selected; inv[selected]--; burst_cell(tx,ty,selected); prg32_audio_beep(392,28);
}
static void cycle_selected(void){
    for(int k=0;k<12;k++){
        selected++; if(selected>=TF_TILE_COUNT) selected=TILE_GRASS;
        if(inv[selected]>0 && selected!=TILE_WATER && selected!=TILE_LAVA && selected!=TILE_CRYSTAL) return;
    }
    selected=TILE_DIRT;
}

static void try_move(int dx,int dy){
    int nx=player_x+dx, ny=player_y;
    if(passable_q8(nx,ny)) player_x=nx;
    nx=player_x; ny=player_y+dy;
    if(passable_q8(nx,ny)) player_y=ny;
}
static void update_bots(void){
    for(int i=0;i<MAX_BOTS;i++) if(bots[i].alive){
        int dx=cos32[bots[i].dir]>>3, dy=sin32[bots[i].dir]>>3;
        int nx=bots[i].x+dx, ny=bots[i].y+dy;
        if(!cell_solid(nx>>8,ny>>8)){ bots[i].x=nx; bots[i].y=ny; }
        else bots[i].dir=wrap_angle(bots[i].dir + 8 + (int)(rnd()&15));
        if((frame_no + bots[i].phase)%43==0) bots[i].dir=wrap_angle(bots[i].dir + (int)(rnd()%7)-3);
        int ddx=(bots[i].x-player_x)>>8, ddy=(bots[i].y-player_y)>>8;
        if(ddx*ddx+ddy*ddy<2 && (frame_no&15)==0 && health>0){ health--; prg32_audio_beep(90,50); }
    }
}
static void update_particles(void){
    for(int i=0;i<MAX_PARTICLES;i++) if(particles[i].life>0){ particles[i].x+=particles[i].vx; particles[i].y+=particles[i].vy; particles[i].vx=(particles[i].vx*7)/8; particles[i].vy=(particles[i].vy*7)/8; particles[i].life--; }
}
static void floor_effects(void){
    int tx=player_x>>8, ty=player_y>>8;
    uint8_t f=floor_tile[ty][tx];
    if(f==TILE_WATER && (frame_no&31)==0 && health<9){ health++; prg32_audio_beep(520,20); }
    if(f==TILE_LAVA && (frame_no&7)==0 && health>0){ health--; prg32_audio_beep(80,30); }
}

void terraforge32_c_init(void){
    define_assets(); generate_world(); reset_player(); reset_bots();
    selected=TILE_DIRT; health=9; relics=0; frame_no=0; last_input=0; game_state=0; day_q8=0; rng_state=0x32c0ffeeu; music_restart();
    for(int i=0;i<13;i++) inv[i]=0;
    inv[TILE_DIRT]=24; inv[TILE_STONE]=8; inv[TILE_WOOD]=8;
    for(int i=0;i<MAX_PARTICLES;i++) particles[i].life=0;
    prg32_audio_play_notes(tf_melody_start, sizeof(tf_melody_start)/sizeof(tf_melody_start[0]));
}

void terraforge32_c_update(void){
    uint32_t input=prg32_input_read(); uint32_t press=input & ~last_input; last_input=input;
    if(press&PRG32_BTN_START){ terraforge32_c_init(); return; }
    if(game_state){ frame_no++; return; }
    if(input&PRG32_BTN_LEFT) player_angle=wrap_angle(player_angle-1);
    if(input&PRG32_BTN_RIGHT) player_angle=wrap_angle(player_angle+1);
    int speed=(input&PRG32_BTN_B)?22:14;
    if(input&PRG32_BTN_UP){ try_move((cos32[player_angle]*speed)>>8,(sin32[player_angle]*speed)>>8); if((frame_no&31)==0) prg32_audio_beep(180,12); }
    if((input&PRG32_BTN_DOWN) && !(input&PRG32_BTN_A)){ try_move(-(cos32[player_angle]*speed)>>8,-(sin32[player_angle]*speed)>>8); }
    if((press&PRG32_BTN_B) && !(input&PRG32_BTN_UP)) cycle_selected();
    if(press&PRG32_BTN_A){ if(input&PRG32_BTN_DOWN) place_target(); else break_target(); }
    update_bots(); update_particles(); floor_effects(); music_tick(); center_hit=cast_ray(player_angle);
    if(health==0){ game_state=2; prg32_audio_beep(60,250); }
    if(relics>=4){ game_state=1; prg32_audio_play_notes(tf_melody_relic, sizeof(tf_melody_relic)/sizeof(tf_melody_relic[0])); }
    day_q8=(uint16_t)((day_q8+1)&511); frame_no++;
}

static void draw_uint(int x,int y,uint16_t v,uint16_t fg){ char s[6]; s[5]=0; for(int i=4;i>=0;i--){ s[i]=(char)('0'+v%10); v/=10; } prg32_gfx_text8(x,y,s,fg,PRG32_COLOR_BLACK); }
static void draw_crosshair(void){ prg32_gfx_rect(156,99,8,2,PRG32_COLOR_WHITE); prg32_gfx_rect(159,96,2,8,PRG32_COLOR_WHITE); }
static void draw_background(void){
    uint16_t sky=(day_q8<256)?TF_COLOR_SKY:0x1082;
    prg32_gfx_clear(sky);
    prg32_gfx_rect(0,100,320,100,TF_COLOR_DIRT_DARK);
    for(int y=110;y<200;y+=18) prg32_gfx_rect(0,y,320,2,(y&32)?TF_COLOR_STONE_DARK:TF_COLOR_DIRT);
    for(int y=24;y<96;y+=24) prg32_gfx_rect(0,y,320,1,(day_q8<256)?TF_COLOR_WHITE:TF_COLOR_PURPLE_DARK);
}
static void draw_ray_column(int col,rayhit_t h,int angle_offset){
    int corr=h.dist;
    int oa=abs_i(angle_offset); if(oa>0){ corr=(corr*(int)cos32[oa])/256; if(corr<1) corr=1; }
    int wall_h=46080/(corr+16); wall_h=clamp_i(wall_h,6,190);
    int y0=100-wall_h/2, y1=100+wall_h/2; y0=clamp_i(y0,18,199); y1=clamp_i(y1,18,199);
    uint16_t c=shade_for(h.tile,h.side,corr);
    int x=col*COL_W;
    prg32_gfx_rect(x,y0,COL_W,y1-y0,c);
    prg32_gfx_rect(x,y0,COL_W,2,TF_COLOR_WHITE);
    if(h.tile==TILE_WOOD || h.tile==TILE_SAND){ for(int yy=y0+6; yy<y1; yy+=12) prg32_gfx_rect(x,yy,COL_W,1,tf_tile_bg[h.tile]); }
    if(h.tile==TILE_CRYSTAL){ prg32_gfx_rect(x+1,y0+wall_h/4,2,wall_h/2,TF_COLOR_WHITE); }
}
static void draw_3d(void){
    draw_background();
    for(int r=0;r<RAYS;r++){
        int off=((r - RAYS/2)*8)/RAYS;
        int a=wrap_angle(player_angle+off);
        rayhit_t h=cast_ray(a);
        draw_ray_column(r,h,off);
    }
}
static void draw_bot_3d(void){
    int ca=cos32[player_angle], sa=sin32[player_angle];
    for(int i=0;i<MAX_BOTS;i++) if(bots[i].alive){
        int dx=bots[i].x-player_x, dy=bots[i].y-player_y;
        int forward=(dx*ca + dy*sa)>>8;
        int side=(dx*(-sa) + dy*ca)>>8;
        if(forward>120 && forward<2800 && abs_i(side)*2<forward*3){
            int sx=160 + (side*180)/forward;
            int sz=clamp_i(24000/(forward+1),8,58);
            int y=104-sz/2;
            prg32_gfx_rect(sx-sz/3,y,sz*2/3,sz,TF_COLOR_PURPLE_DARK);
            prg32_gfx_rect(sx-sz/6,y+sz/4,sz/8,sz/8,TF_COLOR_CRYSTAL);
            prg32_gfx_rect(sx+sz/8,y+sz/4,sz/8,sz/8,TF_COLOR_CRYSTAL);
        }
    }
}
static void draw_particles_3d(void){
    int ca=cos32[player_angle], sa=sin32[player_angle];
    for(int i=0;i<MAX_PARTICLES;i++) if(particles[i].life>0){
        int dx=particles[i].x-player_x, dy=particles[i].y-player_y;
        int forward=(dx*ca + dy*sa)>>8;
        int side=(dx*(-sa) + dy*ca)>>8;
        if(forward>60 && forward<1800 && abs_i(side)*2<forward*3){
            int sx=160 + (side*170)/forward; int sy=100 + ((particles[i].life&7)-3)*160/(forward/32+1);
            int sz=clamp_i(800/(forward/64+1),1,5); prg32_gfx_rect(sx,sy,sz,sz,particles[i].color);
        }
    }
}
static void draw_minimap(void){
    int ox=4, oy=126;
    prg32_gfx_rect(0,122,138,78,PRG32_COLOR_BLACK);
    for(int y=0;y<WORLD_H;y++) for(int x=0;x<WORLD_W;x++){
        uint16_t c=TF_COLOR_BLACK;
        if(world[y][x]) c=shade_for(world[y][x],0,2000);
        else if(floor_tile[y][x]==TILE_WATER) c=TF_COLOR_WATER;
        else if(floor_tile[y][x]==TILE_LAVA) c=TF_COLOR_LAVA;
        else continue;
        prg32_gfx_rect(ox+x*2,oy+y*2,2,2,c);
    }
    for(int i=0;i<MAX_BOTS;i++) if(bots[i].alive) prg32_gfx_rect(ox+(bots[i].x>>7)-1,oy+(bots[i].y>>7)-1,3,3,TF_COLOR_PURPLE_DARK);
    int px=ox+(player_x>>7), py=oy+(player_y>>7); prg32_gfx_rect(px-2,py-2,5,5,PRG32_COLOR_WHITE);
    prg32_gfx_rect(px,py,(cos32[player_angle]>>7),1,PRG32_COLOR_YELLOW); prg32_gfx_rect(px,py,1,(sin32[player_angle]>>7),PRG32_COLOR_YELLOW);
}
static void draw_hud(void){
    prg32_gfx_rect(0,0,320,18,PRG32_COLOR_BLACK);
    prg32_gfx_text8(4,5,"TERRAFORGE32 3D",PRG32_COLOR_WHITE,PRG32_COLOR_BLACK);
    prg32_gfx_text8(126,5,"HP",PRG32_COLOR_WHITE,PRG32_COLOR_BLACK); for(int i=0;i<health;i++) prg32_gfx_rect(148+i*5,7,4,6,PRG32_COLOR_RED);
    prg32_gfx_text8(210,5,"SEL",PRG32_COLOR_YELLOW,PRG32_COLOR_BLACK); prg32_sprite_draw_8x8(242,5,tf_tiles[selected],tf_tile_fg[selected],tf_tile_bg[selected]); draw_uint(254,5,inv[selected],PRG32_COLOR_YELLOW);
    prg32_gfx_text8(292,5,"R",PRG32_COLOR_CYAN,PRG32_COLOR_BLACK); draw_uint(300,5,relics,PRG32_COLOR_CYAN);
    draw_crosshair();
    if(center_hit.tile!=TILE_AIR && center_hit.dist<1536){ prg32_gfx_text8(126,184,"A DIG",PRG32_COLOR_WHITE,PRG32_COLOR_BLACK); }
    else { prg32_gfx_text8(112,184,"DOWN+A PLACE",PRG32_COLOR_WHITE,PRG32_COLOR_BLACK); }
}
static void draw_end(void){
    if(game_state==1){ prg32_gfx_rect(52,74,216,48,PRG32_COLOR_BLACK); prg32_gfx_text8(72,84,"ALL FOUR RELICS FORGED!",PRG32_COLOR_YELLOW,PRG32_COLOR_BLACK); prg32_gfx_text8(78,104,"PRESS START TO RESEED",PRG32_COLOR_WHITE,PRG32_COLOR_BLACK); }
    if(game_state==2){ prg32_gfx_rect(70,74,180,48,PRG32_COLOR_BLACK); prg32_gfx_text8(104,84,"CORE COLLAPSE",PRG32_COLOR_RED,PRG32_COLOR_BLACK); prg32_gfx_text8(80,104,"PRESS START TO REBUILD",PRG32_COLOR_WHITE,PRG32_COLOR_BLACK); }
}

void terraforge32_c_draw(void){
    draw_3d(); draw_particles_3d(); draw_bot_3d(); draw_minimap(); draw_hud();
    prg32_gfx_text8(144,188,"UP/DOWN MOVE  LEFT/RIGHT TURN  B RUN/SELECT",PRG32_COLOR_WHITE,PRG32_COLOR_BLACK);
    draw_end();
}
