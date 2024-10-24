#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include "strsafe.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define HEIGHT 600
#define WIDTH  600
#define MAX_RADIUS 5
#define MIN_RADIUS 3

RECT windowRect;

enum COLOR_NAMES {
    RED = 0,
    BLUE,
    GREEN,
    YELLOW,
    PINK,
    WHITE,
    LIGHT_BLUE,
    GRAY,
    LIGHT_GRAY,
    DARK_GRAY,
    BLACK,
    COLOR_COUNT,
};

COLORREF colors[COLOR_COUNT] = {
    RGB(0xFF,0x00,0x00),
    RGB(0x00,0x00,0xFF),
    RGB(0x00,0xFF,0x00),
    RGB(0xFF,0xFF,0x00),
    RGB(0xFF,0x00,0xFF),
    RGB(0xFF,0xFF,0xFF),
    RGB(0x00,0xFF,0xFF),
    RGB(0x80,0x80,0x80),
    RGB(0xDC,0xDC,0xDC),
    RGB(0x18,0x18,0x18),
    RGB(0x00,0x00,0x00),
};

#define BG_COLOR_BRUSH CreateSolidBrush(colors[WHITE])
#define AXIS_COLOR colors[BLACK]

typedef struct {
    int x;
    int y;
    int z;
} Vec3;

typedef struct {
    int length;
    int width;
    int height;
} Size;

typedef struct {
    int r;
    int g;
    int b;
} Color;

typedef struct {
    Vec3 pos;
    Color color;
    int radius;
    int closest_K;
} Point;

typedef struct {
    Vec3 pos;
    Size size;
    int angle;
    Point points[8]; //more like vertices. 0-3 are the forward facing square, 4-7 are the back facing sqaure. Both starting in top left corner going clockwise.
} Graph;

Graph graph;

Point *points;
int ok, width, height, comps;
int pointCount = 0;

#define K_COUNT 2
Point K[K_COUNT] = {0};


WINBOOL CompareColors(Point p1, Point p2){
    return p1.color.r == p2.color.r && p1.color.g == p2.color.g && p1.color.b == p2.color.b;
}

/*-------------------------------------
|                                     |
|        K-Means functions            |
|                                     |
-------------------------------------*/

void InitializeK(Point *p, Color color, int index){
    p->pos.x = rand()/255;
    p->pos.y = rand()/255;
    p->pos.z = rand()/255;
    p->radius = 8;
    p->color = color;
    p->closest_K = index;

}

void update (Point K[K_COUNT], Point *dots){
    int ax = 0;
    int ay = 0;
    int cc = 0;
    for (int i = 0; i < K_COUNT; i++){
        for (int j = 0; j < pointCount; j++){
            if (CompareColors(K[i], dots[j])){
                ax += dots[j].pos.x;
                ay += dots[j].pos.y;
                cc++;
            }
        }

        if (cc == 0){
            K[i].pos.x = rand() % WIDTH;
            K[i].pos.y = rand() % HEIGHT;
        }
        else{
            K[i].pos.x = (int)ax/cc;
            K[i].pos.y = (int)ay/cc;
        }
        
        cc = 0;
        ax = 0;
        ay = 0;
    }
}

int SquaredDistance(Point b1, Point b2){
    return ((b2.pos.x-b1.pos.x)*(b2.pos.x-b1.pos.x)) + ((b2.pos.y-b1.pos.y)*(b2.pos.y-b1.pos.y)) + ((b2.pos.z-b1.pos.z)*(b2.pos.z-b1.pos.z));
}

float lerp(int a, int b, float t)
{
    return a + t * (b - a);
}

/*-------------------------------------
|                                     |
|        Drawing functions            |
|                                     |
-------------------------------------*/


void DrawBackground(HDC screen, HBRUSH color){
    SelectObject(screen, color);
    Rectangle(screen, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
    DeleteObject(color);
}

void DrawCircle(HDC screen, Color color,int x, int y, int radius){
    HBRUSH brush = CreateSolidBrush(RGB(color.r, color.g, color.b));
    SelectObject(screen, brush);
    Ellipse(screen, x-radius, y-radius, x+radius, y+radius);
    DeleteObject(brush);
}

void Create2DPoint(Point *point, int x, int y, int radius, Color color){
    point->pos.x = x;
    point->pos.y = y;
    point->pos.z = 0;
    point->radius = radius;
    point->color = color;
    point->closest_K = -1;
}

/*-------------------------------------
|                                     |
|        Graph functions              |
|                                     |
-------------------------------------*/

void InitializeGraph(Graph *graph, Vec3 pos, Size size, int angle){
    //1,2,5,6 pushed back a lil to look more like a graph
    //0,3,4,7 pushed forward a lil to look more like a graph
    graph->pos  = pos;
    graph->size = size;
    graph->angle = angle;
    int x = pos.x;
    int y = pos.y;
    int half_len = size.length/2;
    int w = size.width;
    int h = size.height;

    //first square
    Create2DPoint(&graph->points[0], x +     angle, y     + angle, 5,(Color){0,0,0});
    Create2DPoint(&graph->points[1], x + w - angle, y     - angle, 5,(Color){0,0,0}); 
    Create2DPoint(&graph->points[2], x + w - angle, y + h - angle, 5,(Color){0,0,0});
    Create2DPoint(&graph->points[3], x +     angle, y + h + angle, 5,(Color){0,0,0});
    
    //second square
    Create2DPoint(&graph->points[4], x - half_len     + angle, y - half_len     + angle, 5,(Color){0,0,0});
    Create2DPoint(&graph->points[5], x + w - half_len - angle, y - half_len     - angle, 5,(Color){0,0,0});
    Create2DPoint(&graph->points[6], x + w - half_len - angle, y + h - half_len - angle, 5,(Color){0,0,0});
    Create2DPoint(&graph->points[7], x - half_len     + angle, y + h - half_len + angle, 5,(Color){0,0,0});
}

void DrawAxes(HDC screen, Graph g){;

    //drawing thick lines of graph
    HPEN ThickPen = CreatePen(PS_SOLID, 3, AXIS_COLOR);
    SelectObject(screen, ThickPen);
    MoveToEx(screen, g.points[2].pos.x, g.points[2].pos.y, (LPPOINT) NULL);
    LineTo(screen,   g.points[3].pos.x, g.points[3].pos.y);
    LineTo(screen,   g.points[7].pos.x, g.points[7].pos.y);
    LineTo(screen,   g.points[4].pos.x, g.points[4].pos.y);
    DeleteObject(ThickPen);

    //drawing thin lines of graph
    HPEN ThinPen = CreatePen(PS_SOLID, 1, AXIS_COLOR);
    SelectObject(screen, ThinPen);

    for (float t = 0.3333f; t <= 1; t += 0.3333f){
        MoveToEx(screen,(int)lerp(g.points[3].pos.x, g.points[2].pos.x, t), (int)lerp(g.points[3].pos.y, g.points[2].pos.y, t), (LPPOINT) NULL);
        LineTo(screen,  (int)lerp(g.points[7].pos.x, g.points[6].pos.x, t), (int)lerp(g.points[7].pos.y, g.points[6].pos.y, t));
        LineTo(screen,  (int)lerp(g.points[4].pos.x, g.points[5].pos.x, t), (int)lerp(g.points[4].pos.y, g.points[5].pos.y, t));
        
        MoveToEx(screen,(int)lerp(g.points[3].pos.x, g.points[7].pos.x, t), (int)lerp(g.points[3].pos.y, g.points[7].pos.y, t), (LPPOINT) NULL);
        LineTo(screen,  (int)lerp(g.points[2].pos.x, g.points[6].pos.x, t), (int)lerp(g.points[2].pos.y, g.points[6].pos.y, t));
        //prevents thick line in the back
        if (t < 0.9999f)
            LineTo(screen,  (int)lerp(g.points[1].pos.x, g.points[5].pos.x, t), (int)lerp(g.points[1].pos.y, g.points[5].pos.y, t));

        MoveToEx(screen,(int)lerp(g.points[7].pos.x, g.points[4].pos.x, t), (int)lerp(g.points[7].pos.y, g.points[4].pos.y, t), (LPPOINT) NULL);
        LineTo(screen,  (int)lerp(g.points[6].pos.x, g.points[5].pos.x, t), (int)lerp(g.points[6].pos.y, g.points[5].pos.y, t));
        LineTo(screen,  (int)lerp(g.points[2].pos.x, g.points[1].pos.x, t), (int)lerp(g.points[2].pos.y, g.points[1].pos.y, t));
    }
    MoveToEx(screen, g.points[2].pos.x, g.points[2].pos.y, (LPPOINT) NULL);
    LineTo(screen,   g.points[1].pos.x, g.points[1].pos.y);
    
    DeleteObject(ThinPen);
}

void DrawLabels(HDC screen){

    //setup for creating rotated text
    int angle = 150;
    HGDIOBJ hfnt;
    PLOGFONT plf = (PLOGFONT) LocalAlloc(LPTR, sizeof(LOGFONT)); 
    StringCchCopy(plf->lfFaceName, 6, TEXT("Arial"));
    plf->lfWeight = FW_NORMAL; 
    SetBkMode(screen, TRANSPARENT); 

    //Write text
    plf->lfEscapement = angle; 
    hfnt = CreateFontIndirect(plf); 
    SelectObject(screen, hfnt);
    TextOut(screen, 390, 510, "Red", 3); 
    TextOut(screen, 340, 490, "85",  2); 
    TextOut(screen, 412, 475, "170", 3);
    TextOut(screen, 482, 462, "255", 3);  

    angle = 3100;
    plf->lfEscapement = angle; 
    hfnt = CreateFontIndirect(plf); 
    SelectObject(screen, hfnt);
    TextOut(screen, 150, 430, "Blue", 4); 

    angle = 0;
    plf->lfEscapement = angle; 
    hfnt = CreateFontIndirect(plf); 
    SelectObject(screen, hfnt);
    TextOut(screen, 80, 210, "G", 1); 
    TextOut(screen, 83, 225, "r", 1); 
    TextOut(screen, 80, 240, "e", 1); 
    TextOut(screen, 80, 255, "e", 1);
    TextOut(screen, 80, 270, "n", 1);  

    TextOut(screen, 265, 500, "0",   1); 

    DeleteObject(hfnt);
    DeleteObject(plf);
}


void DrawMarkers(HDC screen, Graph g){
    int MarkerLen = 10;
    HPEN ThickPen = CreatePen(PS_SOLID, 3, colors[WHITE]);
    SelectObject(screen, ThickPen);
    for (float t = 0.3333f; t <= 1; t += 0.3333f){
        MoveToEx(screen,(int)lerp(g.points[3].pos.x, g.points[2].pos.x, t), (int)lerp(g.points[3].pos.y, g.points[2].pos.y, t), (LPPOINT) NULL);
        LineTo(screen,(int)lerp(g.points[3].pos.x, g.points[2].pos.x, t) + MarkerLen/2, (int)lerp(g.points[3].pos.y, g.points[2].pos.y, t) + MarkerLen/2);

        MoveToEx(screen,(int)lerp(g.points[3].pos.x, g.points[7].pos.x, t), (int)lerp(g.points[3].pos.y, g.points[7].pos.y, t), (LPPOINT) NULL);
        LineTo(screen,(int)lerp(g.points[3].pos.x, g.points[7].pos.x, t) - MarkerLen, (int)lerp(g.points[3].pos.y, g.points[7].pos.y, t) + 2);

        MoveToEx(screen,(int)lerp(g.points[7].pos.x, g.points[4].pos.x, t), (int)lerp(g.points[7].pos.y, g.points[4].pos.y, t), (LPPOINT) NULL);
        LineTo(screen,(int)lerp(g.points[7].pos.x, g.points[4].pos.x, t) - MarkerLen, (int)lerp(g.points[7].pos.y, g.points[4].pos.y, t) + 2);
    }
    DeleteObject(ThickPen);
}

void DrawGraph(HDC screen, Graph g){
    DrawAxes(screen, g);
    DrawMarkers(screen, g);
    DrawLabels(screen);
}


/*-------------------------------------
|                                     |
|        Point functions              |
|                                     |
-------------------------------------*/

void InitializePoint(Point *point, unsigned char r, unsigned char g,  unsigned char b){
    point->pos.x  = r;
    point->pos.y  = g;
    point->pos.z  = b; 

    //if you want varied radius, put "1"
    //if you want constant radius, put "0"
    #if 0
    float t = ((float)b/255)*100;
    point->radius = ((MAX_RADIUS)-((int)(t)%MAX_RADIUS))+ MIN_RADIUS;
    #else
    point->radius = 4;
    #endif

    point->color  = (Color){r,g,b};
    point->closest_K = -1;
}

void PrintPoint(Point p){
    printf("x: %d, y: %d, z: %d, radius: %d ", p.pos.x, p.pos.y, p.pos.z, p.radius);
}

void DrawPoint(HDC screen, Point point, Graph g){
    int x = g.points[3].pos.x + point.pos.x - (point.pos.z/2);
    int y = g.points[3].pos.y - point.pos.y - (point.pos.z/2);
    if (x > g.points[3].pos.x){
        x = x - (g.angle*2);
        y = y - (g.angle*2);
    }
    DrawCircle(screen, point.color, x, y, point.radius);
}

void DrawPointWithColor(HDC screen, Point *point, Graph g, Color color){
    int x = g.points[3].pos.x + point->pos.x - (point->pos.z/2);
    int y = g.points[3].pos.y - point->pos.y - (point->pos.z/2);
    if (x > g.points[3].pos.x){
        x = x - (g.angle*2);
        y = y - (g.angle*2);
    }
    point->color = color;
    DrawCircle(screen, color, x, y, point->radius);
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    switch(msg)
    {   
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_CREATE:
            //create variables
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            switch (wParam)
            {
                //R
                case 0x52:
                    update(K, points);
                    InvalidateRect(hwnd, &(RECT){windowRect.top, windowRect.left, windowRect.right, windowRect.bottom}, FALSE);
                    break;
                default:
                    break;
            }
            break;

        case WM_PAINT:
            Point closest = K[0];
            Color K_colors[K_COUNT] = {0};
            int K_colors_count[K_COUNT] = {0};
            for (int i = 0; i < pointCount; i++){

                for (int j = 0; j < K_COUNT; j++){
                    if (SquaredDistance(points[i], closest) >= SquaredDistance(points[i], K[j])){
                        closest = K[j];
                        points[i].closest_K = j;

                    }
                }
                K_colors[points[i].closest_K].r += points[i].color.r;
                K_colors[points[i].closest_K].g += points[i].color.g;
                K_colors[points[i].closest_K].b += points[i].color.b;

                K_colors_count[points[i].closest_K] += 1;
                //printf("COLOR %d = r: %d, g: %d, b: %d, count: %d\n", i, K_colors[points[i].closest_K].r, K_colors[points[i].closest_K].g, K_colors[points[i].closest_K].b, K_colors_count[points[i].closest_K]);
            }
            for (int i = 0; i < K_COUNT; i++){
                if (K_colors_count[i] != 0){
                    K_colors[i].r /= K_colors_count[i];
                    K_colors[i].g /= K_colors_count[i];
                    K_colors[i].b /= K_colors_count[i];
                }
                printf("COLOR %d = r: %d, g: %d, b: %d, count: %d\n", i, K_colors[i].r, K_colors[i].g, K_colors[i].b, K_colors_count[i]);
            }

            // sets up backbuffer
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC hdcBack = CreateCompatibleDC(hdc);
            GetClientRect(hwnd, &windowRect); 
            HPEN nullPen = CreatePen(PS_NULL, 1, colors[WHITE]);
            HBITMAP backBuffer = CreateCompatibleBitmap(hdc, windowRect.right, windowRect.bottom);
            SelectObject(hdcBack, backBuffer);
            DrawBackground(hdcBack, BG_COLOR_BRUSH);
    
            //draws
            DrawGraph(hdcBack, graph);

            SelectObject(hdcBack, nullPen); 

            /*
            for (int i = 0; i < pointCount; i++){
                DrawPoint(hdcBack, points[i], graph);
            }
            */

            for (int i = 0; i < pointCount; i++){
                DrawPointWithColor(hdcBack, &points[i], graph, K_colors[points[i].closest_K]);
            }

            //draws Ks
            for (int i = 0; i < K_COUNT; i++){
                DrawPointWithColor(hdcBack, &K[i], graph, K_colors[K[i].closest_K]);
            }
            //copies back buffer into front buffer
            BitBlt(hdc, 0,0, windowRect.right, windowRect.bottom, hdcBack,0,0,SRCCOPY);

            //deletes objects
            DeleteDC(hdcBack);
            DeleteObject(backBuffer);
            DeleteObject(nullPen);
            EndPaint(hwnd, &ps);

            //logic
            
            //slows down program
            Sleep(20);
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}


void WinEasyCreateWindowClass(WNDCLASSEX *wc, HINSTANCE hInstance, char *ClassName){
    wc->cbSize        = sizeof(WNDCLASSEX);
    wc->style         = 0;
    wc->lpfnWndProc   = WndProc;
    wc->cbClsExtra    = 0;
    wc->cbWndExtra    = 0;
    wc->hInstance     = hInstance;
    wc->hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc->hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc->lpszMenuName  = NULL;
    wc->lpszClassName = ClassName;
    wc->hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
}

HWND WinEasyCreateWindow(char *windowTitle, char *className, HINSTANCE hInstance, int w, int h){
    RECT rect = {0, 0, w, h};
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_CLIENTEDGE);
    return CreateWindowEx(
        WS_EX_CLIENTEDGE,
        className,
        windowTitle,
        WS_OVERLAPPEDWINDOW,
        200, 200, rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInstance, NULL);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    srand(time(NULL));
    int argc;
    PWSTR *argv = CommandLineToArgvW(GetCommandLineW(),&argc);
    if (argc < 2){
        printf("ERROR: User must provide input image\n");
        printf("USAGE: ./color-quantizer <image_filepath>\n");
        exit(1);
    }

    size_t filename_len = wcslen(argv[1]);
    char *filename = (char *)malloc(sizeof(char)*(filename_len));
    wcstombs(filename, argv[1], filename_len);
    filename[filename_len] = '\0';

    /*
    if (argc >= 3){
        char *temp = (char *)malloc(sizeof(char)*(wcslen(argv[2])));
        wcstombs(temp, argv[2], wcslen(argv[2]));
        temp[wcslen(argv[2])] = '\0';
        K_COUNT = atoi(temp);

        free(temp);
    }
    */
    ok = stbi_info(filename, &width, &height, &comps);
    if (ok == 0){
        printf("ERROR: Image is not a supported format. Please supply one that is.\n");
        exit(1);
    }
 
    unsigned char *pixels = stbi_load(filename, &width, &height, &comps, 0);
    points = (Point *)malloc((sizeof(Point))*((width*height))); 
    //setup

    printf("K_COUNT: %d\n", K_COUNT);
    printf("width: %d, height: %d, comps: %d\n", width, height, comps);

    for (size_t i = 0; i < (size_t)width*height*comps; i += comps){
        InitializePoint(&points[pointCount], pixels[i],pixels[i+1],pixels[i+2]);
        //PrintPoint(points[pointCount]);
        pointCount++;
    }


    printf("PointCount: %d\n", pointCount);
    free(filename);

    for (int i = 0; i < K_COUNT; i++){
        InitializeK(&K[i], (Color){0,0,0}, i);
    }

    Vec3 GraphPoint = {.x = (WIDTH/2)-50, .y = (HEIGHT/2)-80, .z = 0};
    Size GraphSize =  {.length = 255, .width = 255,  .height = 255};
    int angle = 20;
    InitializeGraph(&graph, GraphPoint, GraphSize, angle);

    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;
    char ClassName[] = "myWindowClass";

    WinEasyCreateWindowClass(&wc, hInstance, ClassName);

    if(!RegisterClassEx(&wc))
    {
        MessageBox(NULL, "Window Registration Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Step 2: Creating the Window
    hwnd = WinEasyCreateWindow("Color Quantizer", ClassName, hInstance, WIDTH, HEIGHT);

    if(hwnd == NULL)
    {
        MessageBox(NULL, "Window Creation Failed!", "Error!",
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    UpdateWindow(hwnd);

    // Step 3: The Message Loop
    while(GetMessage(&Msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return Msg.wParam;
    LocalFree(argv);
}

