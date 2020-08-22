#include <windows.h>
#include <gdiplus.h>
#include <shellscalingapi.h>
#include "resource.h"
#include "DrawType.hpp"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "Shcore.lib")

typedef enum tagMode {
	BACKSTAGE,				// ��̨ģʽ
	START,					// ��ʼģʽ
	SELECT					// ѡ��ģʽ
} mode;

// ����������
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

// ��Ϣ��
#define WM_TRAY				(WM_USER + 1)
#define UR_QUIT				(WM_USER + 2)
#define HK_START			(WM_USER + 3)

// ȫ����Դ
NOTIFYICONDATA						nid;								// ��������
HMENU								hMenu;								// ���̲˵�
const wchar_t*						pClassName = L"screenshot";			// ��������
HINSTANCE							g_hInstance;						// ȫ��ʵ�����
int									desktop_width;						// ȫ�ִ��ڴ�С��width��
int									desktop_height;						// ȫ�ִ��ڴ�С��height��
HWND								child_hwnd;							// �Ӵ��ھ��
mode								ctn_mode;							// ���Ʊ���, ����ģʽ���
bool								ctn_isSelect;						// ���Ʊ������ж��Ƿ����ѡ��ģʽ
POINT								lt_point;							// left/top���Ͻ�����
POINT								rb_point;							// right/bottom���½�����
const int							drawNum = 13;						// ��������������չ
DrawType*							drawData[drawNum];					// ��������
ULONG_PTR							m_gdiplusToken;						// GDI��ʼ������
Gdiplus::GdiplusStartupInput		gdiplusStartupInput;				// GDI��ʼ������
PBITMAPINFO							pbmi;								// ȫ��λͼ��Ϣ
LPBYTE								lpBits;								// ȫ��λͼ����
const wchar_t*						saveFileName = 
		L"C:/Users/Administrator/Desktop/screenshot.bmp";				// ����λ��
wchar_t*							maskFileName = L"./mask.png";		// ��ɫ����ͼƬλ��
wchar_t*							tipInfo =							// ��ʾ��Ϣ
		L"����ģʽ, Esc�˳�����ģʽ�� Enter���ɽ���ͼƬ"; 
wchar_t buf_pos[32];
wchar_t buf_rgb[32];

/*
	������void InitTray(HINSTANCE hInstance, HWND hWnd)
	���ܣ�ʵ��������
	������
		1��HINSTANCE hInstance������ʵ�����
		2��HWND hWnd�����ھ��
*/
void InitTray(HINSTANCE hInstance, HWND hWnd)
{
	/* ������������ nid , Header��<shellapi.h> */
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;	// �������̳�����
	nid.uID = IDI_ICON1;	// ��������ͼ����
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_INFO;
	nid.uCallbackMessage = WM_TRAY; // ������Ϣ�ص��¼�
	nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));	// ����ͼ��
	lstrcpy(nid.szTip, pClassName);	// ������ʾ��Ϣ

	hMenu = CreatePopupMenu();//�������̲˵�
	AppendMenu(hMenu, 0, 0, L"����");
	AppendMenu(hMenu, 0, 0, L"�ȼ�����(Alt+z)");	
	AppendMenu(hMenu, MF_STRING, ID_EXIT, L"�˳�");	// �˵������"�˳�"�������ص���ϢΪID_EXIT
	Shell_NotifyIcon(NIM_ADD, &nid);
}

/*
	������void paint_text(HDC hMemDC, DT_text *data)
	���ܣ����ڻ���λͼ
	������
		1��HDC hMemDC����������ʾ�豸
		2��DT_text *data����������Դ
*/
void paint_text(HDC hMemDC, DT_text *data)
{
	DT_text* dt_text = data;
	// ��������������Ϣ 
	LOGFONT logfont;
	ZeroMemory(&logfont, sizeof(LOGFONT));
	logfont.lfCharSet = GB2312_CHARSET;			// ������ʽ
	logfont.lfHeight = -1 * dt_text->getH();	// �߶�
	HFONT hFont = CreateFontIndirect(&logfont); // ��������
	SelectObject(hMemDC, hFont);				// ������ѡ���ڴ��豸��
	// ��������ģʽ����ɫ 
	SetBkMode(hMemDC, dt_text->getMode());		// ���ñ���͸��
	SetTextColor(hMemDC, dt_text->getColor());	// ����������ɫ
	// ������ֵ��豸��
	TextOutW(
		hMemDC,						// �豸�ڴ�
		dt_text->getX(), dt_text->getY(),		// ��ʼλ��
		dt_text->getData(),				// ���ַ����ַ���
		lstrlen(dt_text->getData())		// �ַ�������
	);
	// �ͷ���Դ
	DeleteObject(hFont);
}

/*
	������void paint_rectangle(HDC hMemDC, DT_rectangle *data)
	���ܣ����ڻ���λͼ
	������
		1��HDC hMemDC����������ʾ�豸
		2��DT_rectangle *data����������Դ
*/
void paint_rectangle(HDC hMemDC, DT_rectangle *data)
{
	DT_rectangle* dt_rectangle = data;
	// ���û��ƾ��εı߿���ʽ���߿��ϸ���߿���ɫ 
	HPEN hPen = ::CreatePen(dt_rectangle->getStyle(), dt_rectangle->getThick(), dt_rectangle->getColor());
	// ���þ�������仭ˢ 
	HBRUSH hBrush = (HBRUSH)::GetStockObject(dt_rectangle->getI());
	// ��hPen��hBrushѡ���ڴ���
	::SelectObject(hMemDC, hPen);
	::SelectObject(hMemDC, hBrush);
	Rectangle(
		hMemDC,						// �����ڴ��豸���
		dt_rectangle->getL(),		// left
		dt_rectangle->getT(),		// top
		dt_rectangle->getR(),		// right
		dt_rectangle->getB()		// bottom
	);
	// �ͷ���Դ
	DeleteObject(hPen);
	DeleteObject(hBrush);
}

/*
	������void paint_image(HDC hMemDC, DT_image *data)
	���ܣ����ڻ���λͼ
	������
		1��HDC hMemDC����������ʾ�豸
		2��DT_image *data��λͼ����Դ
*/
void paint_image(HDC hMemDC, DT_image *data)
{
	DT_image* dt_image = data;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL); // GDI��������
	Gdiplus::Graphics graphics(hMemDC); // ����һ������
	graphics.DrawImage(
		dt_image->getImg(),							// ��Ҫ���Ƶ�λͼ
		dt_image->getX(), dt_image->getY(),			// ���Ƶ����ڵ����Ͻ�����λ��
		dt_image->getW(), dt_image->getH()			// ������Ⱥ͸߶ȣ��˴��п��ܷ�������Ч����
	);
}

/*
	������void paint(HWND hWnd)
	���ܣ������ػ�
	������
		1��HWND hWnd�����ھ��
*/
void paint(HWND hWnd)
{
	if (hWnd == child_hwnd) {	// ֻ����Ӵ��ڽ����ػ�
		// ��ȡ��������Rect
		RECT rtClient;
		GetClientRect(hWnd, &rtClient);
		/* ����win32 ˫���弼����ͼ����������λͼһ֡һ֡��ʾ���������ˢ��Ƶ�ʹ��� */
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(hWnd, &ps);
		HDC hMemDC = ::CreateCompatibleDC(hdc); // ������ǰ�����ĵļ���dc(�ڴ�DC)
		HBITMAP hBitmap = ::CreateCompatibleBitmap(
			hdc,
			rtClient.right - rtClient.left,
			rtClient.bottom - rtClient.top
		); // ������ǰ�����ĵļ���λͼ
		::SelectObject(hMemDC, hBitmap); // ��λͼ���ص��ڴ�DC

		// ������DrawData���ݻ��Ƶ�hMemDC�ڴ���
		for (int i = 0; i < drawNum; i++) {
			if (drawData[i] == NULL)
				continue;

			switch (drawData[i]->getId()) { 	// �жϻ�������
			case IMAGE: {						// ����λͼ
				paint_image(hMemDC, (DT_image*)drawData[i]);
				break;
			}
			case RECTANGLE: {		// ���ƾ���
				paint_rectangle(hMemDC, (DT_rectangle*)drawData[i]);
				break;
			}
			case TEXT: {			// ��������
				paint_text(hMemDC, (DT_text*)drawData[i]);
				break;
			}
			}
		}
		// ���ڴ��е�����������ʾ������
		BitBlt(
			hdc,
			0, 0,
			rtClient.right - rtClient.left, rtClient.bottom - rtClient.top,
			hMemDC,
			0, 0,
			SRCCOPY
		);
		// �ͷ���Դ
		DeleteDC(hMemDC);
		DeleteObject(hBitmap);
		// ��������
		EndPaint(hWnd, &ps);
	}
}

/*
	������void LoadFullScreenShotIntoMemory()
	���ܣ���ȫ����ͼ�������ڴ���
	��������
*/
void LoadFullScreenShotIntoMemory()
{
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = desktop_width;
	rect.bottom = desktop_height;

	ScreenCapture(rect, pbmi, lpBits);
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL); // GDI��������
	Gdiplus::Bitmap img(pbmi, lpBits);
	drawData[0] = new DT_image(0, 0, desktop_width, desktop_height, &img);
}

/*
	������void createTopWindow()
	���ܣ�����һ����������
	��������
*/
void createTopWindow()
{
	child_hwnd = CreateWindowEx(
		WS_EX_TOPMOST,							// ���ڵ���չ���
		pClassName,								// ָ��ע��������ָ��
		L"child",								// ָ�򴰿����Ƶ�ָ��
		WS_VISIBLE | WS_POPUP,					// ���ڷ��
		0, 0, desktop_width, desktop_height,	// ���ڵ� x,y �����Լ����
		NULL,									// �����ڵľ��
		NULL,									// �˵��ľ��
		g_hInstance,							// Ӧ�ó���ʵ���ľ��
		NULL									// ָ�򴰿ڵĴ�������
	);
	/* ȥ���Ӵ��ڱ��������˵��������С���������� */
	::SetWindowLong(child_hwnd, GWL_STYLE, GetWindowLong(child_hwnd, GWL_STYLE) & (~(WS_CAPTION | WS_SYSMENU | WS_SIZEBOX)));
	::SetWindowLong(child_hwnd, GWL_EXSTYLE, GetWindowLong(child_hwnd, GWL_EXSTYLE) & (~(WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME)) | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
}

/*
	������void releaseBackstageResources()
	���ܣ��ͷź�̨ģʽ�´�������Դ
	��������
*/
void releaseBackstageResources()
{
	// �ͷź�̨��Դ
	delete drawData[1];
	delete drawData[2];
	delete drawData[3];
	GlobalFree(pbmi);
	GlobalFree(lpBits);
}

/*
	������void releaseSelectModeResources(HWND hWnd)
	���ܣ��ͷ�ѡ��ģʽ�´�������Դ
	������
		1��HWND hWnd�����ھ��
*/
void releaseSelectResources()
{
	ctn_mode = BACKSTAGE;					// ת��ɺ�̨ģʽ
	for (int i = 0; i < drawNum; i++) {
		if (i == 1 || i == 2 || i == 3)		// �±�Ϊ1��2��3��Ϊ��̨��פ��Դ������Ҫ�ͷ�
			continue;
		delete drawData[i];
		drawData[i] = NULL;
	}
}

/*
	������void paint_background(HWND hWnd)
	����: �ػ汳��
	������
		1��HWND hWnd�����ھ��
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
	������void paint_selectRect()
	����: ����ѡ������ı���ɫ���ǻ�ɫ����
	��������
*/
void paint_selectRect(const POINT rb_point)
{
	/* ����ѡ�������ɫ�߿� */
	drawData[4] == NULL ?
		drawData[4] = new DT_rectangle(lt_point.x, lt_point.y, rb_point.x,
			rb_point.y, RGB(255, 255, 255), 1, NULL_BRUSH, PS_DASHDOTDOT) :
			((DT_rectangle*)drawData[4])->setData(
				lt_point.x, lt_point.y, rb_point.x, rb_point.y);

	/* ����ѡ�������ɫ�߿� */
	drawData[5] == NULL ?
		drawData[5] = new DT_rectangle(lt_point.x - 1, lt_point.y - 1, rb_point.x + 1,
			rb_point.y + 1, RGB(0, 0, 0), 1, NULL_BRUSH, PS_DASHDOTDOT) :
			((DT_rectangle*)drawData[5])->setData(
				lt_point.x - 1, lt_point.y - 1, rb_point.x + 1, rb_point.y + 1);

	/* ����ѡ��ͼƬ����ԭͼ���ǻ�ɫ���� */
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
	������void paint_info(const POINT rb_point)
	����: ������굱ǰ�����λ�á���ɫ����Ϣ
	������
		1��HWND hWnd�����ھ��
*/
void paint_info(const POINT rb_point)
{
	const int radius = 25;			// ��ʵ��뾶25pix
	const int interval = 50;		// ��ʾ���������ݼ��Ϊ50pix
	const int frameSize = 150;		// ��ʾ�������Σ��ĺ���sizeΪ150pix
	const int textBoxWidth = 50;	// ���ֿ�߶�Ϊ50pix
	const int textSize = 17;		// �����С

									/* ���ƷŴ��ͼƬ */
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

	/* ������ʮ�ּ� */
	drawData[8] == NULL ?
		drawData[8] = new DT_rectangle(
			rb_point.x + interval + frameSize / 2, rb_point.y + interval,
			rb_point.x + interval + frameSize / 2 + 2, rb_point.y + interval + frameSize,
			RGB(0, 255, 0), 2, NULL_BRUSH, PS_SOLID) :
			((DT_rectangle*)drawData[8])->setData(rb_point.x + interval + frameSize / 2, rb_point.y + interval,
				rb_point.x + interval + frameSize / 2 + 2, rb_point.y + interval + frameSize);

	/* ���ƺ�ʮ�ּ� */
	drawData[9] == NULL ?
		drawData[9] = new DT_rectangle(
			rb_point.x + interval, rb_point.y + interval + frameSize / 2,
			rb_point.x + interval + frameSize, rb_point.y + interval + frameSize / 2 + 2,
			RGB(0, 255, 0), 2, NULL_BRUSH, PS_SOLID) :
			((DT_rectangle*)drawData[9])->setData(
				rb_point.x + interval, rb_point.y + interval + frameSize / 2,
				rb_point.x + interval + frameSize, rb_point.y + interval + frameSize / 2 + 2);

	/* �����������򱳾� */
	drawData[10] == NULL ?
		drawData[10] = new DT_rectangle(
			rb_point.x + interval, rb_point.y + interval + frameSize,
			rb_point.x + interval + frameSize, rb_point.y + interval + frameSize + textBoxWidth,
			RGB(0, 0, 0), 1, BLACK_BRUSH, PS_SOLID) :
			((DT_rectangle*)drawData[10])->setData(
				rb_point.x + interval, rb_point.y + interval + frameSize,
				rb_point.x + interval + frameSize, rb_point.y + interval + frameSize + textBoxWidth);

	/* ����POS��Ϣ */
	memset(buf_pos, 0, sizeof(buf_pos));
	wsprintf(buf_pos, L"Pos:(%d,%d)", rb_point.x, rb_point.y);
	drawData[11] == NULL ?
		drawData[11] = new DT_text(
			TRANSPARENT, RGB(255, 255, 255),
			rb_point.x + interval, rb_point.y + interval + frameSize,
			buf_pos, textSize) :
			((DT_text*)drawData[11])->setData(
				rb_point.x + interval, rb_point.y + interval + frameSize, buf_pos);

	/* ����RGB��Ϣ */
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
	������void outputScreenShot(const wchar_t* saveFileName = ::saveFileName)
	����: �����ͼ������
	������
		1��const wchar_t* saveFileName�������ļ�·��
*/
void outputScreenShot(const wchar_t* saveFileName = ::saveFileName)
{
	/* ����λ�� */
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
	// �����ͼ
	RECT rect;
	rect.left = lt_point.x;
	rect.top = lt_point.y;
	rect.right = lt_point.x + width;
	rect.bottom = lt_point.y + height;
	ScreenCapture(rect, saveFileName);
}

/*
	������LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	���ܣ�win32�ص����������ڴ���win32�����еĻص��¼�
	������
		1��HWND hWnd�����ھ��
		2��UINT uMsg����Ϣ������ʶ�� 
		3��WPARAM wParam��WPARAMʵ���Ͼ���UINT,��32λ�޷�����������Ϣ�ĸ�����Ϣ
		4��LPARAM lParam��LPARAMʵ���Ͼ���LONG,��32λ�з�����������Ϣ�ĸ�����Ϣ
*/
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	/* WM_PAINT��ÿ�����ڷ����ػ棬���ᷢ�������Ϣ֪ͨ���� */
	case WM_PAINT: {
		// �ػ洰��
		paint(hWnd);
		break;
	}
	/* WM_HOTKEY �ȼ���Ϣ�����û������ȼ���ϵͳ�����������Ϣ���͸����� */
	case WM_HOTKEY: {
		// ��Ӧ WM_TRAY WM_LBUTTONDBLCLK ��Ϣ
		PostMessage(hWnd, WM_TRAY, 0, WM_LBUTTONDBLCLK);
		break;
	}
	/* WM_TRAY �û��Զ�����Ϣ�����������¼� */
	case WM_TRAY: {
		switch (lParam) {
		case WM_RBUTTONDOWN: {
			//��ȡ�������
			POINT pt;
			GetCursorPos(&pt);
			//����ڲ˵��ⵥ������˵�����ʧ������
			SetForegroundWindow(hWnd);
			//��ʾ����ȡѡ�еĲ˵�
			int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, NULL, hWnd, NULL);
			if (cmd == ID_EXIT)
				PostMessage(hWnd, WM_DESTROY, UR_QUIT, NULL);
			break;
		}
		case WM_LBUTTONDBLCLK: {
			if (ctn_mode == BACKSTAGE) { 
				/* ���ع�� */
				//HCURSOR hcur = LoadCursorFromFile(L"cursor1.cur");
				//SetSystemCursor(hcur, 32512);
				
				/* ��ȫ��λͼ���ص��ڴ��� */	
				LoadFullScreenShotIntoMemory();

				/*  ����һ���������� */
				createTopWindow();
				
				// ��ģʽת��ɿ�ʼ״̬
				ctn_mode = START;
			}
			break;
		}
		}
		break;
	}
	/* WM_KEYDOWN �����û������豸�����¼� */
	case WM_KEYDOWN: {
		switch (wParam) {
		case VK_ESCAPE: {								// ���̰���Esc�˳���
			if (hWnd == child_hwnd) {
				/* �ͷ�ѡ��ģʽ�´�������Դ */
				releaseSelectResources();

				/* �رմ��� */
				PostMessage(child_hwnd, WM_CLOSE, 0, 0);
			}
			break;
		}
		case VK_RETURN: {					// ���̰��»س���,����ͼƬ������
			if (ctn_mode == SELECT) {
				/* �ñ�������ȫ�� */
				paint_background(hWnd);

				/* �����ͼ������ */
				outputScreenShot();
				
				/* �˳�ѡ��ģʽ�����̨ģʽ */
				PostMessage(child_hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
			}
			break;
		}
		}
		break;
	}
	case WM_RBUTTONDOWN: {		// �Ҽ����£������ͼ����Ӧ WM_KEYDOWN VK_RETURN ��Ϣ
		PostMessage(child_hwnd, WM_KEYDOWN, VK_RETURN, 0);
		break;
	}
	case WM_LBUTTONDOWN: {
		if (hWnd == child_hwnd) {
			// ��ȡ�󶥵�λ��
			GetCursorPos(&lt_point);
			ctn_isSelect = true;
		}
		break; 
	}
	case WM_MOUSEMOVE:{
		// ��ȡ��ǰ��������
		POINT rb_point;
		GetCursorPos(&rb_point);

		/* ������굱ǰ�����λ�á���ɫ����Ϣ */
		paint_info(rb_point);

		if (ctn_isSelect) {
			/* ģʽת��Ϊѡ��ģʽ */
			ctn_mode = SELECT;

			/* ����ѡ������ı���ɫ���ǻ�ɫ���� */
			paint_selectRect(rb_point);
		}
		// ˢ�´���
		InvalidateRect(child_hwnd, NULL, true);
		UpdateWindow(child_hwnd);
		break;
	}
	case WM_LBUTTONUP: {
		// ��ȡ�Ҷ���λ��
		GetCursorPos(&rb_point);
		ctn_isSelect = false;
		break;
	}
	case WM_DESTROY: {
		//��������ʱɾ������
		if (wParam == UR_QUIT) {
			/* �ͷź�̨��Դ */
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
	������int setRegisterClassStruct(HINSTANCE hInstance)
	���ܣ�����ע����ṹ��
	������
		1��HINSTANCE hInstance������ʵ�����
*/
ATOM setRegisterClassStruct(HINSTANCE hInstance)
{
	WNDCLASS wc;
	wc.style = NULL;
	wc.hIcon = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;						// �ص�����ΪWndProc����
	wc.hbrBackground = NULL;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = pClassName;					// ��������ΪpClassName
	wc.hCursor = LoadCursor(NULL, IDC_CROSS);		// ���ع��Ϊʮ����
	return RegisterClass(&wc);
}
/*
	������void initGlobalResources(HINSTANCE m_hInstance)
	���ܣ���ʼ��ȫ����Դ
	������
		1��HINSTANCE hInstance������ʵ�����
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
	// ���ý���Ĭ�ϵ�DPI��֪
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

	// ��ʼ��ȫ����Դ
	initGlobalResources(hInstance);

	// 1. ע�ᴰ��ṹ����Ϣ
	ret = setRegisterClassStruct(hInstance);
	if (ret == 0)
	{
		MessageBox(NULL, L"setRegisterClassStruct error", NULL, 0);
		return 0;
	}

	// 2. ��������
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

	// 3. ��ʾ����
	ShowWindow(MainHwnd, iCmdShow);
	UpdateWindow(MainHwnd);

	// 4. ʵ��������
	InitTray(hInstance, MainHwnd);

	// 5. ע���ȼ� ALT+Z ����HK_START
	RegisterHotKey(MainHwnd, HK_START, MOD_ALT, 'Z');

	// 6. ��Ϣѭ��
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
		MessageBox(NULL, L"GetObject()����", NULL, 0);
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
		MessageBox(NULL, L"�ڴ�������", NULL, 0);
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
		MessageBox(NULL, L"�����ļ�ʧ��", NULL, 0);
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
		MessageBox(NULL, L"дBMP�ļ�ͷʧ��", NULL, 0);
		return;
	}
	if (!WriteFile(hfile, (LPVOID)pbih, sizeof(BITMAPINFOHEADER)
		+ pbih->biClrUsed * sizeof(RGBQUAD),
		(LPDWORD)&dwTmp, (NULL)))
	{
		MessageBox(NULL, L"дBMP�ļ�ͷʧ��", NULL, 0);
		return;
	}
	DWORD cb = pbih->biSizeImage;
	BYTE *hp = lpBits;
	if (!WriteFile(hfile, (LPSTR)hp, (int)cb, (LPDWORD)&dwTmp, NULL))
	{
		MessageBox(NULL, L"д��BMP�ļ�ʧ��", NULL, 0);
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