#ifndef CUBES_MAIN_H
#define CUBES_MAIN_H

extern float g_aspect;
extern int g_windowWidth;
extern int g_windowHeight;

extern float g_time;

extern int g_glUniformAlignment;
extern int g_paused;

void setSoundtrack(const char *file);
void setWindowTitle(const char *title);
void presentWindow();

#endif
