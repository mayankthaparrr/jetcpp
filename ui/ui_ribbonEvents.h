//
// Created by Mayank Thapar on 10-09-2026.
//

#ifndef ATR_UI_RIBBONEVENTS_H
#define ATR_UI_RIBBONEVENTS_H
extern char *filename;
extern int quitRequested; // set by File -> Quit; checked in the main loop
void newfile(void);
void selectall(void);
void openfile(void);
void savefile(void);
void saveas(void);
#endif //ATR_UI_RIBBONEVENTS_H
