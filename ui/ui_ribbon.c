#include <stdio.h>
#include "ui_ribbonEvents.h"
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

ribbonItems items[10]={
    { 'F',"ile"},
    {'E',"dit"},
    {'S',"earch"},
    {'R',"un"},
    {'C',"ompile"},
    {'D',"ebug"},
    {'P',"roject"},
    {'O',"ptions"},
    {'W',"indow"},
    {'H',"elp"}
};


typedef struct {
    char *name;
    int highlight;
}ribbonOptions;

ribbonOptions file[8]={
    {"New",0},
    {"Open",0},
    {"Save",0},
    {"Save As",0},
    {"Save All",0},
    {"Change Directory",0},
    {"Print",0},
    {"Quit",0}
};

ribbonOptions edit[7]={
    {"Undo",0},
    {"Redo",0},
    {"Cut",0},
    {"Copy",0},
    {"Paste",0},
    {"Clear",0},
    {"Show Clipboard",0}
};
ribbonOptions search[7]={
    {"Find",0},
    {"Replace",0},
{"Search again",0},
    {"Go To Line Number",0},
    {"Previous Error",0},
    {"Next Error",0},
    {"Locate Error",0}
};

ribbonOptions run[6]={
    {"Run",0},
    {"Program reset",0},
    {"Go to Cursor",0},
    {"Trace into",0},
    {"Step over",0},
    {"Arguements",0}
};

ribbonOptions compile[6] = {
    {"Compile",0},
    {"Make",0},
    {"Link",0},
    {"Build All",0},
    {"Information",0},
    {"Remove Messages",0}
};
ribbonOptions debug[6] = {
    {"Inspect",0},
    {"Evaluate/Modify",0},
    {"Call Stack",0},
    {"Watches",0},
    {"Toggle Breakpoint",0},
    {"Breakpoints..",0}
};

ribbonOptions project[6]={
    {"Open Project",0},
    {"Close Project",0},
    {"Add Item",0},
    {"Delete Item",0},
    {"Local Options",0},
    {"Include Files",0}
};
ribbonOptions options[10]={
    {"Application",0},
    {"Compiler",0},
    {"Transfer",0},
    {"Make",0},
    {"Linker",0},
    {"Librarian",0},
    {"Debugger",0},
    {"Directories",0},
    {"Environment",0},
    {"Save",0}
};

ribbonOptions window[15]={
    {"Size/Move",0},
    {"Zoom",0},
    {"Tile",0},
    {"Cascade",0},
    {"Next",0},
    {"Close",0},
    {"Close All",0},
    {"Message",0},
    {"Output",0},
    {"Watch",0},
    {"User Screen",0},
    {"Register",0},
    {"Project",0},
    {"Project Notes",0},
    {"List All",0}
};

ribbonOptions help[6]={
    {"Contents",0},
    {"Index",0},
    {"Topic Search",0},
    {"Previous Topic",0},
    {"Help on Help",0},
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

    DrawRectangleRounded((Rectangle){posx2,MeasureTextEx(usedfont,options[i].name,fontsize,0).y-15,width,height+15},0.1,1,color);
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
            case 1:j=8;drawMenu(file,j,color,usedfont,fontsize,items[0].x);break;//file
            case 2:j=7;drawMenu(edit,j,color,usedfont,fontsize,items[1].x);break;//edit
            case 3:j=7;drawMenu(search,j,color,usedfont,fontsize,items[2].x);break;//search
            case 4: j=6;drawMenu(run,j,color,usedfont,fontsize,items[3].x);break;//run
            case 5: j=6; drawMenu(compile,j,color,usedfont,fontsize,items[4].x); break;//compile
            case 6: j=6; drawMenu(debug,j,color,usedfont,fontsize,items[5].x);break;//debug
            case 7: j=6; drawMenu(project,j,color,usedfont,fontsize,items[6].x);break;//project
            case 8: j=10; drawMenu(options,j,color,usedfont,fontsize,items[7].x);break;//options
            case 9: j=15; drawMenu(window,j,color,usedfont,fontsize,items[8].x);break;//window
            case 10: j=6; drawMenu(help,j,color,usedfont,fontsize,items[9].x); break;//help
    }
}

static int activeMenu=-1;

void handleRibbonHover(void) {
    if (activeMenu == -1) {
        return;
    }


    Vector2 mouse = GetMousePosition();
    int overRibbon=0;

    for (int i = 0; i < 10; i++) {
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

    for (int i = 0; i < 10; i++) {
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
                newfile();
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

            if (strcmp(option,"Save All") == 0) {
                saveall();
            }
        }
    }
}
