// CrossBarWarrier.cpp : アプリケーションのエントリ ポイントを定義します。
//

#include "framework.h"
#include "CrossBarWarrier.h"
#include <vector>
#include <functional>
#include <CommCtrl.h>

#pragma comment(lib, "Comctl32.lib")

#define MAX_LOADSTRING 100

// グローバル変数:
HINSTANCE hInst;                                // 現在のインターフェイス
WCHAR szTitle[MAX_LOADSTRING];                  // タイトル バーのテキスト
WCHAR szWindowClass[MAX_LOADSTRING];            // メイン ウィンドウ クラス名

// ゲーム関連のグローバル変数
const int BOARD_SIZE = 1000;
const int CONTROL_HEIGHT = 100;

HWND hBtnStart;
HWND hRadioPlayer1;
HWND hRadioPlayer2;
HWND hEditStatus;
HWND hSpinner;
HWND hSpinnerBuddy;

enum class CellState {
    EMPTY,
    RED_DOT,
    GREEN_DOT,
    RED_LINE,
    GREEN_LINE,
    RED_DOT_RED_LINE,
    GREEN_DOT_GREEN_LINE
};

struct GameState {
    bool isPlaying;
    bool isPlayer1Turn;
    int gridSize;
    int cellSize;
    std::vector<std::vector<CellState>> grid;
    
    GameState() : isPlaying(false), isPlayer1Turn(true), gridSize(3), cellSize(0) {}
};

GameState g_game;

// このコード モジュールに含まれる関数の宣言を転送します:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// ゲーム関数
void InitializeGame(int level);
void DrawBoard(HDC hdc, HWND hWnd);
bool CheckValidClick(int row, int col);
void PlaceLine(int row, int col);
bool CheckWin();
void UpdateStatusText(const WCHAR* text);
bool DFSRed(int row, int col, std::vector<std::vector<bool>>& visited);
bool DFSGreen(int row, int col, std::vector<std::vector<bool>>& visited);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: ここにコードを挿入してください。

    // グローバル文字列を初期化する
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CROSSBARWARRIER, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // アプリケーション初期化の実行:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CROSSBARWARRIER));

    MSG msg;

    // メイン メッセージ ループ:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  関数: MyRegisterClass()
//
//  目的: ウィンドウ クラスを登録します。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CROSSBARWARRIER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CROSSBARWARRIER);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   関数: InitInstance(HINSTANCE, int)
//
//   目的: インスタンス ハンドルを保存して、メイン ウィンドウを作成します
//
//   コメント:
//
//        この関数で、グローバル変数でインスタンス ハンドルを保存し、
//        メイン プログラム ウィンドウを作成および表示します。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // グローバル変数にインスタンス ハンドルを格納する

   // ウィンドウサイズを計算
   RECT rect = { 0, 0, BOARD_SIZE, BOARD_SIZE + CONTROL_HEIGHT };
   AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, TRUE);
   
   int windowWidth = rect.right - rect.left;
   int windowHeight = rect.bottom - rect.top;

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, 
      WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
      CW_USEDEFAULT, 0, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  関数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目的: メイン ウィンドウのメッセージを処理します。
//
//  WM_COMMAND  - アプリケーション メニューの処理
//  WM_PAINT    - メイン ウィンドウを描画する
//  WM_DESTROY  - 中止メッセージを表示して戻る
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        {
            // コントロールの作成
            hBtnStart = CreateWindowW(L"BUTTON", L"開始",
                WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                10, 10, 80, 30, hWnd, (HMENU)IDC_BTN_START, hInst, NULL);
            
            hRadioPlayer1 = CreateWindowW(L"BUTTON", L"人",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
                100, 10, 60, 30, hWnd, (HMENU)IDC_RADIO_PLAYER1, hInst, NULL);
            
            hRadioPlayer2 = CreateWindowW(L"BUTTON", L"PC",
                WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
                170, 10, 60, 30, hWnd, (HMENU)IDC_RADIO_PLAYER2, hInst, NULL);
            
            SendMessage(hRadioPlayer1, BM_SETCHECK, BST_CHECKED, 0);
            
            hEditStatus = CreateWindowW(L"EDIT", L"",
                WS_VISIBLE | WS_CHILD | ES_READONLY | ES_CENTER,
                240, 10, 100, 30, hWnd, (HMENU)IDC_EDIT_STATUS, hInst, NULL);
            
            hSpinnerBuddy = CreateWindowW(L"EDIT", L"1",
                WS_VISIBLE | WS_CHILD | ES_NUMBER,
                350, 10, 50, 30, hWnd, (HMENU)IDC_SPINNER_BUDDY, hInst, NULL);
            
            hSpinner = CreateWindowW(UPDOWN_CLASS, NULL,
                WS_VISIBLE | WS_CHILD | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
                0, 0, 0, 0, hWnd, (HMENU)IDC_SPINNER, hInst, NULL);
            
            SendMessage(hSpinner, UDM_SETBUDDY, (WPARAM)hSpinnerBuddy, 0);
            SendMessage(hSpinner, UDM_SETRANGE, 0, MAKELPARAM(10, 1));
            SendMessage(hSpinner, UDM_SETPOS, 0, 1);
        }
        break;
        
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case IDC_BTN_START:
                {
                    int level = (int)SendMessage(hSpinner, UDM_GETPOS, 0, 0);
                    InitializeGame(level);
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
        
    case WM_LBUTTONDOWN:
        {
            if (!g_game.isPlaying)
                break;
            
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            
            if (y < CONTROL_HEIGHT)
                break;
            
            y -= CONTROL_HEIGHT;
            
            int col = x / g_game.cellSize;
            int row = y / g_game.cellSize;
            
            if (CheckValidClick(row, col))
            {
                PlaceLine(row, col);
                
                if (CheckWin())
                {
                    g_game.isPlaying = false;
                    UpdateStatusText(g_game.isPlayer1Turn ? L"☗勝ち" : L"☖勝ち");
                }
                else
                {
                    g_game.isPlayer1Turn = !g_game.isPlayer1Turn;
                    UpdateStatusText(g_game.isPlayer1Turn ? L"☗番" : L"☖番");
                }
                
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        break;
        
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            DrawBoard(hdc, hWnd);
            EndPaint(hWnd, &ps);
        }
        break;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
        
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// バージョン情報ボックスのメッセージ ハンドラーです。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// ゲームロジック関数の実装

void UpdateStatusText(const WCHAR* text)
{
    SetWindowTextW(hEditStatus, text);
}

void InitializeGame(int level)
{
    g_game.gridSize = level * 2 + 1;
    g_game.cellSize = BOARD_SIZE / g_game.gridSize;
    g_game.isPlaying = true;
    g_game.isPlayer1Turn = true;
    
    g_game.grid.clear();
    g_game.grid.resize(g_game.gridSize, std::vector<CellState>(g_game.gridSize, CellState::EMPTY));
    
    // 赤い点を配置（偶数行、奇数列）
    for (int row = 0; row < g_game.gridSize; row += 2)
    {
        for (int col = 1; col < g_game.gridSize; col += 2)
        {
            g_game.grid[row][col] = CellState::RED_DOT;
        }
    }
    
    // 緑の点を配置（奇数行、偶数列）
    for (int row = 1; row < g_game.gridSize; row += 2)
    {
        for (int col = 0; col < g_game.gridSize; col += 2)
        {
            g_game.grid[row][col] = CellState::GREEN_DOT;
        }
    }
    
    UpdateStatusText(L"☗番");
}

void DrawBoard(HDC hdc, HWND hWnd)
{
    if (!g_game.isPlaying || g_game.grid.empty())
        return;
    
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int boardTop = CONTROL_HEIGHT;
    
    // 各セルを描画
    for (int row = 0; row < g_game.gridSize; row++)
    {
        for (int col = 0; col < g_game.gridSize; col++)
        {
            int x = col * g_game.cellSize;
            int y = boardTop + row * g_game.cellSize;
            int centerX = x + g_game.cellSize / 2;
            int centerY = y + g_game.cellSize / 2;
            
            CellState state = g_game.grid[row][col];
            
            // 点を描画
            if (state == CellState::RED_DOT || state == CellState::RED_DOT_RED_LINE)
            {
                HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, redBrush);
                Ellipse(hdc, centerX - 5, centerY - 5, centerX + 5, centerY + 5);
                SelectObject(hdc, oldBrush);
                DeleteObject(redBrush);
            }
            else if (state == CellState::GREEN_DOT || state == CellState::GREEN_DOT_GREEN_LINE)
            {
                HBRUSH greenBrush = CreateSolidBrush(RGB(0, 255, 0));
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, greenBrush);
                Ellipse(hdc, centerX - 5, centerY - 5, centerX + 5, centerY + 5);
                SelectObject(hdc, oldBrush);
                DeleteObject(greenBrush);
            }
            
            // 線を描画
            if (state == CellState::RED_LINE || state == CellState::RED_DOT_RED_LINE)
            {
                if (row > 0 && (g_game.grid[row - 1][col] == CellState::RED_DOT || 
                                g_game.grid[row - 1][col] == CellState::RED_DOT_RED_LINE))
                {
                    HPEN redPen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
                    HPEN oldPen = (HPEN)SelectObject(hdc, redPen);
                    MoveToEx(hdc, centerX, centerY, NULL);
                    LineTo(hdc, centerX, y - g_game.cellSize / 2);
                    SelectObject(hdc, oldPen);
                    DeleteObject(redPen);
                }
            }
            else if (state == CellState::GREEN_LINE || state == CellState::GREEN_DOT_GREEN_LINE)
            {
                if (col > 0 && (g_game.grid[row][col - 1] == CellState::GREEN_DOT || 
                                g_game.grid[row][col - 1] == CellState::GREEN_DOT_GREEN_LINE))
                {
                    HPEN greenPen = CreatePen(PS_SOLID, 3, RGB(0, 255, 0));
                    HPEN oldPen = (HPEN)SelectObject(hdc, greenPen);
                    MoveToEx(hdc, centerX, centerY, NULL);
                    LineTo(hdc, x - g_game.cellSize / 2, centerY);
                    SelectObject(hdc, oldPen);
                    DeleteObject(greenPen);
                }
            }
        }
    }
}

bool CheckValidClick(int row, int col)
{
    if (row < 0 || row >= g_game.gridSize || col < 0 || col >= g_game.gridSize)
        return false;
    
    if (g_game.grid[row][col] != CellState::EMPTY)
        return false;
    
    if (g_game.isPlayer1Turn)
    {
        // 先手：上下に赤い点があるか
        bool hasRedAbove = (row > 0 && (g_game.grid[row - 1][col] == CellState::RED_DOT || 
                                        g_game.grid[row - 1][col] == CellState::RED_DOT_RED_LINE));
        bool hasRedBelow = (row < g_game.gridSize - 1 && 
                           (g_game.grid[row + 1][col] == CellState::RED_DOT || 
                            g_game.grid[row + 1][col] == CellState::RED_DOT_RED_LINE));
        return hasRedAbove && hasRedBelow;
    }
    else
    {
        // 後手：左右に緑の点があるか
        bool hasGreenLeft = (col > 0 && (g_game.grid[row][col - 1] == CellState::GREEN_DOT || 
                                         g_game.grid[row][col - 1] == CellState::GREEN_DOT_GREEN_LINE));
        bool hasGreenRight = (col < g_game.gridSize - 1 && 
                             (g_game.grid[row][col + 1] == CellState::GREEN_DOT || 
                              g_game.grid[row][col + 1] == CellState::GREEN_DOT_GREEN_LINE));
        return hasGreenLeft && hasGreenRight;
    }
}

void PlaceLine(int row, int col)
{
    if (g_game.isPlayer1Turn)
    {
        g_game.grid[row][col] = CellState::RED_LINE;
    }
    else
    {
        g_game.grid[row][col] = CellState::GREEN_LINE;
    }
}

bool DFSRed(int row, int col, std::vector<std::vector<bool>>& visited)
{
    if (row < 0 || row >= g_game.gridSize || col < 0 || col >= g_game.gridSize)
        return false;
    if (visited[row][col])
        return false;
    
    CellState state = g_game.grid[row][col];
    if (state != CellState::RED_DOT && state != CellState::RED_LINE && 
        state != CellState::RED_DOT_RED_LINE)
        return false;
    
    visited[row][col] = true;
    
    if (row == g_game.gridSize - 1 && (state == CellState::RED_DOT || 
                                       state == CellState::RED_DOT_RED_LINE))
        return true;
    
    if (DFSRed(row + 1, col, visited)) return true;
    if (DFSRed(row - 1, col, visited)) return true;
    
    return false;
}

bool DFSGreen(int row, int col, std::vector<std::vector<bool>>& visited)
{
    if (row < 0 || row >= g_game.gridSize || col < 0 || col >= g_game.gridSize)
        return false;
    if (visited[row][col])
        return false;
    
    CellState state = g_game.grid[row][col];
    if (state != CellState::GREEN_DOT && state != CellState::GREEN_LINE && 
        state != CellState::GREEN_DOT_GREEN_LINE)
        return false;
    
    visited[row][col] = true;
    
    if (col == g_game.gridSize - 1 && (state == CellState::GREEN_DOT || 
                                       state == CellState::GREEN_DOT_GREEN_LINE))
        return true;
    
    if (DFSGreen(row, col + 1, visited)) return true;
    if (DFSGreen(row, col - 1, visited)) return true;
    
    return false;
}

bool CheckWin()
{
    if (g_game.isPlayer1Turn)
    {
        // 先手の勝利判定：上から下へ赤い線でつながっているか
        std::vector<std::vector<bool>> visited(g_game.gridSize, std::vector<bool>(g_game.gridSize, false));
        
        for (int col = 0; col < g_game.gridSize; col++)
        {
            if ((g_game.grid[0][col] == CellState::RED_DOT || 
                 g_game.grid[0][col] == CellState::RED_DOT_RED_LINE) && DFSRed(0, col, visited))
                return true;
        }
    }
    else
    {
        // 後手の勝利判定：左から右へ緑の線でつながっているか
        std::vector<std::vector<bool>> visited(g_game.gridSize, std::vector<bool>(g_game.gridSize, false));
        
        for (int row = 0; row < g_game.gridSize; row++)
        {
            if ((g_game.grid[row][0] == CellState::GREEN_DOT || 
                 g_game.grid[row][0] == CellState::GREEN_DOT_GREEN_LINE) && DFSGreen(row, 0, visited))
                return true;
        }
    }
    
    return false;
}
