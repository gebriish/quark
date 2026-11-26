#ifndef EDITOR_CONFIG_H
#define EDITOR_CONFIG_H

typedef u32 Color_Kind;
enum
{
  Color_Kind_Normal,
  Color_Kind_COUNT,
};

typedef struct Color_Field Color_Field;
struct Color_Field
{
  Vec4F32 fg;
  Vec4F32 bg;
};

typedef struct Config Config;
struct Config
{
  Fnt_Handle code_font;
  Fnt_Handle ui_font;
  u32 code_font_size;
  u32 ui_font_size;
  Color_Field colors[Color_Kind_COUNT];
};

function Color_Field get_color(Config *config, Color_Kind kind);

#endif // EDITOR_CONFIG_H 
