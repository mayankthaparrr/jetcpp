//this is ui_messages.h
#ifndef ATR_UI_MESSAGES_H
#define ATR_UI_MESSAGES_H
#include "raylib.h"

void messagesClear(void);
void messagesLine(const char *text);            // add one line
void messagesAppend(const char *text, int len); // raw compiler output, split on '\n'
void messagesShow(int show);
int messagesOpen(void);
int messagesHovered(void); // mouse is over the pane (the pane gets the wheel then)
void drawMessages(Font font, float fontsize);

#endif
