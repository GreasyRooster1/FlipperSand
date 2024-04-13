#include <furi.h>
#include <gui/gui.h>
#include <sand_icons.h>

#define WIDTH 128
#define HEIGHT 64

int gridSize = 2;
bool readyForInput = false;
bool simulating = false;
bool splash = true;

int pointerX = 64;
int pointerY = 32;

int borderX = 128;
int borderY = 64;

//allocate for the maximum resolution
bool currentCells[128][64] = {};
bool previousCells[128][64] = {};

static void setup_game(){
    pointerX = 64/gridSize;
    pointerY = 32/gridSize;
    
    borderX = 128/gridSize;
    borderY = 64/gridSize;

    //set all cells to 0
    for (int i = 0; i < WIDTH; i++){
        for (int j = 0; j < HEIGHT; j++){
            currentCells[i][j] = 0;
            currentCells[i][j] = 0;
            previousCells[i][j] = 0;
        }
    }
}

static void draw_rect(Canvas* const canvas,int x,int y,int w,int h){
    for(int i = 0; i < w; i++){
		for(int j = 0; j < h; j++){
			canvas_draw_dot(canvas, x+i, y+j);
		}
	}
}

static void setupSimulation(){
    memcpy(previousCells, currentCells, WIDTH*HEIGHT*sizeof(bool));
}

static bool placeFree(int x,int y){
    if(y>borderY-1||y<0){
        return false;
    }
    if(x>borderX-1||x<0){
        return false;
    }
    return !previousCells[x][y];
}

static void simulate(){
    memcpy(previousCells, currentCells, WIDTH*HEIGHT*sizeof(bool));
    for (int i = 0; i < WIDTH; i++){
        for (int j = 0; j < HEIGHT; j++){
            if(previousCells[i][j]==1){
                if(placeFree(i,j+1)){
                    currentCells[i][j]=0;
                    currentCells[i][j+1]=1;
                }else{
                    if(placeFree(i+1,j+1)){
                        currentCells[i][j]=0;
                        currentCells[i+1][j+1]=1;
                    }else if(placeFree(i-1,j+1)){
                        currentCells[i][j]=0;
                        currentCells[i-1][j+1]=1;
                    }
                }
            }
        }
    }
}

static void set_scene_draw(Canvas* canvas){
    for (int i = 0; i < WIDTH; i++){
        for (int j = 0; j < HEIGHT; j++){
            if(currentCells[i][j]==1){
                draw_rect(canvas, i*gridSize, j*gridSize,gridSize,gridSize);
            }
        }
    }
    if(currentCells[pointerX][pointerY]==1){
        draw_rect(canvas, pointerX*gridSize-1, pointerY*gridSize,gridSize+2,gridSize);
        draw_rect(canvas, pointerX*gridSize, pointerY*gridSize-1,gridSize,gridSize+2);
        canvas_set_color(canvas,ColorWhite);
        draw_rect(canvas, pointerX*gridSize, pointerY*gridSize,gridSize,gridSize);
        canvas_set_color(canvas,ColorBlack);
    }else{
        draw_rect(canvas, pointerX*gridSize, pointerY*gridSize,gridSize,gridSize);
    }
}

static void sand_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    readyForInput = true;
    canvas_set_color(canvas,ColorBlack);

    canvas_clear(canvas);
    
    if(splash){
        char snum[5];
        itoa(gridSize, snum, 10);

        canvas_draw_icon(canvas,0,0,&I_Splash_128x64);
        canvas_draw_str(canvas, 85,10,"Scale:");
        canvas_draw_str(canvas, 116,10,snum);
        canvas_draw_str(canvas, 85,18,"Use up");
        canvas_draw_str(canvas, 85,26,"and down");
        canvas_draw_str(canvas, 85,34,"to change");
        return;
    }

    if(simulating){
        simulate();
    }
    set_scene_draw(canvas);
}

static void sand_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;

    furi_message_queue_put(event_queue, input_event, FuriWaitForever);
}

static void flip(){
    if(currentCells[pointerX][pointerY]){
        currentCells[pointerX][pointerY] = 0;
    }else{
        currentCells[pointerX][pointerY] = 1;
    }
}

int32_t sand_app(void* p) {
    UNUSED(p);

    InputEvent event;
    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    // Configure view port
    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, sand_draw_callback, NULL);
    view_port_input_callback_set(view_port, sand_input_callback, event_queue);

    // Register view port in GUI
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    

    while(1) {
        furi_check(furi_message_queue_get(event_queue, &event, FuriWaitForever) == FuriStatusOk);

        
        if(splash){
            if(event.type == InputTypePress){
                if(event.key == InputKeyBack) {
                    break;
                }
                if(event.key == InputKeyOk){
                    splash = false;
                    setup_game();
                }
                if(event.key == InputKeyUp) {
                    gridSize++;
                }
                if(event.key == InputKeyDown) {
                    gridSize--;
                }
            }
            continue;
        }

        if(event.key == InputKeyBack) {
            if(event.type==InputTypeLong){
                splash = true;
            }
            if(event.type==InputTypeShort){
                simulating = !simulating;
                if(simulating){
                    setupSimulation();
                }
            }
        }
        if(event.type==InputTypePress){
            if(event.key == InputKeyUp) {
                pointerY-=1;
            }
            if(event.key == InputKeyDown) {
                pointerY+=1;
            }
            if(event.key == InputKeyLeft) {
                pointerX-=1;
            }
            if(event.key == InputKeyRight) {
                pointerX+=1;
            }
            if(event.key == InputKeyOk) {
                flip();
            }
        }
    }

    furi_message_queue_free(event_queue);

    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_record_close(RECORD_GUI);

    return 0;
}