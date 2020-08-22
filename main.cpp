#include <windows.h>
#include <gdiplus.h>
#include <shellscalingapi.h>
#include "resource.h"
#include "DrawType.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Shcore.lib")

typedef enum tagMode {
	BACKSTAGE,				// 后台模式
	START,					// 开始模式
	SELECT					// 选中模式
} mode;

// 声明函数体
void InitTray(HINSTANCE hInstance, HWND hWnd);
void paint_text(HDC hMemDC, DT_text *data);
void paint_rectangle(HDC hMemDC, DT_rectangle *data);
void paint_image(HDC hMemDC, DT_image *data);
void paint(HWND hWnd);
void LoadFullScreenShotIntoMemory();
void createTopWindow();
void releaseBackstageResources();
void releaseSelectResources();
void paint_background(HWND hWnd);
void paint_selectRect(const POINT rb_point);
void paint_info(const POINT rb_point);
void outputScreenShot(const wchar_t* saveFileName);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
ATOM setRegisterClassStruct(HINSTANCE hInstance);
void initGlobalResources(HINSTANCE m_hInstance);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow);
void ScreenCapture(RECT &rect, const wchar_t* fileName);
PBITMAPINFOHEADER ScreenCapture(RECT &rect, PBITMAPINFO &pbmi, LPBYTE &lpBits);

// 消息宏
#define WM_TRAY				(WM_USER + 1)
#define UR_QUIT				(WM_USER + 2)
#define HK_START			(WM_USER + 3)

// 全局资源
NOTIFYICONDATA						nid;								// 托盘属性
HMENU								hMenu;								// 托盘菜单
const wchar_t*						pClassName = L"screenshot";			// 窗口组名
HINSTANCE							g_hInstance;						// 全局实例句柄
int									desktop_width;						// 全局窗口大小（width）
int									desktop_height;						// 全局窗口大小（height）
HWND								child_hwnd;							// 子窗口句柄
mode								ctn_mode;							// 控制变量, 区分模式类别
bool								ctn_isSelect;						// 控制变量，判断是否进入选中模式
POINT								lt_point;							// left/top左上角坐标
POINT								rb_point;							// right/bottom右下角坐标
const int							drawNum = 13;						// 绘制数量，可扩展
DrawType*							drawData[drawNum];					// 绘制数据
ULONG_PTR							m_gdiplusToken;						// GDI初始化变量
Gdiplus::GdiplusStartupInput		gdiplusStartupInput;				// GDI初始化变量
PBITMAPINFO							pbmi;								// 全屏位图信息
LPBYTE								lpBits;								// 全屏位图数据
const wchar_t*						saveFileName = 
		L"C:/Users/Administrator/Desktop/screenshot.bmp";				// 保存位置
wchar_t*							maskFileName = L"./mask.png";		// 灰色遮罩图片位置
wchar_t*							tipInfo =							// 提示信息
		L"截屏模式, Esc退出截屏模式， Enter生成截屏图片"; 
wchar_t buf_pos[32];
wchar_t buf_rgb[32];

/*
	函数：void InitTray(HINSTANCE hInstance, HWND hWnd)
	功能：实例化托盘
	参数：
		1、HINSTANCE hInstance：窗口实例句柄
		2、HWND hWnd：窗口句柄
*/
void InitTray(HINSTANCE hInstance, HWND hWnd)
{
	/* 设置托盘属性 nid , Header：<shellapi.h> */
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;	// 设置托盘程序句柄
	nid.uID = IDI_ICON1;	// 设置托盘图标句柄
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	nid.uCallbackMessage = WM_TRAY; // 设置消息回调事件
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));	// 加载图标
	lstrcpy(nid.szTip, pClassName);	// 设置提示信息

	hMenu = CreatePopupMenu();//生成托盘菜单
	AppendMenu(hMenu, 0, 0, L"关于");
	AppendMenu(hMenu, 0, 0, L"热键启动(Alt+z)");	
	AppendMenu(hMenu, MF_STRING, ID_EXIT, L"退出");	// 菜单栏添加"退出"按键，回调消息为ID_EXIT
	Shell_NotifyIcon(NIM_ADD, &nid);
}

/*
	函数：void paint_text(HDC hMemDC, DT_text *data)
	功能：窗口绘制位图
	参数：
		1、HDC hMemDC：上下文显示设备
		2、DT_text *data：文字类资源
*/
void paint_text(HDC hMemDC, DT_text *data)
{
	DT_text* dt_text = data;
	// 设置文字字体信息 
	LOGFONT logfont;
	ZeroMemory(&logfont, sizeof(LOGFONT));
	logfont.lfCharSet = GB2312_CHARSET;			// 字体样式
	logfont.lfHeight = -1 * dt_text->getH();	// 高度
	HFONT hFont = CreateFontIndirect(&logfont); // 字体类型
	SelectObject(hMemDC, hFont);				// 将字体选入内存设备中
	// 设置字体模式、颜色 
	SetBkMode(hMemDC, dt_text->getMode());		// 设置背景透明
	SetTextColor(hMemDC, dt_text->getColor());	// 设置字体颜色
	// 输出文字到设备中
	TextOutW(
		hMemDC,						// 设备内存
		dt_text->getX(), dt_text->getY(),		// 起始位置
		dt_text->getData(),				// 宽字符的字符串
		lstrlen(dt_text->getData())		// 字符串长度
	);
	// 释放资源
	DeleteObject(hFont);
}

/*
	函数：void paint_rectangle(HDC hMemDC, DT_rectangle *data)
	功能：窗口绘制位图
	参数：
		1、HDC hMemDC：上下文显示设备
		2、DT_rectangle *data：矩形类资源
*/
void paint_rectangle(HDC hMemDC, DT_rectangle *data)
{
	DT_rectangle* dt_rectangle = data;
	// 设置绘制矩形的边框样式、边框粗细、边框颜色 
	HPEN hPen = ::CreatePen(dt_rectangle->getStyle(), dt_rectangle->getThick(), dt_rectangle->getColor());
	// 设置矩形内填充画刷 
	HBRUSH hBrush = (HBRUSH)::GetStockObject(dt_rectangle->getI());
	// 将hPen、hBrush选入内存中
	::SelectObject(hMemDC, hPen);
	::SelectObject(hMemDC, hBrush);
	Rectangle(
		hMemDC,						// 绘制内存设备句柄
		dt_rectangle->getL(),		// left
		dt_rectangle->getT(),		// top
		dt_rectangle->getR(),		// right
		dt_rectangle->getB()		// bottom
	);
	// 释放资源
	DeleteObject(hPen);
	DeleteObject(hBrush);
}

/*
	函数：void paint_image(HDC hMemDC, DT_image *data)
	功能：窗口绘制位图
	参数：
		1、HDC hMemDC：上下文显示设备
		2、DT_image *data：位图类资源
*/
void paint_image(HDC hMemDC, DT_image *data)
{
	DT_image* dt_image = data;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL); // GDI启动绘制
	Gdiplus::Graphics graphics(hMemDC); // 创建一个画布
	graphics.DrawImage(
		dt_image->getImg(),							// 需要绘制的位图
		dt_image->getX(), dt_image->getY(),			// 绘制到窗口的左上角坐标位置
		dt_image->getW(), dt_image->getH()			// 绘制深度和高度（此处有可能发生缩放效果）
	);
}

/*
	函数：void paint(HWND hWnd)
	功能：窗口重绘
	参数：
		1、HWND hWnd：窗口句柄
*/
void paint(HWND hWnd)
{
	if (hWnd == child_hwnd) {	// 只针对子窗口进行重绘
		// 获取窗口区域Rect
		RECT rtClient;
		GetClientRect(hWnd, &rtClient);
		/* 利用win32 双缓冲技术绘图技术，避免位图一帧一帧显示，解决窗口刷新频率过快 */
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hWnd, &ps);
		HDC hMemDC = ::CreateCompatibleDC(hdc); // 创建当前上下文的兼容dc(内存DC)
		HBITMAP hBitmap = ::CreateCompatibleBitmap(
			hdc,
			rtClient.right - rtClient.left,
			rtClient.bottom - rtClient.top
		); // 创建当前上下文的兼容位图
		::SelectObject(hMemDC, hBitmap); // 将位图加载到内存DC

		// 遍历将DrawData数据绘制到hMemDC内存中
		for (int i = 0; i < drawNum; i++) {
			if (drawData[i] == NULL)
				continue;

			switch (drawData[i]->getId()) { 	// 判断绘制类型
			case IMAGE: {						// 绘制位图
				paint_image(hMemDC, (DT_image*)drawData[i]);
				break;
			}
			case RECTANGLE: {		// 绘制矩形
				paint_rectangle(hMemDC, (DT_rectangle*)drawData[i]);
				break;
			}
			case TEXT: {			// 绘制文字
				paint_text(hMemDC, (DT_text*)drawData[i]);
				break;
			}
			}
		}
		// 将内存中的所有内容显示到窗口
		BitBlt(
			hdc,
			0, 0,
			rtClient.right - rtClient.left, rtClient.bottom - rtClient.top,
			hMemDC,
			0, 0,
			SRCCOPY
		);
		// 释放资源
		DeleteDC(hMemDC);
		DeleteObject(hBitmap);
		// 结束绘制
		EndPaint(hWnd, &ps);
	}
}

/*
	函数：void LoadFullScreenShotIntoMemory()
	功能：将全屏截图加载入内存中
	参数：无
*/
void LoadFullScreenShotIntoMemory()
{
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = desktop_width;
	rect.bottom = desktop_height;

	ScreenCapture(rect, pbmi, lpBits);
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL); // GDI启动绘制
	Gdiplus::Bitmap img(pbmi, lpBits);
	drawData[0] = new DT_image(0, 0, desktop_width, desktop_height, &img);
}

/*
	函数：void createTopWindow()
	功能：创建一个顶级窗口
	参数：无
*/
void createTopWindow()
{
	child_hwnd = CreateWindowEx(
		WS_EX_TOPMOST,							// 窗口的扩展风格
		pClassName,								// 指向注册类名的指针
		L"child",								// 指向窗口名称的指针
		WS_VISIBLE | WS_POPUP,					// 窗口风格
		0, 0, desktop_width, desktop_height,	// 窗口的 x,y 坐标以及宽高
		NULL,									// 父窗口的句柄
		NULL,									// 菜单的句柄
		g_hInstance,							// 应用程序实例的句柄
		NULL									// 指向窗口的创建数据
	);
	/* 去除子窗口标题栏、菜单、最大化最小化导航栏等 */
	::SetWindowLong(child_hwnd, GWL_STYLE, GetWindowLong(child_hwnd, GWL_STYLE) & (~(WS_CAPTION | WS_SYSMENU | WS_SIZEBOX)));
	::SetWindowLong(child_hwnd, GWL_EXSTYLE, GetWindowLong(child_hwnd, GWL_EXSTYLE) & (~(WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME)) | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
}

/*
	函数：void releaseBackstageResources()
	功能：释放后台模式下创建的资源
	参数：无
*/
void releaseBackstageResources()
{
	// 释放后台资源
	delete drawData[1];
	delete drawData[2];
	delete drawData[3];
	GlobalFree(pbmi);
	GlobalFree(lpBits);
}

/*
	函数：void releaseSelectModeResources(HWND hWnd)
	功能：释放选择模式下创建的资源
	参数：
		1、HWND hWnd：窗口句柄
*/
void releaseSelectResources()
{
	ctn_mode = BACKSTAGE;					// 转变成后台模式
	for (int i = 0; i < drawNum; i++) {
		if (i == 1 || i == 2 || i == 3)		// 下标为1、2、3的为后台常驻资源，不需要释放
			continue;
		delete drawData[i];
		drawData[i] = NULL;
	}
}

/*
	函数：void paint_background(HWND hWnd)
	功能: 重绘背景
	参数：
		1、HWND hWnd：窗口句柄
*/
void paint_background(HWND hWnd)
{
	InvalidateRect(hWnd, NULL, true);
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	DT_image *image = (DT_image*)drawData[0];
	Gdiplus::Graphics graphics(hdc);
	graphics.DrawImage(image->getImg(), 0, 0, desktop_width, desktop_height);
	EndPaint(hWnd, &ps);
}

/*
	函数：void paint_selectRect()
	功能: 绘制选中区域的背景色覆盖灰色遮罩
	参数：无
*/
void paint_selectRect(const POINT rb_point)
{
	/* 设置选中区域黑色边框 */
	drawData[4] == NULL ?
		drawData[4] = new DT_rectangle(lt_point.x, lt_point.y, rb_point.x,
			rb_point.y, RGB(255, 255, 255), 1, NULL_BRUSH, PS_DASHDOTDOT) :
			((DT_rectangle*)drawData[4])->setData(
				lt_point.x, lt_point.y, rb_point.x, rb_point.y);

	/* 设置选中区域白色边框 */
	drawData[5] == NULL ?
		drawData[5] = new DT_rectangle(lt_point.x - 1, lt_point.y - 1, rb_point.x + 1,
			rb_point.y + 1, RGB(0, 0, 0), 1, NULL_BRUSH, PS_DASHDOTDOT) :
			((DT_rectangle*)drawData[5])->setData(
				lt_point.x - 1, lt_point.y - 1, rb_point.x + 1, rb_point.y + 1);

	/* 设置选中图片区域原图覆盖灰色遮罩 */
	drawData[6] == NULL ?
		drawData[6] = new DT_image(
			lt_point.x, lt_point.y,
			rb_point.x - lt_point.x, rb_point.y - lt_point.y,
			((DT_image*)drawData[0])->getImg()) :
			((DT_image*)drawData[6])->setData(
				lt_point.x, lt_point.y,
				rb_point.x - lt_point.x, rb_point.y - lt_point.y,
				((DT_image*)drawData[0])->getImg(),
				lt_point.x, lt_point.y,
				rb_point.x - lt_point.x, rb_point.y - lt_point.y);
}

/*
	函数：void paint_info(const POINT rb_point)
	功能: 绘制鼠标当前坐标等位置、颜色等信息
	参数：
		1、HWND hWnd：窗口句柄
*/
void paint_info(const POINT rb_point)
{
	const int radius = 25;			// 真实框半径25pix
	const int interval = 50;		// 显示框与鼠标横纵间隔为50pix
	const int frameSize = 150;		// 显示框（正方形）的横纵size为150pix
	const int textBoxWidth = 50;	// 文字框高度为50pix
	const int textSize = 17;		// 字体大小

									/* 绘制放大框图片 */
	drawData[7] == NULL ?
		drawData[7] = new DT_image(
			rb_point.x + interval, rb_point.y + interval,
			frameSize, frameSize,
			((DT_image*)drawData[0])->getImg(),
			rb_point.x - radius, rb_point.y - radius, 2 * radius, 2 * radius) :
			((DT_image*)drawData[7])->setData(
				rb_point.x + interval, rb_point.y + interval,
				frameSize, frameSize,
				((DT_image*)drawData[0])->getImg(),
				rb_point.x - radius, rb_point.y - radius, 2 * radius, 2 * radius);

	/* 绘制竖十字架 */
	drawData[8] == NULL ?
		drawData[8] = new DT_rectangle(
			rb_point.x + interval + frameSize / 2, rb_point.y + interval,
			rb_point.x + interval + frameSize / 2 + 2, rb_point.y + interval + frameSize,
			RGB(0, 255, 0), 2, NULL_BRUSH, PS_SOLID) :
			((DT_rectangle*)drawData[8])->setData(rb_point.x + interval + frameSize / 2, rb_point.y + interval,
				rb_point.x + interval + frameSize / 2 + 2, rb_point.y + interval + frameSize);

	/* 绘制横十字架 */
	drawData[9] == NULL ?
		drawData[9] = new DT_rectangle(
			rb_point.x + interval, rb_point.y + interval + frameSize / 2,
			rb_point.x + interval + frameSize, rb_point.y + interval + frameSize / 2 + 2,
			RGB(0, 255, 0), 2, NULL_BRUSH, PS_SOLID) :
			((DT_rectangle*)drawData[9])->setData(
				rb_point.x + interval, rb_point.y + interval + frameSize / 2,
				rb_point.x + interval + frameSize, rb_point.y + interval + frameSize / 2 + 2);

	/* 绘制文字区域背景 */
	drawData[10] == NULL ?
		drawData[10] = new DT_rectangle(
			rb_point.x + interval, rb_point.y + interval + frameSize,
			rb_point.x + interval + frameSize, rb_point.y + interval + frameSize + textBoxWidth,
			RGB(0, 0, 0), 1, BLACK_BRUSH, PS_SOLID) :
			((DT_rectangle*)drawData[10])->setData(
				rb_point.x + interval, rb_point.y + interval + frameSize,
				rb_point.x + interval + frameSize, rb_point.y + interval + frameSize + textBoxWidth);

	/* 绘制POS信息 */
	memset(buf_pos, 0, sizeof(buf_pos));
	wsprintf(buf_pos, L"Pos:(%d,%d)", rb_point.x, rb_point.y);
	drawData[11] == NULL ?
		drawData[11] = new DT_text(
			TRANSPARENT, RGB(255, 255, 255),
			rb_point.x + interval, rb_point.y + interval + frameSize,
			buf_pos, textSize) :
			((DT_text*)drawData[11])->setData(
				rb_point.x + interval, rb_point.y + interval + frameSize, buf_pos);

	/* 绘制RGB信息 */
	Gdiplus::Color color;
	((DT_image*)drawData[0])->getImg()->GetPixel(rb_point.x, rb_point.y, &color);
	memset(buf_rgb, 0, sizeof(buf_rgb));
	wsprintf(buf_rgb, L"RGB:(%d,%d,%d)", color.GetR(), color.GetG(), color.GetB());
	drawData[12] == NULL ?
		drawData[12] = new DT_text(
			TRANSPARENT, RGB(255, 255, 255),
			rb_point.x + interval, rb_point.y + interval + frameSize + textBoxWidth / 2,
			buf_rgb, textSize) :
			((DT_text*)drawData[12])->setData(
				rb_point.x + interval, rb_point.y + interval + frameSize + textBoxWidth / 2, buf_rgb);

}

/*
	函数：void outputScreenShot(const wchar_t* saveFileName = ::saveFileName)
	功能: 输出截图至磁盘
	参数：
		1、const wchar_t* saveFileName：保存文件路径
*/
void outputScreenShot(const wchar_t* saveFileName = ::saveFileName)
{
	/* 调整位置 */
	int width = rb_point.x - lt_point.x;
	int height = rb_point.y - lt_point.y;
	if (width < 0) {
		lt_point.x += width;
		width *= -1;
	}
	if (height < 0) {
		lt_point.y += height;
		height *= -1;
	}
	// 输出截图
	RECT rect;
	rect.left = lt_point.x;
	rect.top = lt_point.y;
	rect.right = lt_point.x + width;
	rect.bottom = lt_point.y + height;
	ScreenCapture(rect, saveFileName);
}

/*
	函数：LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	功能：win32回调函数，用于处理win32窗口中的回调事件
	参数：
		1、HWND hWnd：窗口句柄
		2、UINT uMsg：消息常量标识符 
		3、WPARAM wParam：WPARAM实际上就是UINT,即32位无符号整数，消息的附加信息
		4、LPARAM lParam：LPARAM实际上就是LONG,即32位有符号整数，消息的附加信息
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	/* WM_PAINT：每当窗口发生重绘，都会发送这个消息通知程序 */
	case WM_PAINT: {
		// 重绘窗口
		paint(hWnd);
		break;
	}
	/* WM_HOTKEY 热键消息，当用户按下热键，系统将产生这个消息发送个程序 */
	case WM_HOTKEY: {
		// 响应 WM_TRAY WM_LBUTTONDBLCLK 消息
		PostMessage(hWnd, WM_TRAY, 0, WM_LBUTTONDBLCLK);
		break;
	}
	/* WM_TRAY 用户自定义消息，处理托盘事件 */
	case WM_TRAY: {
		switch (lParam) {
		case WM_RBUTTONDOWN: {
			//获取鼠标坐标
			POINT pt;
			GetCursorPos(&pt);
			//解决在菜单外单击左键菜单不消失的问题
			SetForegroundWindow(hWnd);
			//显示并获取选中的菜单
			int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hWnd, NULL);
			if (cmd == ID_EXIT)
				PostMessage(hWnd, WM_DESTROY, UR_QUIT, NULL);
			break;
		}
		case WM_LBUTTONDBLCLK: {
			if (ctn_mode == BACKSTAGE) { 
				/* 加载光标 */
				//HCURSOR hcur = LoadCursorFromFile(L"cursor1.cur");
				//SetSystemCursor(hcur, 32512);
				
				/* 将全屏位图加载到内存中 */	
				LoadFullScreenShotIntoMemory();

				/*  创建一个顶级窗口 */
				createTopWindow();
				
				// 将模式转变成开始状态
				ctn_mode = START;
			}
			break;
		}
		}
		break;
	}
	/* WM_KEYDOWN 处理用户键盘设备按下事件 */
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_ESCAPE: {								// 键盘按下Esc退出键
			if (hWnd == child_hwnd) {
				/* 释放选择模式下创建的资源 */
				releaseSelectResources();

				/* 关闭窗口 */
				PostMessage(child_hwnd, WM_CLOSE, 0, 0);
			}
			break;
		}
		case VK_RETURN: {					// 键盘按下回车键,保存图片至桌面
			if (ctn_mode == SELECT) {
				/* 用背景覆盖全局 */
				paint_background(hWnd);

				/* 输出截图至磁盘 */
				outputScreenShot();
				
				/* 退出选择模式进入后台模式 */
				PostMessage(child_hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
			}
			break;
		}
		}
		break;
	}
	case WM_RBUTTONDOWN: {		// 右键按下，保存截图，响应 WM_KEYDOWN VK_RETURN 消息
		PostMessage(child_hwnd, WM_KEYDOWN, VK_RETURN, 0);
		break;
	}
	case WM_LBUTTONDOWN: {
		if (hWnd == child_hwnd) {
			// 获取左顶点位置
			GetCursorPos(&lt_point);
			ctn_isSelect = true;
		}
		break; 
	}
	case WM_MOUSEMOVE:{
		// 获取当前鼠标的坐标
		POINT rb_point;
		GetCursorPos(&rb_point);

		/* 绘制鼠标当前坐标等位置、颜色等信息 */
		paint_info(rb_point);

		if (ctn_isSelect) {
			/* 模式转变为选中模式 */
			ctn_mode = SELECT;

			/* 绘制选中区域的背景色覆盖灰色遮罩 */
			paint_selectRect(rb_point);
		}
		// 刷新窗口
		InvalidateRect(child_hwnd, NULL, true);
		UpdateWindow(child_hwnd);
		break;
	}
	case WM_LBUTTONUP: {
		// 获取右顶点位置
		GetCursorPos(&rb_point);
		ctn_isSelect = false;
		break;
	}
	case WM_DESTROY: {
		//窗口销毁时删除托盘
		if (wParam == UR_QUIT) {
			/* 释放后台资源 */
			releaseBackstageResources();

			Shell_NotifyIcon(NIM_DELETE, &nid);
			PostQuitMessage(0);
		}
		break;
	}
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
	函数：int setRegisterClassStruct(HINSTANCE hInstance)
	功能：设置注册类结构体
	参数：
		1、HINSTANCE hInstance：窗口实例句柄
*/
ATOM setRegisterClassStruct(HINSTANCE hInstance)
{
	WNDCLASS wc;
	wc.style = NULL;
	wc.hIcon = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;						// 回调函数为WndProc函数
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = pClassName;					// 设置类名为pClassName
	wc.hCursor = LoadCursor(NULL, IDC_CROSS);		// 加载光标为十字形
	return RegisterClass(&wc);
}
/*
	函数：void initGlobalResources(HINSTANCE m_hInstance)
	功能：初始化全局资源
	参数：
		1、HINSTANCE hInstance：窗口实例句柄
*/

void initGlobalResources(HINSTANCE m_hInstance)
{
	g_hInstance = m_hInstance;
	ctn_mode = BACKSTAGE;
	ctn_isSelect = false;
	desktop_width = GetSystemMetrics(SM_CXSCREEN);
	desktop_height = GetSystemMetrics(SM_CYSCREEN);
	drawData[1] = new DT_image(0, 0, desktop_width, desktop_height, maskFileName, m_gdiplusToken, gdiplusStartupInput);
	drawData[2] = new DT_text(TRANSPARENT, RGB(255, 0, 0), 20, 20, tipInfo, 30);
	drawData[3] = new DT_rectangle(1, 1, desktop_width - 1, desktop_height - 1, RGB(0, 255, 0), 5, NULL_BRUSH, PS_DASHDOTDOT);
	memset(buf_pos, 0, sizeof(buf_pos));
	memset(buf_rgb, 0, sizeof(buf_rgb));
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int iCmdShow)
{
	int ret;
	// 设置进程默认的DPI感知
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	// 初始化全局资源
	initGlobalResources(hInstance);

	// 1. 注册窗体结构体信息
	ret = setRegisterClassStruct(hInstance);
	if (ret == 0)
	{
		MessageBox(NULL, L"setRegisterClassStruct error", NULL, 0);
		return 0;
	}

	// 2. 创建窗口
	HWND MainHwnd = CreateWindowEx(
		WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		pClassName,
		pClassName,
		WS_POPUP,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);
	if (MainHwnd == NULL)
	{
		MessageBox(NULL, L"CreateWindowEx error", NULL, 0);
		return 0;
	}

	// 3. 显示窗口
	ShowWindow(MainHwnd, iCmdShow);
	UpdateWindow(MainHwnd);

	// 4. 实例化托盘
	InitTray(hInstance, MainHwnd);

	// 5. 注册热键 ALT+Z 启动HK_START
	RegisterHotKey(MainHwnd, HK_START, MOD_ALT, 'Z');

	// 6. 消息循环
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

PBITMAPINFOHEADER ScreenCapture(RECT &rect, PBITMAPINFO &pbmi, LPBYTE &lpBits)
{
	BITMAP bmp;
	HDC hDC = GetDC(NULL);
	HDC hdcCompatible = CreateCompatibleDC(hDC);
	HBITMAP hbmScreen = CreateCompatibleBitmap(hDC, rect.right - rect.left, rect.bottom - rect.top);
	if (hbmScreen == NULL)
	{
		MessageBox(NULL, L"CreateCompatibleBitmap error", NULL, 0);
		return NULL;
	}
	if (!SelectObject(hdcCompatible, hbmScreen))
	{
		MessageBox(NULL, L"SelectObject error", NULL, 0);
		return NULL;
	}
	if (!BitBlt(hdcCompatible, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hDC, rect.left, rect.top, SRCCOPY))
	{
		MessageBox(NULL, L"Screen to Compat Blt Failed", NULL, 0);
		return NULL;
	}
	if (!GetObject(hbmScreen, sizeof(BITMAP), (LPSTR)&bmp))
	{
		MessageBox(NULL, L"GetObject()出错！", NULL, 0);
		return NULL;
	}
	WORD cClrBits = (WORD)(bmp.bmPlanes * bmp.bmBitsPixel);
	if (cClrBits == 1)
		cClrBits = 1;
	else if (cClrBits <= 4)
		cClrBits = 4;
	else if (cClrBits <= 8)
		cClrBits = 8;
	else if (cClrBits <= 16)
		cClrBits = 16;
	else if (cClrBits <= 24)
		cClrBits = 24;
	else cClrBits = 32;

	if (cClrBits != 24)
		pbmi = (PBITMAPINFO)GlobalAlloc(LPTR, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * (1 << cClrBits));
	else
		pbmi = (PBITMAPINFO)GlobalAlloc(LPTR, sizeof(BITMAPINFOHEADER));

	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth = bmp.bmWidth;
	pbmi->bmiHeader.biHeight = bmp.bmHeight;
	pbmi->bmiHeader.biPlanes = bmp.bmPlanes;
	pbmi->bmiHeader.biBitCount = bmp.bmBitsPixel;
	if (cClrBits < 24)
		pbmi->bmiHeader.biClrUsed = (1 << cClrBits);
	pbmi->bmiHeader.biCompression = BI_RGB;
	pbmi->bmiHeader.biSizeImage = ((pbmi->bmiHeader.biWidth * cClrBits + 31) & ~31) / 8
		* pbmi->bmiHeader.biHeight;
	pbmi->bmiHeader.biClrImportant = 0;

	PBITMAPINFOHEADER pbih = (PBITMAPINFOHEADER)pbmi;
	lpBits = (LPBYTE)GlobalAlloc(GMEM_FIXED, pbih->biSizeImage);
	if (!lpBits)
	{
		MessageBox(NULL, L"内存分配错误！", NULL, 0);
		return NULL;
	}
	if (!GetDIBits(hDC, hbmScreen, 0, (WORD)pbih->biHeight, lpBits, pbmi, DIB_RGB_COLORS))
	{
		MessageBox(NULL, L"GetDIBits() error", NULL, 0);
		return NULL;
	}
	return pbih;
}

void ScreenCapture(RECT &rect, const wchar_t* fileName)
{
	PBITMAPINFO pbmi;
	LPBYTE lpBits;
	PBITMAPINFOHEADER pbih = ScreenCapture(rect, pbmi, lpBits);

	HANDLE hfile = CreateFile(fileName,
		GENERIC_READ | GENERIC_WRITE,
		(DWORD)0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE)NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, L"创建文件失败", NULL, 0);
		return;
	}
	BITMAPFILEHEADER hdr;
	hdr.bfType = 0x4d42;
	hdr.bfSize = (DWORD)(sizeof(BITMAPFILEHEADER) +
		pbih->biSize + pbih->biClrUsed
		* sizeof(RGBQUAD) + pbih->biSizeImage);
	hdr.bfReserved1 = 0;
	hdr.bfReserved2 = 0;

	hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) +
		pbih->biSize + pbih->biClrUsed
		* sizeof(RGBQUAD);

	DWORD dwTmp;
	// Copy the BITMAPFILEHEADER into the .BMP file. 
	if (!WriteFile(hfile, (LPVOID)&hdr, sizeof(BITMAPFILEHEADER),
		(LPDWORD)&dwTmp, NULL))
	{
		MessageBox(NULL, L"写BMP文件头失败", NULL, 0);
		return;
	}
	if (!WriteFile(hfile, (LPVOID)pbih, sizeof(BITMAPINFOHEADER)
		+ pbih->biClrUsed * sizeof(RGBQUAD),
		(LPDWORD)&dwTmp, (NULL)))
	{
		MessageBox(NULL, L"写BMP文件头失败", NULL, 0);
		return;
	}
	DWORD cb = pbih->biSizeImage;
	BYTE *hp = lpBits;
	if (!WriteFile(hfile, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, NULL))
	{
		MessageBox(NULL, L"写入BMP文件失败", NULL, 0);
		return;
	}

	// Close the .BMP file. 
	if (!CloseHandle(hfile))
	{
		MessageBox(NULL, L"Can't close BMP file.", NULL, 0);
		return;
	}

	// Free memory. 
	GlobalFree((HGLOBAL)lpBits);
	LocalFree((HLOCAL)pbmi);
}