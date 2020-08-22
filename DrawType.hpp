#pragma once
#include <Windows.h>

typedef enum tag_DataType_Flags
{
	IMAGE,
	RECTANGLE,
	TEXT,
	SPACE
}DTFLAGS;

class DrawType
{
public:
	virtual DTFLAGS getId() {
		return SPACE;
	};
	virtual ~DrawType() {};
};

class DT_image : public DrawType
{
public:
	DT_image(int x, int y, int width, int height, wchar_t* fileName,
		ULONG_PTR m_gdiplusToken, Gdiplus::GdiplusStartupInput gdiplusStartupInput) :
		x(x), y(y), width(width), height(height) {
		Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
		img = new Gdiplus::Bitmap(fileName);
	};
	DT_image(int x, int y, int width, int height, Gdiplus::Bitmap* img) :
		x(x), y(y), width(width), height(height) {
		if (width < 0) {
			x -= width;
			width *= -1;
		}
		if (height < 0) {
			y -= height;
			height *= -1;
		}
		this->img = img->Clone(x, y, width, height, PixelFormat32bppRGB);
	};
	void setData(int x, int y, int width, int height, Gdiplus::Bitmap* img, int srcx, int srcy, int srcw, int srch)
	{
		if (width < 0) {
			x += width;
			width *= -1;
		}
		if (height < 0) {
			y += height;
			height *= -1;
		}
		if (srcw < 0) {
			srcx += srcw;
			srcw *= -1;
		}
		if (srch < 0) {
			srcy += srch;
			srch *= -1;
		}
		this->x = x;
		this->y = y;
		this->width = width;
		this->height = height;
		if (this->img != NULL)
			delete this->img;
		this->img = img->Clone(srcx, srcy, srcw, srch, PixelFormat32bppRGB);
	}
	DT_image(int x, int y, int width, int height, Gdiplus::Bitmap* img, int srcx, int srcy, int srcw, int srch) :
		x(x), y(y), width(width), height(height) {
		this->img = img->Clone(srcx, srcy, srcw, srch, PixelFormat32bppRGB);
	};
	~DT_image()
	{
		if (img != NULL) {
			delete img;
			img = NULL;
		}
	}
	Gdiplus::Bitmap* getImg() { return this->img; };
	int getX() { return this->x; };
	int getY() { return this->y; };
	int getW() { return this->width; };
	int getH() { return this->height; };
private:
	int x, y, width, height;
	Gdiplus::Bitmap* img;
	DTFLAGS getId() { return IMAGE; };
};

class DT_rectangle : public DrawType
{
public:
	DT_rectangle(int left, int top, int right, int bottom, COLORREF color, int thick, int i, int iStyle) :
		left(left), top(top), right(right), bottom(bottom), color(color), thick(thick), i(i), iStyle(iStyle) {};
	void setData(int left, int top, int right, int bottom) {
		this->left = left;
		this->top = top;
		this->right = right;
		this->bottom = bottom;
	}
	int getL() { return this->left; };
	int getT() { return this->top; };
	int getR() { return this->right; };
	int getB() { return this->bottom; };
	COLORREF getColor() { return this->color; };
	int getThick() { return this->thick; };
	int getI() { return this->i; };
	int getStyle() { return this->iStyle; };
private:
	int left, top, right, bottom;
	COLORREF color;
	int thick;
	int i;
	int iStyle;
	DTFLAGS getId() { return RECTANGLE; };
};

class DT_text : public DrawType
{
public:
	DT_text(int mode, COLORREF color, int x, int y, wchar_t* data, int height) :
		mode(mode), color(color), x(x), y(y), data(data), height(height) { };
	void setData(int x, int y, wchar_t*data) {
		this->x = x;
		this->y = y;
		this->data = data;
	}
	int getMode() { return this->mode; };
	COLORREF getColor() { return this->color; };
	int getX() { return this->x; };
	int getY() { return this->y; };
	int getH() { return this->height; };
	wchar_t* getData() { return this->data; };
private:
	int mode;
	COLORREF color;
	int x, y;
	wchar_t* data;
	int height;
	DTFLAGS getId() { return TEXT; };
};