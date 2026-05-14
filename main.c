/*
 * 智能自动输入工具
 * 功能：自动模拟键盘输入，支持中英文混合
 * 
 * 编译命令：
 * gcc -O2 -s -mwindows -municode -o AutoInputTool.exe main.c -luser32 -lkernel32 -lcomctl32 -lgdi32
 */

#include <windows.h>    /* Windows系统函数 */
#include <commctrl.h>   /* 进度条、滑块等控件 */
#include <stdio.h>      /* 标准输入输出 */
#include <stdlib.h>     /* 内存分配等 */
#include <string.h>     /* 字符串处理 */
#include <stdbool.h>    /* 布尔类型 true/false */

/* 链接控件库 */
#pragma comment(lib, "comctl32.lib")

/* ==================== 控件ID定义 ==================== */
/* 每个控件需要一个唯一的ID，就像每个人都有身份证号 */
#define ID_INPUT_BOX     1001   /* 输入文本框 */
#define ID_START_BUTTON  1002   /* 开始按钮 */
#define ID_STOP_BUTTON   1003   /* 停止按钮 */
#define ID_SPEED_SLIDER  1004   /* 速度滑块 */
#define ID_SPEED_LABEL   1005   /* 速度标签 */
#define ID_STATUS_LABEL  1006   /* 状态标签 */
#define ID_PROGRESS_BAR  1007   /* 进度条 */

/* 自定义消息，用于线程和界面通信 */
#define MSG_INPUT_DONE   (WM_USER + 1)   /* 输入完成 */
#define MSG_INPUT_PROGRESS (WM_USER + 2) /* 输入进度更新 */

/* ==================== 窗口尺寸定义 ==================== */
#define WINDOW_WIDTH   600   /* 窗口宽度 */
#define WINDOW_HEIGHT  500   /* 窗口高度 */
#define HEADER_HEIGHT   80   /* 顶部标题栏高度 */
#define FOOTER_HEIGHT   60   /* 底部状态栏高度 */
#define PADDING         20   /* 内边距 */

/* ==================== 颜色定义 ==================== */
/* RGB(红, 绿, 蓝)，取值范围 0-255 */
#define COLOR_BACKGROUND  RGB(245, 247, 250)   /* 浅灰背景 */
#define COLOR_HEADER      RGB(41, 128, 185)    /* 蓝色标题栏 */
#define COLOR_TEXT_DARK   RGB(44, 62, 80)      /* 深色文字 */
#define COLOR_WHITE       RGB(255, 255, 255)   /* 白色 */

/* ==================== 数据结构定义 ==================== */

/* 
 * 控件集合结构体
 * 把所有控件句柄放在一起，方便管理
 * HWND = Handle to Window，窗口句柄（类似遥控器）
 */
typedef struct {
    HWND inputBox;       /* 输入文本框 */
    HWND startButton;    /* 开始按钮 */
    HWND stopButton;     /* 停止按钮 */
    HWND speedSlider;    /* 速度滑块 */
    HWND speedLabel;     /* 速度标签 */
    HWND statusLabel;    /* 状态标签 */
    HWND progressBar;    /* 进度条 */
    HFONT fontTitle;     /* 标题字体 */
    HFONT fontNormal;    /* 普通字体 */
    HFONT fontBold;      /* 粗体字体 */
    HBRUSH brushBg;      /* 背景画刷 */
    HBRUSH brushHeader;  /* 标题栏画刷 */
} Controls;

/* Controls????????????????????????????? */

/*
 * 输入状态结构体
 * 记录输入过程的各种状态
 */
typedef struct {
    bool isRunning;      /* 是否正在输入 */
    bool shouldStop;     /* 是否应该停止 */
    int delayMs;         /* 每个字符的延迟（毫秒） */
    int totalChars;      /* 总字符数 */
    int currentChar;     /* 当前输入到第几个字符 */
    HANDLE threadHandle; /* 输入线程句柄 */
    CRITICAL_SECTION cs; /* 临界区，用于线程同步（防止多个线程同时修改数据） */
} InputState;

/* InputState???????????????????????????? */

/* ==================== 全局变量 ==================== */
/* static 表示只在当前文件可见 */
static Controls g_controls = {0};      /* 控件集合，初始化为0 */
static InputState g_state = {0};       /* 输入状态，初始化为0 */
static HINSTANCE g_appInstance = NULL; /* 应用程序实例句柄 */

/* ==================== 函数声明 ==================== */
/* 初始化函数 */
void InitInputState(void);
void CleanupInputState(void);

/* 界面创建函数 */
HFONT CreateMyFont(int size, bool bold);
void CreateAllControls(HWND parentWindow);

/* 输入模拟函数 */
void PressKey(WORD keyCode);
void TypeChar(wchar_t ch);
void TypeEnter(void);
void TypeTab(void);
void TypeSpace(void);
bool IsChinese(wchar_t ch);

/* 输入控制函数 */
DWORD WINAPI InputThread(LPVOID param);
void StartTyping(HWND window);
void StopTyping(void);

/* 界面更新函数 */
void UpdateSpeedLabel(int delay);
void UpdateStatus(const wchar_t* text);
void UpdateProgress(int percent);

/* 窗口消息处理函数 */
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

/* ==================== 初始化函数实现 ==================== */

/*
 * 初始化输入状态
 * 就像游戏开始前重置所有分数
 */
void InitInputState(void) {
    /* ???????????????????? */
    g_state.isRunning = false;      /* 不在运行 */
    g_state.shouldStop = false;     /* 不需要停止 */
    g_state.delayMs = 50;           /* 默认50毫秒延迟 */
    g_state.totalChars = 0;         /* 0个字符 */
    g_state.currentChar = 0;        /* 从0开始 */
    g_state.threadHandle = NULL;    /* 没有线程 */
    
    /* 初始化临界区（线程锁） */
    InitializeCriticalSection(&g_state.cs);
}

/*
 * 清理输入状态
 * 程序结束时释放资源
 */
void CleanupInputState(void) {
    /* ??????????????????? */
    DeleteCriticalSection(&g_state.cs);
}

/* ==================== 界面创建函数实现 ==================== */

/*
 * 创建字体
 * size: 字体大小
 * bold: 是否粗体
 */
HFONT CreateMyFont(int size, bool bold) {
    /* ??????????????????????? */
    /* CreateFontW 创建字体，W表示宽字符版本（支持中文） */
    return CreateFontW(
        size,               /* 字体高度 */
        0,                  /* 字体宽度（0表示自动） */
        0, 0,               /* 倾斜角度 */
        bold ? FW_BOLD : FW_NORMAL,  /* 粗细 */
        FALSE,              /* 不斜体 */
        FALSE,              /* 无下划线 */
        FALSE,              /* 无删除线 */
        DEFAULT_CHARSET,    /* 默认字符集 */
        OUT_DEFAULT_PRECIS, /* 输出精度 */
        CLIP_DEFAULT_PRECIS,/* 裁剪精度 */
        CLEARTYPE_QUALITY,  /* 清晰类型质量 */
        DEFAULT_PITCH | FF_SWISS,  /* 字体间距和族 */
        L"Microsoft YaHei"  /* 字体名称：微软雅黑 */
    );
}

/*
 * 创建所有界面控件
 */
void CreateAllControls(HWND parentWindow) {
    /* ????????????????????????????? */
    /* 创建字体 */
    g_controls.fontTitle = CreateMyFont(24, true);   /* 大标题 */
    g_controls.fontNormal = CreateMyFont(14, false); /* 普通文字 */
    g_controls.fontBold = CreateMyFont(14, true);    /* 粗体文字 */
    
    /* 创建画刷（用于填充背景色） */
    g_controls.brushBg = CreateSolidBrush(COLOR_BACKGROUND);
    g_controls.brushHeader = CreateSolidBrush(COLOR_HEADER);
    
    /* -------- 输入框 -------- */
    /* 标签 */
    HWND label = CreateWindowExW(
        0,                  /* 扩展样式 */
        L"STATIC",          /* 控件类名：静态文本 */
        L"输入内容：",       /* 显示文字 */
        WS_CHILD | WS_VISIBLE | SS_LEFT,  /* 样式：子窗口、可见、左对齐 */
        PADDING,            /* X坐标 */
        HEADER_HEIGHT + PADDING + 10,  /* Y坐标 */
        200, 25,            /* 宽度和高度 */
        parentWindow,       /* 父窗口 */
        NULL,               /* 菜单/ID */
        g_appInstance,      /* 应用程序实例 */
        NULL                /* 附加参数 */
    );
    SendMessage(label, WM_SETFONT, (WPARAM)g_controls.fontBold, TRUE);
    
    /* 多行文本输入框 */
    g_controls.inputBox = CreateWindowExW(
        WS_EX_CLIENTEDGE,   /* 扩展样式：凹陷边框 */
        L"EDIT",            /* 控件类名：编辑框 */
        L"",                /* 初始文字（空） */
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        PADDING, HEADER_HEIGHT + PADDING + 40,
        WINDOW_WIDTH - PADDING * 2, 200,
        parentWindow,
        (HMENU)ID_INPUT_BOX,
        g_appInstance,
        NULL
    );
    SendMessage(g_controls.inputBox, WM_SETFONT, (WPARAM)g_controls.fontNormal, TRUE);
    
    /* -------- 按钮 -------- */
    /* 开始按钮 */
    g_controls.startButton = CreateWindowExW(
        0, L"BUTTON", L"▶ 开始输入 (F5)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        PADDING, HEADER_HEIGHT + PADDING + 260,
        180, 45,
        parentWindow,
        (HMENU)ID_START_BUTTON,
        g_appInstance,
        NULL
    );
    SendMessage(g_controls.startButton, WM_SETFONT, (WPARAM)g_controls.fontBold, TRUE);
    
    /* 停止按钮（初始禁用），放在开始按钮旁边 */
    g_controls.stopButton = CreateWindowExW(
        0, L"BUTTON", L"⏹ 停止输入 (ESC)",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,  /* WS_DISABLED：禁用 */
        PADDING + 200, HEADER_HEIGHT + PADDING + 260,
        180, 45,
        parentWindow,
        (HMENU)ID_STOP_BUTTON,
        g_appInstance,
        NULL
    );
    SendMessage(g_controls.stopButton, WM_SETFONT, (WPARAM)g_controls.fontBold, TRUE);
    
    /* -------- 速度控制 -------- */
    /* 滑块 - 显示在停止按钮右侧 */
    g_controls.speedSlider = CreateWindowExW(
        0,
        L"msctls_trackbar32",  /* 滑块控件类名 */
        L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS,  /* 添加 WS_VISIBLE 显示控件 */
        PADDING + 400, HEADER_HEIGHT + PADDING + 265,  /* 位置：停止按钮右侧 */
        150, 30,  /* 大小 */
        parentWindow,
        (HMENU)ID_SPEED_SLIDER,
        g_appInstance,
        NULL
    );
    /* 设置滑块范围：10-200毫秒 */
    SendMessage(g_controls.speedSlider, TBM_SETRANGE, TRUE, MAKELPARAM(10, 200));
    /* 设置初始位置：50毫秒 */
    SendMessage(g_controls.speedSlider, TBM_SETPOS, TRUE, 50);
    /* 设置刻度频率：每10个单位一个刻度 */
    SendMessage(g_controls.speedSlider, TBM_SETTICFREQ, 10, 0);
    
    /* 速度标签 - 显示在滑块右侧 */
    g_controls.speedLabel = CreateWindowExW(
        0, L"STATIC", L"延迟：50ms",
        WS_CHILD | WS_VISIBLE,  /* 添加 WS_VISIBLE 显示控件 */
        PADDING + 560, HEADER_HEIGHT + PADDING + 270,  /* 位置：滑块右侧 */
        100, 25,  /* 大小 */
        parentWindow,
        (HMENU)ID_SPEED_LABEL,
        g_appInstance,
        NULL
    );
    SendMessage(g_controls.speedLabel, WM_SETFONT, (WPARAM)g_controls.fontNormal, TRUE);
    
    /* -------- 状态和进度 -------- */
    /* 状态标签 - 位置随按钮上移 */
    g_controls.statusLabel = CreateWindowExW(
        0, L"STATIC", L"状态：就绪",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        PADDING, HEADER_HEIGHT + PADDING + 320,
        400, 25,
        parentWindow,
        (HMENU)ID_STATUS_LABEL,
        g_appInstance,
        NULL
    );
    SendMessage(g_controls.statusLabel, WM_SETFONT, (WPARAM)g_controls.fontNormal, TRUE);
    
    /* 进度条 - 位置随按钮上移 */
    g_controls.progressBar = CreateWindowExW(
        0, L"msctls_progress32",  /* 进度条控件类名 */
        L"",
        WS_CHILD | WS_VISIBLE | PBS_SMOOTH,  /* 平滑样式 */
        PADDING, HEADER_HEIGHT + PADDING + 350,
        WINDOW_WIDTH - PADDING * 2, 20,
        parentWindow,
        (HMENU)ID_PROGRESS_BAR,
        g_appInstance,
        NULL
    );
    /* 设置进度条范围：0-100 */
    SendMessage(g_controls.progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(g_controls.progressBar, PBM_SETPOS, 0, 0);
    
    /* -------- 底部提示栏 -------- */
    HWND hintLabel = CreateWindowExW(
        0, L"STATIC",
        L"快捷键：F5 = 开始 | ESC = 紧急停止 | 支持中英文混合输入 | 自动识别回车换行",
        WS_CHILD | WS_VISIBLE | SS_CENTER,  /* 居中对齐 */
        0, WINDOW_HEIGHT - FOOTER_HEIGHT + 10,
        WINDOW_WIDTH, 25,
        parentWindow,
        NULL,
        g_appInstance,
        NULL
    );
    SendMessage(hintLabel, WM_SETFONT, (WPARAM)g_controls.fontNormal, TRUE);
    
    /* 注册全局热键 F5 */
    RegisterHotKey(parentWindow, 1, 0, VK_F5);
    
    /* 启动定时器，用于检测ESC键（每50毫秒检查一次） */
    SetTimer(parentWindow, 1, 50, NULL);
}

/* ==================== 输入模拟函数实现 ==================== */

/*
 * 判断字符是否是中文字符
 * 中文Unicode范围：
 *   基本汉字：0x4E00 - 0x9FFF
 *   扩展A区：0x3400 - 0x4DBF
 *   兼容汉字：0xF900 - 0xFAFF
 */
bool IsChinese(wchar_t ch) {
    /* ?? Unicode ??????????????? */
    return (ch >= 0x4E00 && ch <= 0x9FFF) ||
           (ch >= 0x3400 && ch <= 0x4DBF) ||
           (ch >= 0xF900 && ch <= 0xFAFF);
}

/*
 * 模拟按下并释放一个键
 */
void PressKey(WORD keyCode) {
    /* ??????????????????? */
    /* INPUT 结构体用于 SendInput 函数 */
    INPUT input = {0};
    
    /* 按下键 */
    input.type = INPUT_KEYBOARD;    /* 输入类型：键盘 */
    input.ki.wVk = keyCode;         /* 虚拟键码 */
    SendInput(1, &input, sizeof(INPUT));
    
    /* 短暂延迟，模拟真实按键 */
    Sleep(5);
    
    /* 释放键 */
    input.ki.dwFlags = KEYEVENTF_KEYUP;  /* 标志：按键释放 */
    SendInput(1, &input, sizeof(INPUT));
}

/*
 * 输入一个字符（使用Unicode）
 * 用于输入中文字符
 */
void TypeChar(wchar_t ch) {
    /* ???? Unicode ????????????????????? */
    INPUT input[2] = {0};
    
    /* 按下 */
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wScan = ch;              /* 扫描码 = Unicode字符 */
    input[0].ki.dwFlags = KEYEVENTF_UNICODE;  /* Unicode模式 */
    
    /* 释放 */
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wScan = ch;
    input[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

/*
 * 模拟回车键
 */
void TypeEnter(void) {
    /* ???????????????? */
    INPUT input[2] = {0};
    
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_RETURN;  /* 回车键 */
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_RETURN;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

/*
 * 模拟Tab键（用于缩进）
 */
void TypeTab(void) {
    /* ???? Tab ??????????? */
    INPUT input[2] = {0};
    
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_TAB;  /* Tab键 */
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_TAB;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

/*
 * 模拟空格键
 */
void TypeSpace(void) {
    /* ??????????????????? */
    INPUT input[2] = {0};
    
    input[0].type = INPUT_KEYBOARD;
    input[0].ki.wVk = VK_SPACE;  /* 空格键 */
    
    input[1].type = INPUT_KEYBOARD;
    input[1].ki.wVk = VK_SPACE;
    input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    
    SendInput(2, input, sizeof(INPUT));
}

/* ==================== 输入控制函数实现 ==================== */

/*
 * 输入线程函数
 * 这是一个独立的线程，用于执行实际的输入操作
 * 这样界面不会卡住
 */
DWORD WINAPI InputThread(LPVOID param) {
    HWND window = (HWND)param;
    /*
     * ????????????????
     * ???????????????????????????
     */
    
    /* 获取输入框中的文字长度 */
    int len = GetWindowTextLengthW(g_controls.inputBox);
    if (len == 0) {
        PostMessage(window, MSG_INPUT_DONE, 0, 0);
        return 0;
    }
    
    /* 分配内存存储文字 */
    wchar_t* text = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    GetWindowTextW(g_controls.inputBox, text, len + 1);

    /* ????????????????????????????????? */
    
    /* 更新状态 */
    EnterCriticalSection(&g_state.cs);
    g_state.totalChars = len;
    g_state.currentChar = 0;
    g_state.isRunning = true;
    g_state.shouldStop = false;
    LeaveCriticalSection(&g_state.cs);
    
    /* 等待1秒，让用户有时间切换到目标窗口 */
    Sleep(1000);
    /* ?? 1 ???????????????????? */
    
    /* 逐个字符输入 */
    for (int i = 0; i < len; i++) {
        /* ????????????????????? */
        /* 检查是否应该停止 */
        EnterCriticalSection(&g_state.cs);
        if (g_state.shouldStop) {
            LeaveCriticalSection(&g_state.cs);
            break;
        }
        g_state.currentChar = i + 1;
        int progress = (int)((double)(i + 1) / len * 100);
        LeaveCriticalSection(&g_state.cs);
        
        /* 发送进度更新消息到界面 */
        PostMessage(window, MSG_INPUT_PROGRESS, progress, i + 1);
        
        /* 获取当前字符 */
        wchar_t ch = text[i];

        /* ???????????????Tab???????????? */
        
        /* 跳过 Windows 换行符中的 \r（回车） */
        if (ch == L'\r') {
            continue;
        }
        
        /* 根据字符类型选择输入方式 */
        if (ch == L'\n') {
            /* 换行符 -> 回车键 */
            TypeEnter();
        } else if (ch == L'\t') {
            /* Tab符 -> Tab键 */
            TypeTab();
        } else if (ch == L' ') {
            /* 空格 -> 空格键 */
            TypeSpace();
        } else if (IsChinese(ch)) {
            /* 中文字符 -> Unicode输入 */
            TypeChar(ch);
        } else {
            /* 其他字符（英文、数字、符号等）-> 键盘模拟 */
            SHORT vk = VkKeyScanW(ch);
            if (vk != -1) {
                /* ?????????????????? Shift/Ctrl/Alt ??????? */
                BYTE vkCode = LOBYTE(vk);      /* 虚拟键码 */
                BYTE shiftState = HIBYTE(vk);  /* Shift/Ctrl/Alt状态 */
                
                /* 如果需要，按下修饰键 */
                if (shiftState & 1) keybd_event(VK_SHIFT, 0, 0, 0);
                if (shiftState & 2) keybd_event(VK_CONTROL, 0, 0, 0);
                if (shiftState & 4) keybd_event(VK_MENU, 0, 0, 0);
                
                /* 按下并释放主键 */
                PressKey(vkCode);
                
                /* 释放修饰键 */
                if (shiftState & 1) keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
                if (shiftState & 2) keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                if (shiftState & 4) keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
            } else {
                /* 如果键盘映射失败，使用Unicode输入 */
                TypeChar(ch);
            }
        }
        
        /* 延迟一段时间（可调节） */
        EnterCriticalSection(&g_state.cs);
        int currentDelay = g_state.delayMs;
        LeaveCriticalSection(&g_state.cs);
        
        Sleep(currentDelay);
        /* ????????????????????????? */
    }
    
    /* 释放内存 */
    free(text);
    
    /* 更新状态 */
    EnterCriticalSection(&g_state.cs);
    g_state.isRunning = false;
    bool wasStopped = g_state.shouldStop;
    LeaveCriticalSection(&g_state.cs);
    
    /* 发送完成消息 */
    PostMessage(window, MSG_INPUT_DONE, wasStopped ? 1 : 0, 0);
    
    return 0;
}

/*
 * 开始输入
 */
void StartTyping(HWND window) {
    /* ???????????????????????? */
    /* 如果已经在运行，不重复启动 */
    if (g_state.isRunning) return;
    
    /* 创建新线程执行输入 */
    DWORD threadId;
    g_state.threadHandle = CreateThread(
        NULL,           /* 默认安全属性 */
        0,              /* 默认堆栈大小 */
        InputThread,    /* 线程函数 */
        window,         /* 传递给线程的参数 */
        0,              /* 默认创建标志 */
        &threadId       /* 返回线程ID */
    );
    
    if (g_state.threadHandle) {
        /* 更新界面 */
        EnableWindow(g_controls.startButton, FALSE);  /* 禁用开始按钮 */
        EnableWindow(g_controls.stopButton, TRUE);    /* 启用停止按钮 */
        EnableWindow(g_controls.inputBox, FALSE);     /* 禁用输入框 */
        UpdateStatus(L"状态：正在输入...");
        UpdateProgress(0);
    }
}

/*
 * 停止输入
 */
void StopTyping(void) {
    /* ????????????????????????? */
    /* 设置停止标志 */
    EnterCriticalSection(&g_state.cs);
    g_state.shouldStop = true;
    LeaveCriticalSection(&g_state.cs);
    
    /* 等待线程结束（最多2秒） */
    if (g_state.threadHandle) {
        WaitForSingleObject(g_state.threadHandle, 2000);
        CloseHandle(g_state.threadHandle);
        g_state.threadHandle = NULL;
    }
    
    /* 恢复界面 */
    EnableWindow(g_controls.startButton, TRUE);
    EnableWindow(g_controls.stopButton, FALSE);
    EnableWindow(g_controls.inputBox, TRUE);
    UpdateStatus(L"状态：已停止");
}

/* ==================== 界面更新函数实现 ==================== */

void UpdateSpeedLabel(int delay) {
    /* ????????????????? */
    wchar_t text[64];
    swprintf(text, 64, L"输入延迟：%d 毫秒", delay);
    SetWindowTextW(g_controls.speedLabel, text);
}

void UpdateStatus(const wchar_t* text) {
    /* ???????????????????????? */
    SetWindowTextW(g_controls.statusLabel, text);
}

void UpdateProgress(int percent) {
    /* ????????? */
    SendMessage(g_controls.progressBar, PBM_SETPOS, percent, 0);
}

/* ==================== 窗口消息处理 ==================== */

/*
 * 窗口过程函数
 * 处理窗口收到的各种消息（点击、键盘、鼠标等）
 */
LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    /* ?? Win32 ????????????? */
    switch (message) {
        /* 窗口创建时 */
        case WM_CREATE:
            CreateAllControls(window);
            return 0;
        
        /* 定时器消息（用于检测ESC键） */
        case WM_TIMER:
            /* ?????? ESC???????? */
            if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {  /* 检查ESC是否被按下 */
                if (g_state.isRunning) {
                    StopTyping();
                }
            }
            return 0;
        
        /* 滑块位置改变 */
        case WM_HSCROLL:
            /* ???????????????????? */
            if ((HWND)lParam == g_controls.speedSlider) {
                int pos = (int)SendMessage(g_controls.speedSlider, TBM_GETPOS, 0, 0);
                EnterCriticalSection(&g_state.cs);
                g_state.delayMs = pos;
                LeaveCriticalSection(&g_state.cs);
                UpdateSpeedLabel(pos);
            }
            return 0;
        
        /* 命令消息（按钮点击等） */
        case WM_COMMAND:
            /* ???????????????????? */
            switch (LOWORD(wParam)) {
                case ID_START_BUTTON:
                    StartTyping(window);
                    break;
                case ID_STOP_BUTTON:
                    StopTyping();
                    break;
                case ID_INPUT_BOX:
                    if (HIWORD(wParam) == EN_CHANGE) {  /* 文字改变 */
                        int len = GetWindowTextLengthW(g_controls.inputBox);
                        EnableWindow(g_controls.startButton, len > 0);
                    }
                    break;
            }
            return 0;
        
        /* 全局热键 */
        case WM_HOTKEY:
            /* ???? F5 ????? */
            if (wParam == 1 && !g_state.isRunning) {  /* F5 */
                StartTyping(window);
            }
            return 0;
        
        /* 输入完成消息 */
        case MSG_INPUT_DONE:
            /* ?????????????????????? */
            EnableWindow(g_controls.startButton, TRUE);
            EnableWindow(g_controls.stopButton, FALSE);
            EnableWindow(g_controls.inputBox, TRUE);
            UpdateStatus(wParam ? L"状态：已紧急停止" : L"状态：输入完成");
            UpdateProgress(wParam ? 0 : 100);
            if (g_state.threadHandle) {
                CloseHandle(g_state.threadHandle);
                g_state.threadHandle = NULL;
            }
            return 0;
        
        /* 进度更新消息 */
        case MSG_INPUT_PROGRESS:
            /* ??????????????????????? */
            UpdateProgress((int)wParam);
            wchar_t status[256];
            swprintf(status, 256, L"状态：正在输入... %d/%d (%d%%)",
                     (int)lParam, g_state.totalChars, (int)wParam);
            UpdateStatus(status);
            return 0;
        
        /* 控件颜色设置 */
        case WM_CTLCOLORSTATIC: {
            /* ?????????????????? */
            HDC hdc = (HDC)wParam;
            HWND ctrl = (HWND)lParam;
            
            /* 获取控件文字，判断是否是底部提示栏 */
            wchar_t text[256];
            GetWindowTextW(ctrl, text, 256);
            if (wcsstr(text, L"快捷键") != NULL) {
                SetBkColor(hdc, COLOR_HEADER);
                SetTextColor(hdc, COLOR_WHITE);
                return (LRESULT)g_controls.brushHeader;
            }
            
            SetBkColor(hdc, COLOR_BACKGROUND);
            SetTextColor(hdc, COLOR_TEXT_DARK);
            return (LRESULT)g_controls.brushBg;
        }
        
        /* 绘制窗口 */
        case WM_PAINT: {
            /* ????????????????????? */
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(window, &ps);
            
            RECT rect;
            GetClientRect(window, &rect);
            
            /* 填充背景 */
            FillRect(hdc, &rect, g_controls.brushBg);
            
            /* 绘制标题栏 */
            RECT headerRect = {0, 0, rect.right, HEADER_HEIGHT};
            FillRect(hdc, &headerRect, g_controls.brushHeader);
            
            /* 绘制标题文字 */
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, COLOR_WHITE);
            SelectObject(hdc, g_controls.fontTitle);
            DrawTextW(hdc, L"智能自动输入工具", -1, &headerRect,
                     DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            
            /* 绘制底部栏 */
            RECT footerRect = {0, rect.bottom - FOOTER_HEIGHT, rect.right, rect.bottom};
            FillRect(hdc, &footerRect, g_controls.brushHeader);
            
            EndPaint(window, &ps);
            return 0;
        }
        
        /* 不擦除背景（避免闪烁） */
        case WM_ERASEBKGND:
            return 1;
        
        /* 窗口关闭 */
        case WM_DESTROY:
            /* ??????????????? GDI ??? */
            UnregisterHotKey(window, 1);
            KillTimer(window, 1);
            
            if (g_state.threadHandle) {
                StopTyping();
            }
            
            /* 删除资源 */
            DeleteObject(g_controls.fontTitle);
            DeleteObject(g_controls.fontNormal);
            DeleteObject(g_controls.fontBold);
            DeleteObject(g_controls.brushBg);
            DeleteObject(g_controls.brushHeader);
            
            PostQuitMessage(0);
            return 0;
        
        /* 其他消息使用默认处理 */
        default:
            return DefWindowProc(window, message, wParam, lParam);
    }
}

/* ==================== 程序入口点 ==================== */

/*
 * wWinMain：Windows GUI程序的入口点
 * hInstance：应用程序实例句柄
 * nCmdShow：窗口显示方式
 */
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
    /* Win32 ????????????????????? */
    /* 保存应用程序实例 */
    g_appInstance = hInstance;
    
    /* 初始化输入状态 */
    InitInputState();
    
    /* 初始化通用控件库（进度条、滑块等需要） */
    InitCommonControls();
    
    /* -------- 注册窗口类 -------- */
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);    /* 结构体大小 */
    wc.lpfnWndProc = WindowProcedure;   /* 窗口过程函数 */
    wc.hInstance = hInstance;           /* 应用程序实例 */
    wc.lpszClassName = L"AutoInputTool"; /* 窗口类名 */
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  /* 背景色 */
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);  /* 鼠标指针 */
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION); /* 图标 */
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION); /* 小图标 */
    
    if (!RegisterClassExW(&wc)) {
        MessageBoxW(NULL, L"窗口类注册失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    /* -------- 创建窗口 -------- */
    /* 计算窗口居中位置 */
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - WINDOW_WIDTH) / 2;
    int y = (screenHeight - WINDOW_HEIGHT) / 2;
    
    HWND window = CreateWindowExW(
        0,                          /* 扩展样式 */
        L"AutoInputTool",           /* 窗口类名 */
        L"智能自动输入工具",         /* 窗口标题 */
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,  /* 窗口样式 */
        x, y,                       /* 位置 */
        WINDOW_WIDTH, WINDOW_HEIGHT, /* 大小 */
        NULL,                       /* 父窗口 */
        NULL,                       /* 菜单 */
        hInstance,                  /* 应用程序实例 */
        NULL                        /* 附加参数 */
    );
    
    if (!window) {
        MessageBoxW(NULL, L"窗口创建失败！", L"错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    /* 显示窗口 */
    ShowWindow(window, nCmdShow);
    UpdateWindow(window);
    
    /* -------- 消息循环 -------- */
    /* 不断从系统获取消息并处理 */
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);  /* 翻译键盘消息 */
        DispatchMessage(&msg);   /* 分发到窗口过程 */
    }
    
    /* 清理 */
    CleanupInputState();
    
    return (int)msg.wParam;
}

