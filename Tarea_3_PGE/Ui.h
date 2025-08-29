#pragma once
#include <windows.h>

// -------------------- Config --------------------
#ifndef DPI_AWARE
#define DPI_AWARE 1
#endif

// Nombre de clase de ventana
extern const wchar_t* kAppClass;

// Función para registrar la clase de ventana
void RegisterChichiloWindow(HINSTANCE hInst);

// Proc principal (visible porque lo necesita ui.cpp)
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void DrawBitmapFromResource(HDC hdc, int x, int y, int resId);