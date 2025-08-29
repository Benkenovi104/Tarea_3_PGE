#include "ui.h"
#include <windowsx.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <tchar.h>
#include <string>
#include <vector>
#include "Resource.h"

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")

// -------------------- Globals --------------------
const wchar_t* kAppClass = L"ChichiloWin32App";

static int g_dpi = 96; // 96 = 100%
static HFONT g_hFontTitle = nullptr;
static HFONT g_hFontText = nullptr;
static HFONT g_hFontSmall = nullptr;

enum Section { SEC_INICIO, SEC_CARTA, SEC_HISTORIA, SEC_HORARIOS, SEC_CONTACTO };
static Section g_section = SEC_INICIO;

enum Plato { PLATO_NONE, PLATO_RANAS, PLATO_CARACOLES, PLATO_RABAS, PLATO_MERLUZA, PLATO_GAMBAS };
struct PlatoDef { Plato id; const wchar_t* nombre; int recurso; };

static const PlatoDef kPlatos[] = {
    { PLATO_RANAS,     L"Ranas a la provenzal",      IDB_RANAS },
    { PLATO_CARACOLES, L"Caracoles a la Bordaleza",  IDB_CARACOLES },
    { PLATO_RABAS,     L"Rabas a la Calabria",       IDB_RABAS },
    { PLATO_MERLUZA,   L"Merluza al ajillo",         IDB_MERLUZA },
    { PLATO_GAMBAS,    L"Gambas al Ajillo",          IDB_GAMBAS },
};

static Plato g_platoSeleccionado = PLATO_RANAS;
static std::vector<RECT> g_platoRects; // mismos índices que kPlatos

static const int kHeaderHeight = 140;
static const int kSectionBarHeight = 48;

// -------------------- Utilidades --------------------
inline int S(int px) { return MulDiv(px, g_dpi, 96); }

HFONT MakeFont(int pts, int weight = FW_NORMAL, bool italic = false) {
    LOGFONTW lf{};
    lf.lfHeight = -MulDiv(pts, g_dpi, 72);
    lf.lfWeight = weight;
    lf.lfItalic = italic ? TRUE : FALSE;
    lstrcpyW(lf.lfFaceName, L"Segoe UI");
    return CreateFontIndirectW(&lf);
}

void InitFonts() {
    if (!g_hFontTitle) g_hFontTitle = MakeFont(24, FW_SEMIBOLD);
    if (!g_hFontText)  g_hFontText = MakeFont(11);
    if (!g_hFontSmall) g_hFontSmall = MakeFont(9);
}

void DeleteFonts() {
    if (g_hFontTitle) { DeleteObject(g_hFontTitle); g_hFontTitle = nullptr; }
    if (g_hFontText) { DeleteObject(g_hFontText);  g_hFontText = nullptr; }
    if (g_hFontSmall) { DeleteObject(g_hFontSmall); g_hFontSmall = nullptr; }
}

void DrawTextLine(HDC hdc, HFONT font, COLORREF color, int x, int y, const std::wstring& s) {
    HFONT old = (HFONT)SelectObject(hdc, font);
    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);
    TextOutW(hdc, x, y, s.c_str(), (int)s.size());
    SelectObject(hdc, old);
}

// -------------------- Tabs --------------------
struct TabDef { Section id; const wchar_t* label; };
static const TabDef kTabs[] = {
    {SEC_INICIO,   L"Inicio"},
    {SEC_CARTA,    L"Carta"},
    {SEC_HISTORIA, L"Historia"},
    {SEC_HORARIOS, L"Horarios"},
    {SEC_CONTACTO, L"Contacto"},
};

RECT SectionBarRect(RECT rcClient) {
    RECT r = rcClient;
    r.top += S(kHeaderHeight);
    r.bottom = r.top + S(kSectionBarHeight);
    return r;
}

std::vector<RECT> BuildTabRects(const RECT& bar) {
    int count = (int)(sizeof(kTabs) / sizeof(kTabs[0]));
    int w = (bar.right - bar.left) / count;
    std::vector<RECT> out; out.reserve(count);
    for (int i = 0; i < count; i++) {
        RECT r{ bar.left + i * w, bar.top, bar.left + (i + 1) * w, bar.bottom };
        out.push_back(r);
    }
    return out;
}

void FillRectColor(HDC hdc, const RECT& r, COLORREF c) {
    HBRUSH b = CreateSolidBrush(c);
    FillRect(hdc, &r, b);
    DeleteObject(b);
}

void DrawRoundedRect(HDC hdc, const RECT& r, int radius, COLORREF fill, COLORREF border) {
    HBRUSH hBrush = CreateSolidBrush(fill);
    HPEN hPen = CreatePen(PS_SOLID, 1, border);
    HGDIOBJ oldB = SelectObject(hdc, hBrush);
    HGDIOBJ oldP = SelectObject(hdc, hPen);
    RoundRect(hdc, r.left, r.top, r.right, r.bottom, S(radius), S(radius));
    SelectObject(hdc, oldB); SelectObject(hdc, oldP);
    DeleteObject(hBrush); DeleteObject(hPen);
}

// -------------------- Pintado --------------------
void PaintHeader(HDC hdc, const RECT& rcClient) {
    RECT r{ rcClient.left, rcClient.top, rcClient.right, rcClient.top + S(kHeaderHeight) };

    int h = r.bottom - r.top;
    for (int i = 0; i < h; i++) {
        BYTE g = (BYTE)(220 - (i * 120) / max(1, h));
        HPEN p = CreatePen(PS_SOLID, 1, RGB(245, g, 120));
        HGDIOBJ old = SelectObject(hdc, p);
        MoveToEx(hdc, r.left, r.top + i, nullptr);
        LineTo(hdc, r.right, r.top + i);
        SelectObject(hdc, old);
        DeleteObject(p);
    }

    DrawTextLine(hdc, g_hFontTitle, RGB(40, 40, 40), S(24), r.top + S(26), L"Cantina Chichilo");
    DrawTextLine(hdc, g_hFontText, RGB(60, 60, 60), S(26), r.top + S(66), L"Una familia para servirlo desde 1956");
}

void PaintSectionBar(HDC hdc, const RECT& rcClient) {
    RECT bar = SectionBarRect(rcClient);
    FillRectColor(hdc, bar, RGB(250, 246, 240));
    auto rects = BuildTabRects(bar);
    for (size_t i = 0; i < rects.size(); ++i) {
        const auto& t = kTabs[i];
        RECT r = rects[i];
        bool active = (g_section == t.id);
        if (active) {
            RECT rr = r; InflateRect(&rr, -S(8), -S(8));
            DrawRoundedRect(hdc, rr, 12, RGB(255, 255, 255), RGB(230, 180, 120));
        }
        SIZE sz{}; HGDIOBJ old = SelectObject(hdc, g_hFontText);
        GetTextExtentPoint32W(hdc, t.label, (int)wcslen(t.label), &sz);
        SelectObject(hdc, old);
        int cx = (r.left + r.right - sz.cx) / 2;
        int cy = (r.top + r.bottom - sz.cy) / 2;
        DrawTextLine(hdc, g_hFontText, RGB(60, 50, 40), cx, cy, t.label);
    }
}

void DrawParagraph(HDC hdc, int x, int y, int w, const std::wstring& text) {
    RECT r{ x, y, x + w, y + S(1000) };
    HFONT old = (HFONT)SelectObject(hdc, g_hFontText);
    SetTextColor(hdc, RGB(40, 40, 40));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, text.c_str(), (int)text.size(), &r, DT_LEFT | DT_TOP | DT_WORDBREAK);
    SelectObject(hdc, old);
}

// Área de contenido (card) coherente para pintar y para hit-testing
static RECT GetContentCardRect(const RECT& rcClient) {
    RECT bar = SectionBarRect(rcClient);
    RECT content{ rcClient.left + S(24), bar.bottom + S(20), rcClient.right - S(24), rcClient.bottom - S(24) };
    RECT card = content; InflateRect(&card, -S(4), -S(4));
    return card;
}

// Dibuja BMP dentro de un rect, manteniendo aspecto
static void DrawBitmapFromResourceFitRect(HDC hdc, const RECT& dest, int resId) {
    HBITMAP hBmp = LoadBitmap(GetModuleHandle(nullptr), MAKEINTRESOURCE(resId));
    if (!hBmp) return;
    BITMAP bm; GetObject(hBmp, sizeof(bm), &bm);

    HDC mem = CreateCompatibleDC(hdc);
    HGDIOBJ old = SelectObject(mem, hBmp);

    int dstW = dest.right - dest.left;
    int dstH = dest.bottom - dest.top;

    double sx = (double)dstW / bm.bmWidth;
    double sy = (double)dstH / bm.bmHeight;
    double k = min(sx, sy);

    int w = (int)(bm.bmWidth * k);
    int h = (int)(bm.bmHeight * k);
    int x = dest.left + (dstW - w) / 2;
    int y = dest.top + (dstH - h) / 2;

    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, nullptr);
    StretchBlt(hdc, x, y, w, h, mem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

    SelectObject(mem, old);
    DeleteDC(mem);
    DeleteObject(hBmp);
}


// Contenido
void PaintContent(HDC hdc, const RECT& rcClient) {
    RECT bar = SectionBarRect(rcClient);
    RECT content{ rcClient.left + S(24), bar.bottom + S(20), rcClient.right - S(24), rcClient.bottom - S(24) };

    RECT card = content; InflateRect(&card, -S(4), -S(4));
    RECT shadow = card; OffsetRect(&shadow, S(3), S(3));
    FillRectColor(hdc, shadow, RGB(230, 230, 230));
    DrawRoundedRect(hdc, card, 16, RGB(255, 255, 255), RGB(235, 215, 190));

    int pad = S(20);
    int x = card.left + pad; int y = card.top + pad; int w = (card.right - card.left) - 2 * pad;

    switch (g_section) {
    case SEC_INICIO:
        DrawTextLine(hdc, g_hFontTitle, RGB(30, 30, 30), x, y, L"Bienvenido a la Cantina");
        y += S(40);
        DrawParagraph(hdc, x, y, w,
            L"Clásico bodegón porteño en La Paternal...");
        break;
    case SEC_CARTA: {
        DrawTextLine(hdc, g_hFontTitle, RGB(30, 30, 30), x, y, L"Nuestra Carta");
        y += S(44); // más espacio debajo del título

        // Columnas: izquierda = lista, derecha = imagen
        const int leftColW = S(300);
        const int buttonH = S(36);   // botones más altos
        const int buttonGap = S(12); // más aire entre botones

        // Rect de imagen (más grande y menos pegada a la derecha)
        RECT imgRect{
            card.right - pad - S(400),   // ancho imagen ~400
            card.top + pad,
            card.right - pad,
            card.top + pad + S(300)      // alto imagen ~300
        };

        // Guardar rects de platos para el hit-test
        g_platoRects.clear();
        int yBtn = y;
        for (const auto& p : kPlatos) {
            RECT r{ x, yBtn, x + leftColW, yBtn + buttonH };
            g_platoRects.push_back(r);

            COLORREF fondo = (g_platoSeleccionado == p.id) ? RGB(255, 245, 230) : RGB(255, 255, 255);
            DrawRoundedRect(hdc, r, 8, fondo, RGB(210, 190, 160));

            // Texto centrado verticalmente en el botón
            DrawTextLine(hdc, g_hFontText, RGB(30, 30, 30), r.left + S(12), r.top + (buttonH / 4), p.nombre);

            yBtn += buttonH + buttonGap;
        }

        // Marco suave para la imagen
        DrawRoundedRect(hdc, imgRect, 12, RGB(255, 255, 255), RGB(235, 215, 190));

        // Imagen del plato seleccionado dentro del rect
        for (const auto& p : kPlatos) {
            if (p.id == g_platoSeleccionado) {
                RECT inner = imgRect; InflateRect(&inner, -S(14), -S(14));
                DrawBitmapFromResourceFitRect(hdc, inner, p.recurso);
                break;
            }
        }

        // ---- Especialidades debajo ----
        int yEspecial = max(yBtn, imgRect.bottom) + S(36);
        DrawTextLine(hdc, g_hFontTitle, RGB(30, 30, 30), x, yEspecial, L"Nuestras Especialidades");
        yEspecial += S(44);
        DrawParagraph(hdc, x, yEspecial, (card.right - card.left) - 2 * pad,
            L"• Quinotos al Rhun con Helado de Americana\n"
            L"• Mondongo a la Italiana\n"
            L"• Rinones al Vino Blanco\n"
            L"• Calamarettis a la Escarpetta");
    } break;

    case SEC_HISTORIA:
        DrawTextLine(hdc, g_hFontTitle, RGB(30, 30, 30), x, y, L"Historia");
        y += S(36);
        DrawParagraph(hdc, x, y, w,
            L"Desde 1956, Cantina Chichilo es un ícono de barrio...");
        break;
    case SEC_HORARIOS:
        DrawTextLine(hdc, g_hFontTitle, RGB(30, 30, 30), x, y, L"Horarios");
        y += S(50);
        DrawParagraph(hdc, x, y, w,
            L"-De Jueves a Sábado 20:30 a 00:00 hs");
        y += S(30);
        DrawParagraph(hdc, x, y, w,
            L"-Sabados y Domingos de 12:30 a 14:30 hs");
        break;
    case SEC_CONTACTO:
        DrawTextLine(hdc, g_hFontTitle, RGB(30, 30, 30), x, y, L"Contacto");
        y += S(50);
        DrawParagraph(hdc, x, y, w,
            L"Dirección: Camarones 1901, Esquina terrero 2006");
        y += S(30);
        DrawParagraph(hdc, x, y, w,
            L"Capital Federal");
        y += S(30);
        DrawParagraph(hdc, x, y, w,
            L"Reservas al telefono: 011-4581-1984 o al 011-4584-1263");
        y += S(30);
        DrawParagraph(hdc, x, y, w,
            L"Email de Contacto del lugar: cantinachichilo@cantinachichilo.com.ar");
        y += S(30);
        DrawParagraph(hdc, x, y, w,
            L"Email: chichilo3554@hotmail.com");
        break;
    }
}

void DoPaint(HWND hWnd) {
    PAINTSTRUCT ps; HDC hdc = BeginPaint(hWnd, &ps);
    RECT rc; GetClientRect(hWnd, &rc);

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right - rc.left, rc.bottom - rc.top);
    HGDIOBJ oldBmp = SelectObject(mem, bmp);

    FillRectColor(mem, rc, RGB(252, 250, 247));

    PaintHeader(mem, rc);
    PaintSectionBar(mem, rc);
    PaintContent(mem, rc);

    BitBlt(hdc, 0, 0, rc.right - rc.left, rc.bottom - rc.top, mem, 0, 0, SRCCOPY);
    SelectObject(mem, oldBmp); DeleteObject(bmp); DeleteDC(mem);

    EndPaint(hWnd, &ps);
}

// -------------------- DPI / Mica --------------------
void UpdateDPI(HWND hWnd) {
    HDC hdc = GetDC(hWnd);
    g_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(hWnd, hdc);
    DeleteFonts(); InitFonts();
}

void SetMica(HWND h) {
    BOOL enable = TRUE;
    int backdrop = 2; // 2 = Mica
    DwmSetWindowAttribute(h, (DWMWINDOWATTRIBUTE)38, &backdrop, sizeof(backdrop));
    DwmSetWindowAttribute(h, (DWMWINDOWATTRIBUTE)1029, &enable, sizeof(enable));
}

// -------------------- Window Proc --------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        UpdateDPI(hWnd);
        SetMica(hWnd);
        return 0;
    case WM_DPICHANGED:
        g_dpi = HIWORD(wParam);
        DeleteFonts(); InitFonts();
        if (RECT* prcNew = (RECT*)lParam)
            MoveWindow(hWnd, prcNew->left, prcNew->top, prcNew->right - prcNew->left,
                prcNew->bottom - prcNew->top, TRUE);
        InvalidateRect(hWnd, nullptr, TRUE);
        return 0;
    case WM_SIZE:
        InvalidateRect(hWnd, nullptr, TRUE); return 0;
    case WM_LBUTTONUP: {
        RECT rc; GetClientRect(hWnd, &rc);
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

        // Tabs
        RECT bar = SectionBarRect(rc);
        auto rects = BuildTabRects(bar);
        for (size_t i = 0; i < rects.size(); ++i) {
            if (PtInRect(&rects[i], pt)) {
                g_section = kTabs[i].id;
                InvalidateRect(hWnd, nullptr, TRUE);
                return 0;
            }
        }

        // Botones de platos (solo si estamos en Carta)
        if (g_section == SEC_CARTA) {
            for (size_t i = 0; i < g_platoRects.size(); ++i) {
                if (PtInRect(&g_platoRects[i], pt)) {
                    g_platoSeleccionado = kPlatos[i].id;
                    InvalidateRect(hWnd, nullptr, TRUE);
                    return 0;
                }
            }
        }
        return 0;
    }

    case WM_PAINT:
        DoPaint(hWnd); return 0;
    case WM_DESTROY:
        DeleteFonts(); PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// -------------------- Registro --------------------
void RegisterChichiloWindow(HINSTANCE hInst) {
    WNDCLASSEXW wc{ sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = kAppClass;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    RegisterClassExW(&wc);
}


void DrawBitmapFromResource(HDC hdc, int x, int y, int resId) {
    HBITMAP hBmp = LoadBitmap(GetModuleHandle(nullptr), MAKEINTRESOURCE(resId));
    if (!hBmp) return;

    BITMAP bm;
    GetObject(hBmp, sizeof(bm), &bm);

    HDC hMemDC = CreateCompatibleDC(hdc);
    HGDIOBJ oldBmp = SelectObject(hMemDC, hBmp);

    BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);

    SelectObject(hMemDC, oldBmp);
    DeleteDC(hMemDC);
    DeleteObject(hBmp);
}
