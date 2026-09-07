#include "raylib.h"

typedef struct {
    char first_letter;
    char *remains;
    float x;
    Rectangle rect;
} ribbonItems;

ribbonItems items[10] = {
    {'F', "ile",0},
    {'E', "dit",0},
    {'S', "earch",0},
{'R', "un",0},
    {'C', "ompile",0},
    {'D', "ebug",0},
    {'P', "roject",0},
    {'O', "ptions",0},
    {'W', "indow",0},
    {'H', "elp",0}
};

typedef struct {
    char *name;
    float size;
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

void drawMenu(ribbonOptions option[],int i,Color color, Font usedfont,float fontsize) {

}

void selectMenu(int i, Color color, Font usedfont, float fontsize) {
    int j;
    switch (i) {
            case 1:j=8;drawMenu(file,j,color,usedfont,fontsize);break;//file
            case 2:j=7;drawMenu(edit,j,color,usedfont,fontsize);break;//edit
            case 3:j=7;drawMenu(search,j,color,usedfont,fontsize);break;//search
            case 4: j=6;drawMenu(run,j,color,usedfont,fontsize);break;//run
            case 5: j=6; drawMenu(compile,j,color,usedfont,fontsize); break;//compile
            case 6: j=6; drawMenu(debug,j,color,usedfont,fontsize);break;//debug
            case 7: j=6; drawMenu(project,j,color,usedfont,fontsize);break;//project
            case 8: j=10; drawMenu(options,j,color,usedfont,fontsize);break;//options
            case 9: j=15; drawMenu(window,j,color,usedfont,fontsize);break;//window
            case 10: j=6; drawMenu(help,j,color,usedfont,fontsize); break;//help
    }
}



void drawRibbon(Color color, Font usedfont, float fontsize) {
    Vector2 Size;
    float sumx;
    int flag=0;
    for (int i=0;i<10;i++) {
        char c[2]={items[i].first_letter,'\0'};
        Size=MeasureTextEx(usedfont,c,fontsize,0);
        items[i].x+=Size.x;
        Size=MeasureTextEx(usedfont,items[i].remains,fontsize,0);
        items[i].x+=Size.x;
        sumx+=Size.x;
    }
    sumx=(GetScreenWidth()-sumx-10)/16;
    int posx=(GetScreenWidth()*0.01);
    DrawRectangle(0,0,GetScreenWidth(),Size.y+2,color);
    int posx2=posx;
    for (int i=0;i<10;i++) {
        char c[2]={items[i].first_letter,'\0'};
        int width=MeasureTextEx(usedfont,c,fontsize,0).x;
        width+=MeasureTextEx(usedfont,items[i].remains,fontsize,0).x;
        items[i].rect=(Rectangle){posx2,0,width+4,fontsize};
        posx2+=width;
        posx2+=(GetScreenWidth()*0.02);

        if (CheckCollisionPointRec(GetMousePosition(),items[i].rect)) {
//            DrawRectangleRounded(items[i].rect,0.2,8,LIGHTGRAY);
        }
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(GetMousePosition(),items[i].rect)) {

                while (1) {
                    DrawRectangleRounded(items[i].rect,0.2,8,LIGHTGRAY);
                    selectMenu(i,color,usedfont,fontsize);
                    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                        if (CheckCollisionPointRec(GetMousePosition(),items[i].rect)) {

                        }
                    }
                }
            }
        }
        DrawTextEx(usedfont,c,(Vector2){posx,1},fontsize,0,RED);
        posx+=MeasureTextEx(usedfont,c,fontsize,0).x;
        DrawTextEx(usedfont,items[i].remains,(Vector2){posx,1},fontsize,0,BLACK);
        posx+=MeasureTextEx(usedfont,items[i].remains,fontsize,0).x;
        posx+=(GetScreenWidth()*0.02);

    }
}

