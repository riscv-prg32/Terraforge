#ifndef PRG32_H
#define PRG32_H
#include <stdint.h>
#include <stddef.h>
#define PRG32_PLAYFIELD_COLS 64
#define PRG32_PLAYFIELD_ROWS 32
#define PRG32_TILE_W 8
#define PRG32_TILE_H 8
#define PRG32_GAME_W 320
#define PRG32_GAME_H 200
#define PRG32_BTN_LEFT 1u
#define PRG32_BTN_RIGHT 2u
#define PRG32_BTN_UP 4u
#define PRG32_BTN_DOWN 8u
#define PRG32_BTN_A 16u
#define PRG32_BTN_B 32u
#define PRG32_BTN_START 64u
#define PRG32_COLOR_BLACK 0x0000
#define PRG32_COLOR_WHITE 0xffff
#define PRG32_COLOR_RED 0xf800
#define PRG32_COLOR_YELLOW 0xffe0
#define PRG32_COLOR_CYAN 0x07ff
#define PRG32_TILE_FLAG_SOLID 1u
#define PRG32_TILE_FLAG_PLATFORM 2u
#define PRG32_TILE_FLAG_HAZARD 4u
#define PRG32_TILE_FLAG_COLLECT 8u
#define PRG32_PLATFORM_ON_GROUND 1u
typedef struct { int x,y,vx,vy; uint16_t w,h,state; uint8_t layer,reserved; } prg32_platform_actor_t;
typedef struct { uint16_t frequency_hz; uint16_t duration_ms; } prg32_note_t;
uint32_t prg32_input_read(void); uint32_t prg32_ticks_ms(void); void prg32_audio_beep(uint32_t,uint32_t); void prg32_audio_play_notes(const prg32_note_t*, size_t);
void prg32_tile_define(uint8_t,const uint8_t*,uint16_t,uint16_t); void prg32_platform_tile_flags(uint8_t,uint8_t); void prg32_playfield_put(uint8_t,uint8_t,uint8_t,uint8_t); void prg32_playfield_clear(uint8_t,uint8_t); void prg32_playfield_parallax(uint8_t,int,int); void prg32_playfield_camera(int,int); int prg32_playfield_camera_x(void); int prg32_playfield_camera_y(void); void prg32_platform_actor_init(prg32_platform_actor_t*,uint8_t,int,int,int,int); uint16_t prg32_platform_actor_step(prg32_platform_actor_t*,uint32_t,int,int,int,int); void prg32_platform_camera_follow(const prg32_platform_actor_t*,int,int); int prg32_platform_solid_at(uint8_t,int,int); int prg32_sprite_hitbox(int,int,int,int,int,int,int,int); uint32_t prg32_sprite_anim_frame(uint32_t,uint32_t,uint32_t); void prg32_sprite_draw_frame(int,int,int,int,const uint16_t*,uint32_t,uint16_t); void prg32_sprite_draw_8x8(int,int,const uint8_t*,uint16_t,uint16_t); void prg32_gfx_clear(uint16_t); void prg32_playfield_draw(uint8_t,int); void prg32_gfx_rect(int,int,int,int,uint16_t); void prg32_gfx_text8(int,int,const char*,uint16_t,uint16_t);
#endif
