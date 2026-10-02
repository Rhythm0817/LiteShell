#ifndef SCREEN_H
#define SCREEN_H

void screen_render_header(void);
void screen_render_footer(void);
void screen_render_prompt(void);
void screen_draw_separator(char ch);
int screen_is_wide_mode(void);

#endif
