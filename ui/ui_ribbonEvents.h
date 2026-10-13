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
void loadFileAtPath(const char *path);
int savefile(void); // 1 = saved
int saveas(void);   // 1 = saved
const char *currentFilepath(void); // NULL until the file is saved/opened
#endif //ATR_UI_RIBBONEVENTS_H
