#include <stdio.h>
#include "ui_ribbonEvents.h"
#include "app_spawn/app_spawn.h"
#include <stdlib.h>
#include <string.h>
#include "raylib.h"

void handleRibbonHover();
void handleRibbonEvents();
typedef struct {
    char first_letter;
    char *remains;
    char *name;
    Rectangle rect;
    float x;
    float y;
    float posx;
    float posy;
} ribbonItems;

ribbonItems items[7]={
    { 'F',"ile"},
    {'E',"dit"},
    {'S',"earch"},
    {'R',"un"},
    {'P',"roject"},
    {'O',"ptions"},
    {'H',"elp"}
};


typedef struct {
    char *name;
    int highlight;
}ribbonOptions;

ribbonOptions file[5]={
    {"New",0},
    {"Open",0},
    {"Save",0},
    {"Save As",0},
    {"Quit",0}
};

ribbonOptions edit[7]={
    {"Undo",0},
    {"Redo",0},
    {"Cut",0},
    {"Copy",0},
    {"Paste",0},
    {"Clear",0},
    {"Select All",0}
};
ribbonOptions search[4]={
    {"Find",0},
    {"Find Next",0},
    {"Replace",0},
    {"Go To Line",0}
};

ribbonOptions run[4]={
    {"Compile",0},
    {"Run",0},
    {"Arguments",0},
    {"Stop",0}
};

ribbonOptions project[5]={
    {"Open Project",0},
    {"Close Project",0},
    {"Add Item",0},
    {"Delete Item",0},
    {"Include Files",0}
};

ribbonOptions options[6]={
    {"Directories",0},
    {"Compiler",0},
    {"Linker",0},
    {"Debugger",0},
    {"Environment",0},
    {"Save",0}
};

ribbonOptions help[1]={
    {"About",0}
};
Vector2 mouse;




void drawMenu(ribbonOptions option[],int i,Color color, Font usedfont,float fontsize,int posx2) {
    int height=0;
    int width=0;
    for (int n=0;n<i;n++) {
        height+=MeasureTextEx(usedfont,option[n].name,fontsize,0).y*1.5;
        if (width<MeasureTextEx(usedfont,option[n].name,fontsize,0).x) {
            width=MeasureTextEx(usedfont,option[n].name,fontsize,0).x;
        }
    }
    width*=1.1;

    // Background height must be measured from THIS menu's items, not the
    // (unrelated) `options` array — indexing options[i] by item count used
    // to read out of bounds for the larger menus.
    DrawRectangleRounded((Rectangle){posx2,MeasureTextEx(usedfont,option[0].name,fontsize,0).y-15,width,height+15},0.1,1,color);
    float sposx=posx2*1.01;
    float nposx=sposx+(width*0.90);

    height=fontsize*1.3; width=width*0.1;
    for (int n=0;n<i;n++) {
        float posy=height-(fontsize*0.29);
         mouse = GetMousePosition();
        Rectangle rect={posx2+2,height,(width*10)*0.9998,MeasureTextEx(usedfont,option[n].name,fontsize,0).y};
        if(CheckCollisionPointRec(mouse, rect)){
            handleRibbonEvents((Rectangle){rect.x,rect.y,width*10,rect.height},option[n].name);
            DrawRectangle(rect.x,rect.y,width*10,rect.height,LIGHTGRAY);
        }
        // DrawLine(sposx,posy,nposx,posy,DARKGRAY);
        DrawTextEx(usedfont,option[n].name,(Vector2){posx2+5,height},fontsize,0,BLACK);


        height+=MeasureTextEx(usedfont,option[n].name,fontsize,0).y*1.5;




    }


}

void selectMenu(int i, Color color, Font usedfont, float fontsize) {
    int j;
    switch (i) {
            case 1:j=5;drawMenu(file,j,color,usedfont,fontsize,items[0].x);break;//file
            case 2:j=7;drawMenu(edit,j,color,usedfont,fontsize,items[1].x);break;//edit
            case 3:j=4;drawMenu(search,j,color,usedfont,fontsize,items[2].x);break;//search
            case 4:j=4;drawMenu(run,j,color,usedfont,fontsize,items[3].x);break;//run
            case 5:j=5;drawMenu(project,j,color,usedfont,fontsize,items[4].x);break;//project
            case 6:j=6;drawMenu(options,j,color,usedfont,fontsize,items[5].x);break;//options
            case 7:j=1;drawMenu(help,j,color,usedfont,fontsize,items[6].x);break;//help
    }
}

static int activeMenu=-1;

void handleRibbonHover(void) {
    if (activeMenu == -1) {
        return;
    }


    Vector2 mouse = GetMousePosition();
    int overRibbon=0;

    for (int i = 0; i < 7; i++) {
        if (CheckCollisionPointRec(mouse, items[i].rect)) {
            activeMenu = i + 1;
            return;
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)&&!overRibbon) {
        activeMenu = -1;
    }
}


void drawRibbon(Color color, Font usedfont, float fontsize) {
    float sumx;
    // draw currently active dropdown
    if (activeMenu != -1) {selectMenu(activeMenu, color, usedfont, fontsize);}
    sumx = (GetScreenWidth() - sumx - 10) / 16;
    int posx = (GetScreenWidth() * 0.01);
    DrawRectangle(0, 0, GetScreenWidth(), fontsize * 1.05, color);
    int posx2 = posx;

    for (int i = 0; i < 7; i++) {
        char c[2] = {items[i].first_letter, '\0'};
        int width = MeasureTextEx(usedfont, c, fontsize, 0).x;
        width += MeasureTextEx(usedfont, items[i].remains, fontsize, 0).x;
        items[i].rect = (Rectangle){posx2, 0, width + 4, fontsize};
        items[i].x = posx2;
        posx2 += width + 15;

        // click handling stays here
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {

            if (CheckCollisionPointRec(GetMousePosition(),items[i].rect)) {

                if (activeMenu == i + 1) {
                    activeMenu = -1;
                }
                else {
                    activeMenu = i + 1;
                }
            }
        }

        // drawing...
        if (activeMenu == i + 1 || CheckCollisionPointRec(GetMousePosition(),items[i].rect)) {
            DrawRectangleRounded(items[i].rect,0.2,8,LIGHTGRAY);
        }
        DrawTextEx(usedfont,c,(Vector2){posx, 1},fontsize,0,RED);
        posx += MeasureTextEx(usedfont, c, fontsize, 0).x;
        DrawTextEx(usedfont,items[i].remains,(Vector2){posx, 1},fontsize,0,BLACK);
        posx += MeasureTextEx(usedfont, items[i].remains, fontsize, 0).x;
        posx += 15;
    }
    // handle hover AFTER all rectangles exist
    handleRibbonHover();


    Rectangle bottomRibbon = {0,GetScreenHeight()-(fontsize),GetScreenWidth(),fontsize};
    DrawRectangleRec(bottomRibbon,color);
}
void handleRibbonEvents(Rectangle rect,char *option) {

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (CheckCollisionPointRec(GetMousePosition(),rect)) {

            if (strcmp(option,"New") == 0) {
                // "New" opens a new window: a separate process with a fresh
                // document. Fall back to clearing this document if the
                // process couldn't be spawned.
                if (!spawnNewInstance()) {
                    newfile();
                }
            }

            if (strcmp(option,"Open") == 0) {
                openfile();
            }

            if (strcmp(option,"Save") == 0) {
                savefile();
            }

            if (strcmp(option,"Save As") == 0) {
                saveas();
            }

            if (strcmp(option,"Quit") == 0) {
                // raylib 5.5 has no SetWindowShouldClose(), so flag the
                // request and let the main loop exit cleanly after the frame.
                quitRequested = 1;
            }

            if (strcmp(option,"Select All") == 0) {
                selectall();
            }

            if (strcmp(option,"About") == 0) {
                // Pop the project README open in a new editor window.
                // Falls back to showing it in this window if spawn fails.
                activeMenu = -1;
                if (!spawnNewInstanceWithFile("C:/DEV/ATR/README.md")) {
                    loadFileAtPath("C:/DEV/ATR/README.md");
                }
            }
        }
    }
}
