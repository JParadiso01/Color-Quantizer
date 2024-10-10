#include <Windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

RECT windowRect;
int x = 273;
int y = 300;
int r = 10;

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
};

#define BGCOLOR CreateSolidBrush(colors[GRAY])

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
    Vec3 pos;
    int radius;
    HBRUSH color;
} Point;

typedef struct {
    Vec3 pos;
    Size size;
    int angle;
    Point points[8]; //more like vertices. 0-3 are the forward facing square, 4-7 are the back facing sqaure. Both starting in top left corner going clockwise.
} Graph;

Graph graph;

#define MAX_POINTS 30
Point points[MAX_POINTS];

#define HEIGHT 600
#define WIDTH  600
#define MAX_Z 20

float lerp(int a, int b, float t)
{
    return a + t * (b - a);
}
void DrawBackground(HDC screen, HBRUSH color){
    SelectObject(screen, color);
    Rectangle(screen, windowRect.left, windowRect.top, windowRect.right, windowRect.bottom);
    DeleteObject(color);
}
void DrawCircle(HDC screen, HBRUSH color,int x, int y, int radius){
    SelectObject(screen, color);
    Ellipse(screen, x-radius, y-radius, x+radius, y+radius);
    DeleteObject(color);
}


void Create2DPoint(Point *point, int x, int y, int radius, HBRUSH color){
    point->pos.x = x;
    point->pos.y = y;
    point->pos.z = 0;
    point->radius = radius;
    point->color = color;
}

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
    Create2DPoint(&graph->points[0], x +     angle, y     + angle, 5,(HBRUSH)NULL);
    Create2DPoint(&graph->points[1], x + w - angle, y     - angle, 5,(HBRUSH)NULL); 
    Create2DPoint(&graph->points[2], x + w - angle, y + h - angle, 5,(HBRUSH)NULL);
    Create2DPoint(&graph->points[3], x +     angle, y + h + angle, 5,(HBRUSH)NULL);
    
    //second square
    Create2DPoint(&graph->points[4], x - half_len     + angle, y - half_len     + angle, 5,(HBRUSH)NULL);
    Create2DPoint(&graph->points[5], x + w - half_len - angle, y - half_len     - angle, 5,(HBRUSH)NULL);
    Create2DPoint(&graph->points[6], x + w - half_len - angle, y + h - half_len - angle, 5,(HBRUSH)NULL);
    Create2DPoint(&graph->points[7], x - half_len     + angle, y + h - half_len + angle, 5,(HBRUSH)NULL);
}

void DrawAxes(HDC screen, Graph g){;

    //drawing thick lines of graph
    HPEN ThickPen = CreatePen(PS_SOLID, 3, colors[WHITE]);
    SelectObject(screen, ThickPen);
    MoveToEx(screen, g.points[2].pos.x, g.points[2].pos.y, (LPPOINT) NULL);
    LineTo(screen,   g.points[3].pos.x, g.points[3].pos.y);
    LineTo(screen,   g.points[7].pos.x, g.points[7].pos.y);
    LineTo(screen,   g.points[4].pos.x, g.points[4].pos.y);
    DeleteObject(ThickPen);

    //drawing thin lines of graph
    HPEN ThinPen = CreatePen(PS_SOLID, 1, colors[WHITE]);
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

void DrawLabels(HDC screen, Graph g){
    //TODO: Implement function

}

void DrawMarkers(HDC screen, Graph g){
    int MarkerLen = 10;
    HPEN ThickPen = CreatePen(PS_SOLID, 3, colors[WHITE]);

    DeleteObject(ThickPen);
    //TODO: Implement function

}

void DrawGraph(HDC screen, Graph g){
    DrawAxes(screen, g);
    DrawLabels(screen, g);
    DrawMarkers(screen, g);
}



void InitializePoint(Point *point){
    point->pos.x  = rand()%WIDTH;
    point->pos.y  = rand()%HEIGHT;
    point->pos.z  = rand()%MAX_Z; 
    point->radius = point->pos.z;
    point->color  = CreateSolidBrush(colors[rand()%COLOR_COUNT]);
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
                default:
                    break;
            }
            break;

        case WM_PAINT:
            // sets up backbuffer
            HDC hdc = BeginPaint(hwnd, &ps);
            HDC hdcBack = CreateCompatibleDC(hdc);
            GetClientRect(hwnd, &windowRect); 
            HBITMAP backBuffer = CreateCompatibleBitmap(hdc, windowRect.right, windowRect.bottom);
            SelectObject(hdcBack, backBuffer);
            DrawBackground(hdcBack, BGCOLOR);
            //tells program to redraw the screen
            //InvalidateRect(hwnd, &(RECT){windowRect.top, windowRect.left, windowRect.right, windowRect.bottom}, FALSE);
    
            //draws
            for (int i = 0; i < MAX_POINTS; i++){
                //DrawCircle(hdcBack, points[i].color, points[i].pos.x, points[i].pos.y, points[i].radius);
            }
            DrawGraph(hdcBack, graph);
            //copies back buffer into front buffer
            BitBlt(hdc, 0,0, windowRect.right, windowRect.bottom, hdcBack,0,0,SRCCOPY);

            //deletes objects
            DeleteDC(hdcBack);
            DeleteObject(backBuffer);
            EndPaint(hwnd, &ps);

            //logic
            x = (x+5)%WIDTH;
            y = (y+5)%HEIGHT;
            
            
            //slows down program
            printf("PAINTED ");
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

    //setup
    for (int i = 0; i < MAX_POINTS; i++){
        InitializePoint(&points[i]);
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

}

