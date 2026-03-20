#include "snake_game.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <optional>
#include <random>
#include <string>

namespace
{
constexpr int kBoardWidth = 20;
constexpr int kBoardHeight = 15;
constexpr UINT_PTR kGameTimerId = 1;
constexpr UINT kTickMilliseconds = 140;
constexpr int kWindowWidth = 980;
constexpr int kWindowHeight = 760;

struct AppState
{
    snake::GameState game = snake::CreateGame(kBoardWidth, kBoardHeight, 0);
    std::mt19937 generator{ std::random_device{}() };
    std::deque<snake::Direction> directionQueue;
    int decorSeed = 0;
};

AppState& GetAppState()
{
    static AppState state{};
    return state;
}

int NextFoodSeed(AppState& state)
{
    return static_cast<int>(state.generator());
}

void RestartGame(AppState& state)
{
    state.decorSeed = NextFoodSeed(state);
    state.game = snake::CreateGame(kBoardWidth, kBoardHeight, NextFoodSeed(state));
    state.directionQueue.clear();
}

COLORREF ShadeColor(COLORREF color, double factor)
{
    const auto clampChannel = [factor](int value)
    {
        const int shaded = static_cast<int>(std::round(value * factor));
        return std::clamp(shaded, 0, 255);
    };

    return RGB(
        clampChannel(GetRValue(color)),
        clampChannel(GetGValue(color)),
        clampChannel(GetBValue(color)));
}

RECT CellRect(int gridX, int gridY, int originX, int originY, int cellSize)
{
    RECT rect{};
    rect.left = originX + gridX * cellSize;
    rect.top = originY + gridY * cellSize;
    rect.right = rect.left + cellSize;
    rect.bottom = rect.top + cellSize;
    return rect;
}

void FillPolygon(HDC hdc, const POINT* points, int count, COLORREF fillColor, COLORREF borderColor)
{
    HBRUSH brush = CreateSolidBrush(fillColor);
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    Polygon(hdc, points, count);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
}

void FillRectColor(HDC hdc, const RECT& rect, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);
}

void FillRoundedRect(HDC hdc, const RECT& rect, int radius, COLORREF fillColor, COLORREF borderColor)
{
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HBRUSH brush = CreateSolidBrush(fillColor);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, radius, radius);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawTextLine(HDC hdc, int x, int y, int width, const std::wstring& text, int height, int weight)
{
    HFONT font = CreateFontW(
        height,
        0,
        0,
        0,
        weight,
        FALSE,
        FALSE,
        FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Segoe UI");

    HGDIOBJ oldFont = SelectObject(hdc, font);
    SetBkMode(hdc, TRANSPARENT);
    RECT textRect{ x, y, x + width, y + 40 };
    DrawTextW(hdc, text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

void DrawTileTop(HDC hdc, const RECT& rect, COLORREF color, COLORREF border)
{
    FillRoundedRect(hdc, rect, 8, color, border);

    RECT topHighlight{
        rect.left + 2,
        rect.top + 2,
        rect.right - 2,
        rect.top + 8
    };
    FillRectColor(hdc, topHighlight, ShadeColor(color, 1.08));

    RECT leftHighlight{
        rect.left + 2,
        rect.top + 2,
        rect.left + 6,
        rect.bottom - 2
    };
    FillRectColor(hdc, leftHighlight, ShadeColor(color, 1.05));

    RECT bottomShade{
        rect.left + 3,
        rect.bottom - 6,
        rect.right - 3,
        rect.bottom - 2
    };
    FillRectColor(hdc, bottomShade, ShadeColor(color, 0.82));
}

void FillEllipse(HDC hdc, int left, int top, int right, int bottom, COLORREF fillColor, COLORREF borderColor)
{
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HBRUSH brush = CreateSolidBrush(fillColor);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    Ellipse(hdc, left, top, right, bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawSegmentShadow(HDC hdc, const POINT& center, int width, int height)
{
    FillEllipse(
        hdc,
        center.x - width / 2 + 2,
        center.y - height / 2 + 10,
        center.x + width / 2 + 2,
        center.y + height / 2 + 10,
        RGB(175, 187, 178),
        RGB(175, 187, 178));
}

POINT CellCenter(int gridX, int gridY, int originX, int originY, int cellSize)
{
    const RECT rect = CellRect(gridX, gridY, originX, originY, cellSize);
    return POINT{ (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };
}

int CellNoise(const AppState& state, int x, int y)
{
    std::uint32_t value = static_cast<std::uint32_t>(state.decorSeed);
    value ^= 0x9e3779b9u + static_cast<std::uint32_t>(x) * 0x85ebca6bu + (value << 6) + (value >> 2);
    value ^= 0xc2b2ae35u + static_cast<std::uint32_t>(y) * 0x27d4eb2fu + (value << 6) + (value >> 2);
    value ^= value >> 15;
    value *= 0x2c1b3c6du;
    value ^= value >> 12;
    return static_cast<int>(value & 0x7fffffff);
}

bool CellHasSnake(const AppState& state, int x, int y)
{
    for (const snake::Point& segment : state.game.snake)
    {
        if (segment.x == x && segment.y == y)
        {
            return true;
        }
    }

    return false;
}

void DrawGrass(HDC hdc, const RECT& rect, COLORREF color)
{
    RECT shadow{
        rect.left + 9,
        rect.top + 17,
        rect.left + 24,
        rect.top + 25
    };
    FillEllipse(hdc, shadow.left, shadow.top, shadow.right, shadow.bottom, RGB(168, 181, 165), RGB(168, 181, 165));

    HPEN pen = CreatePen(PS_SOLID, 3, ShadeColor(color, 0.82));
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    const int baseY = rect.bottom - 7;
    MoveToEx(hdc, rect.left + 8, baseY, nullptr);
    LineTo(hdc, rect.left + 11, rect.top + 10);
    MoveToEx(hdc, rect.left + 13, baseY, nullptr);
    LineTo(hdc, rect.left + 16, rect.top + 7);
    MoveToEx(hdc, rect.left + 18, baseY, nullptr);
    LineTo(hdc, rect.left + 21, rect.top + 11);
    MoveToEx(hdc, rect.left + 12, baseY, nullptr);
    LineTo(hdc, rect.left + 9, rect.top + 12);
    MoveToEx(hdc, rect.left + 17, baseY, nullptr);
    LineTo(hdc, rect.left + 20, rect.top + 13);

    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    pen = CreatePen(PS_SOLID, 1, ShadeColor(color, 1.2));
    oldPen = SelectObject(hdc, pen);
    MoveToEx(hdc, rect.left + 9, baseY - 1, nullptr);
    LineTo(hdc, rect.left + 12, rect.top + 10);
    MoveToEx(hdc, rect.left + 14, baseY - 1, nullptr);
    LineTo(hdc, rect.left + 17, rect.top + 8);
    MoveToEx(hdc, rect.left + 19, baseY - 1, nullptr);
    LineTo(hdc, rect.left + 22, rect.top + 11);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

void DrawPebble(HDC hdc, const RECT& rect)
{
    FillEllipse(
        hdc,
        rect.left + 9,
        rect.top + 16,
        rect.left + 23,
        rect.top + 24,
        RGB(173, 182, 171),
        RGB(173, 182, 171));
    FillEllipse(
        hdc,
        rect.left + 10,
        rect.top + 14,
        rect.left + 21,
        rect.top + 22,
        RGB(156, 163, 152),
        RGB(132, 139, 129));
    FillEllipse(
        hdc,
        rect.left + 13,
        rect.top + 15,
        rect.left + 17,
        rect.top + 18,
        RGB(205, 210, 202),
        RGB(205, 210, 202));
}

void DrawEnvironment(HDC hdc, const RECT& rect, const AppState& state, int x, int y)
{
    if (CellHasSnake(state, x, y) || (state.game.food.x == x && state.game.food.y == y))
    {
        return;
    }

    const int noise = CellNoise(state, x, y);
    if (noise % 11 == 0)
    {
        DrawGrass(hdc, rect, RGB(78, 138, 79));
    }
    else if (noise % 17 == 0)
    {
        DrawPebble(hdc, rect);
    }
}

void DrawSnakeSegment(HDC hdc, const POINT& center, int width, int height, COLORREF color, bool isHead)
{
    const COLORREF outline = RGB(28, 50, 36);
    DrawSegmentShadow(hdc, center, width, height / 2);
    FillEllipse(
        hdc,
        center.x - width / 2,
        center.y - height / 2,
        center.x + width / 2,
        center.y + height / 2,
        color,
        outline);

    FillEllipse(
        hdc,
        center.x - width / 4,
        center.y - height / 2 + 4,
        center.x + width / 8,
        center.y - height / 7,
        ShadeColor(color, 1.18),
        ShadeColor(color, 1.18));

    if (!isHead)
    {
        return;
    }
}

void DrawCapsule(HDC hdc, const POINT& start, const POINT& end, int radius, COLORREF color, COLORREF outline)
{
    HPEN pen = CreatePen(PS_SOLID, 1, outline);
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);

    const int left = std::min(start.x, end.x) - radius;
    const int top = std::min(start.y, end.y) - radius;
    const int right = std::max(start.x, end.x) + radius;
    const int bottom = std::max(start.y, end.y) + radius;
    RoundRect(hdc, left, top, right, bottom, radius * 2, radius * 2);

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawLineStroke(HDC hdc, const POINT& start, const POINT& end, int width, COLORREF color)
{
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    MoveToEx(hdc, start.x, start.y, nullptr);
    LineTo(hdc, end.x, end.y);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

snake::Direction HeadFacing(const AppState& state)
{
    if (state.game.snake.size() < 2)
    {
        return state.game.direction;
    }

    const snake::Point& head = state.game.snake[0];
    const snake::Point& neck = state.game.snake[1];

    if (head.x > neck.x)
    {
        return snake::Direction::Right;
    }
    if (head.x < neck.x)
    {
        return snake::Direction::Left;
    }
    if (head.y > neck.y)
    {
        return snake::Direction::Down;
    }
    return snake::Direction::Up;
}

void DrawSnake(HDC hdc, const AppState& state, int originX, int originY, int cellSize)
{
    const int shadowOffset = 3;
    const int bodyRadius = 10;
    const int bodyWidth = bodyRadius * 2;
    const COLORREF shadowColor = RGB(158, 170, 161);
    const COLORREF bodyColor = RGB(86, 177, 108);
    const COLORREF bodyShade = RGB(60, 143, 80);
    const COLORREF outline = RGB(34, 64, 42);

    for (std::size_t index = state.game.snake.size(); index-- > 1;)
    {
        const snake::Point& previous = state.game.snake[index];
        const snake::Point& current = state.game.snake[index - 1];
        const POINT from = CellCenter(previous.x, previous.y, originX, originY, cellSize);
        const POINT to = CellCenter(current.x, current.y, originX, originY, cellSize);

        DrawCapsule(
            hdc,
            { from.x + shadowOffset, from.y + shadowOffset },
            { to.x + shadowOffset, to.y + shadowOffset },
            bodyRadius + 3,
            shadowColor,
            shadowColor);
    }

    for (std::size_t index = state.game.snake.size(); index-- > 1;)
    {
        const snake::Point& previous = state.game.snake[index];
        const snake::Point& current = state.game.snake[index - 1];
        const POINT from = CellCenter(previous.x, previous.y, originX, originY, cellSize);
        const POINT to = CellCenter(current.x, current.y, originX, originY, cellSize);

        DrawCapsule(hdc, from, to, bodyRadius + 1, outline, outline);
        DrawCapsule(hdc, from, to, bodyRadius, bodyColor, bodyColor);
        DrawLineStroke(hdc, { from.x - 2, from.y - 4 }, { to.x - 2, to.y - 4 }, 6, ShadeColor(bodyColor, 1.2));
        DrawLineStroke(hdc, { from.x + 3, from.y + 5 }, { to.x + 3, to.y + 5 }, 5, bodyShade);
    }

    for (std::size_t index = state.game.snake.size(); index-- > 0;)
    {
        const snake::Point& segment = state.game.snake[index];
        const POINT center = CellCenter(segment.x, segment.y, originX, originY, cellSize);
        const bool isHead = index == 0;
        const int width = isHead ? 26 : 22;
        const int height = isHead ? 30 : 22;
        DrawSnakeSegment(hdc, center, width, height, isHead ? RGB(78, 165, 99) : bodyColor, isHead);
    }

    const POINT headCenter = CellCenter(state.game.snake.front().x, state.game.snake.front().y, originX, originY, cellSize);
    const snake::Direction facing = HeadFacing(state);

    POINT nose[3]{};
    POINT tongueStart{};
    POINT tongueLeft{};
    POINT tongueRight{};
    RECT leftEye{};
    RECT rightEye{};
    RECT leftPupil{};
    RECT rightPupil{};

    switch (facing)
    {
    case snake::Direction::Up:
        nose[0] = { headCenter.x - 7, headCenter.y - 10 };
        nose[1] = { headCenter.x + 7, headCenter.y - 10 };
        nose[2] = { headCenter.x, headCenter.y - 20 };
        tongueStart = { headCenter.x, headCenter.y - 18 };
        tongueLeft = { headCenter.x - 4, headCenter.y - 28 };
        tongueRight = { headCenter.x + 4, headCenter.y - 28 };
        leftEye = { headCenter.x - 11, headCenter.y - 11, headCenter.x - 3, headCenter.y - 3 };
        rightEye = { headCenter.x + 3, headCenter.y - 11, headCenter.x + 11, headCenter.y - 3 };
        leftPupil = { headCenter.x - 8, headCenter.y - 8, headCenter.x - 5, headCenter.y - 5 };
        rightPupil = { headCenter.x + 6, headCenter.y - 8, headCenter.x + 9, headCenter.y - 5 };
        break;
    case snake::Direction::Down:
        nose[0] = { headCenter.x - 7, headCenter.y + 10 };
        nose[1] = { headCenter.x + 7, headCenter.y + 10 };
        nose[2] = { headCenter.x, headCenter.y + 20 };
        tongueStart = { headCenter.x, headCenter.y + 18 };
        tongueLeft = { headCenter.x - 4, headCenter.y + 28 };
        tongueRight = { headCenter.x + 4, headCenter.y + 28 };
        leftEye = { headCenter.x - 11, headCenter.y + 3, headCenter.x - 3, headCenter.y + 11 };
        rightEye = { headCenter.x + 3, headCenter.y + 3, headCenter.x + 11, headCenter.y + 11 };
        leftPupil = { headCenter.x - 8, headCenter.y + 6, headCenter.x - 5, headCenter.y + 9 };
        rightPupil = { headCenter.x + 6, headCenter.y + 6, headCenter.x + 9, headCenter.y + 9 };
        break;
    case snake::Direction::Left:
        nose[0] = { headCenter.x - 10, headCenter.y - 7 };
        nose[1] = { headCenter.x - 10, headCenter.y + 7 };
        nose[2] = { headCenter.x - 20, headCenter.y };
        tongueStart = { headCenter.x - 18, headCenter.y };
        tongueLeft = { headCenter.x - 28, headCenter.y - 4 };
        tongueRight = { headCenter.x - 28, headCenter.y + 4 };
        leftEye = { headCenter.x - 12, headCenter.y - 11, headCenter.x - 4, headCenter.y - 3 };
        rightEye = { headCenter.x - 12, headCenter.y + 3, headCenter.x - 4, headCenter.y + 11 };
        leftPupil = { headCenter.x - 9, headCenter.y - 8, headCenter.x - 6, headCenter.y - 5 };
        rightPupil = { headCenter.x - 9, headCenter.y + 6, headCenter.x - 6, headCenter.y + 9 };
        break;
    case snake::Direction::Right:
    default:
        nose[0] = { headCenter.x + 10, headCenter.y - 7 };
        nose[1] = { headCenter.x + 10, headCenter.y + 7 };
        nose[2] = { headCenter.x + 20, headCenter.y };
        tongueStart = { headCenter.x + 18, headCenter.y };
        tongueLeft = { headCenter.x + 28, headCenter.y - 4 };
        tongueRight = { headCenter.x + 28, headCenter.y + 4 };
        leftEye = { headCenter.x + 4, headCenter.y - 11, headCenter.x + 12, headCenter.y - 3 };
        rightEye = { headCenter.x + 4, headCenter.y + 3, headCenter.x + 12, headCenter.y + 11 };
        leftPupil = { headCenter.x + 7, headCenter.y - 8, headCenter.x + 10, headCenter.y - 5 };
        rightPupil = { headCenter.x + 7, headCenter.y + 6, headCenter.x + 10, headCenter.y + 9 };
        break;
    }

    FillPolygon(hdc, nose, 3, RGB(78, 165, 99), outline);
    FillEllipse(hdc, leftEye.left, leftEye.top, leftEye.right, leftEye.bottom, RGB(248, 248, 240), outline);
    FillEllipse(hdc, rightEye.left, rightEye.top, rightEye.right, rightEye.bottom, RGB(248, 248, 240), outline);
    FillEllipse(hdc, leftPupil.left, leftPupil.top, leftPupil.right, leftPupil.bottom, RGB(18, 22, 18), RGB(18, 22, 18));
    FillEllipse(hdc, rightPupil.left, rightPupil.top, rightPupil.right, rightPupil.bottom, RGB(18, 22, 18), RGB(18, 22, 18));

    HPEN tonguePen = CreatePen(PS_SOLID, 2, RGB(190, 52, 62));
    HGDIOBJ oldTonguePen = SelectObject(hdc, tonguePen);
    MoveToEx(hdc, tongueStart.x, tongueStart.y, nullptr);
    LineTo(hdc, tongueLeft.x, tongueLeft.y);
    MoveToEx(hdc, tongueStart.x, tongueStart.y, nullptr);
    LineTo(hdc, tongueRight.x, tongueRight.y);
    SelectObject(hdc, oldTonguePen);
    DeleteObject(tonguePen);
}

void DrawFood(HDC hdc, const RECT& rect)
{
    const POINT center{ (rect.left + rect.right) / 2, (rect.top + rect.bottom) / 2 };
    const int radius = (rect.right - rect.left) / 4;

    FillEllipse(
        hdc,
        center.x - radius + 2,
        center.y - radius + 6,
        center.x + radius + 2,
        center.y + radius + 6,
        RGB(164, 147, 147),
        RGB(164, 147, 147));

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(120, 30, 30));
    HBRUSH brush = CreateSolidBrush(RGB(238, 84, 84));
    HGDIOBJ oldPen = SelectObject(hdc, pen);
    HGDIOBJ oldBrush = SelectObject(hdc, brush);

    Ellipse(hdc, center.x - radius, center.y - radius, center.x + radius, center.y + radius);
    MoveToEx(hdc, center.x, center.y - radius, nullptr);
    LineTo(hdc, center.x + 5, center.y - radius - 12);
    FillEllipse(
        hdc,
        center.x - radius / 2,
        center.y - radius / 2,
        center.x - radius / 8,
        center.y - radius / 8,
        RGB(255, 200, 200),
        RGB(255, 200, 200));

    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(pen);
    DeleteObject(brush);
}

void DrawBoard(HDC hdc, const RECT& clientRect, const AppState& state)
{
    const int cellSize = 30;
    const int boardWidthPixels = state.game.width * cellSize;
    const int boardHeightPixels = state.game.height * cellSize;
    const int originX = clientRect.left + (clientRect.right - clientRect.left - boardWidthPixels) / 2;
    const int originY = clientRect.top + 170;

    RECT boardRect{
        originX,
        originY,
        originX + boardWidthPixels,
        originY + boardHeightPixels
    };
    RECT platformShadow{
        boardRect.left + 16,
        boardRect.top + 16,
        boardRect.right + 18,
        boardRect.bottom + 18
    };
    FillRoundedRect(hdc, platformShadow, 18, RGB(186, 198, 191), RGB(186, 198, 191));

    RECT platformRect{
        boardRect.left - 14,
        boardRect.top - 14,
        boardRect.right + 14,
        boardRect.bottom + 14
    };
    FillRoundedRect(hdc, platformRect, 18, RGB(116, 133, 109), RGB(88, 103, 84));

    RECT platformTop{
        platformRect.left + 8,
        platformRect.top + 8,
        platformRect.right - 8,
        platformRect.bottom - 8
    };
    FillRoundedRect(hdc, platformTop, 16, RGB(208, 216, 202), RGB(162, 174, 157));

    FillRoundedRect(hdc, boardRect, 14, RGB(179, 200, 178), RGB(126, 145, 121));

    for (int y = 0; y < state.game.height; ++y)
    {
        for (int x = 0; x < state.game.width; ++x)
        {
            const RECT rect = CellRect(x, y, originX, originY, cellSize);
            const bool darkTile = ((x + y) % 2) == 0;
            DrawTileTop(
                hdc,
                rect,
                darkTile ? RGB(199, 218, 191) : RGB(184, 205, 177),
                RGB(144, 164, 136));
            DrawEnvironment(hdc, rect, state, x, y);
        }
    }

    DrawSnake(hdc, state, originX, originY, cellSize);

    if (state.game.food.x >= 0 && state.game.food.y >= 0)
    {
        DrawFood(hdc, CellRect(state.game.food.x, state.game.food.y, originX, originY, cellSize));
    }

    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(86, 104, 82));
    HGDIOBJ oldPen = SelectObject(hdc, borderPen);
    HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(hdc, boardRect.left, boardRect.top, boardRect.right, boardRect.bottom);
    SelectObject(hdc, oldPen);
    SelectObject(hdc, oldBrush);
    DeleteObject(borderPen);
}

void DrawHud(HDC hdc, const RECT& clientRect, const AppState& state)
{
    RECT titleBand{ clientRect.left + 32, clientRect.top + 26, clientRect.right - 32, clientRect.top + 118 };
    FillRectColor(hdc, titleBand, RGB(247, 250, 247));

    SetTextColor(hdc, RGB(33, 48, 40));
    DrawTextLine(hdc, 48, 38, 420, L"Classic Snake", 34, FW_SEMIBOLD);

    std::wstring subtitle = L"Score: " + std::to_wstring(state.game.score);
    if (state.game.isPaused)
    {
        subtitle += L"   Paused";
    }
    if (state.game.isGameOver)
    {
        subtitle += L"   Game Over";
    }

    DrawTextLine(hdc, 50, 80, 420, subtitle, 22, FW_NORMAL);
    DrawTextLine(hdc, clientRect.right - 380, 52, 330, L"WASD / Arrows  Move    P  Pause    R  Restart", 19, FW_NORMAL);
}

void DrawScene(HDC hdc, const RECT& clientRect, const AppState& state)
{
    FillRectColor(hdc, clientRect, RGB(224, 231, 227));

    RECT wall{ clientRect.left, clientRect.top, clientRect.right, clientRect.top + 220 };
    FillRectColor(hdc, wall, RGB(240, 243, 239));

    RECT floor{ clientRect.left, clientRect.top + 220, clientRect.right, clientRect.bottom };
    FillRectColor(hdc, floor, RGB(214, 221, 216));

    for (int stripe = clientRect.top + 220; stripe < clientRect.bottom; stripe += 26)
    {
        RECT band{ clientRect.left, stripe, clientRect.right, stripe + 4 };
        FillRectColor(hdc, band, RGB(208, 215, 210));
    }

    DrawHud(hdc, clientRect, state);
    DrawBoard(hdc, clientRect, state);
}

bool CanQueueDirection(const AppState& state, snake::Direction candidate)
{
    snake::Direction comparison = state.game.direction;
    if (!state.directionQueue.empty())
    {
        comparison = state.directionQueue.back();
    }

    return !snake::IsOppositeDirection(comparison, candidate);
}

void QueueDirection(AppState& state, snake::Direction candidate)
{
    if (!CanQueueDirection(state, candidate))
    {
        return;
    }

    if (!state.directionQueue.empty() && state.directionQueue.back() == candidate)
    {
        return;
    }

    if (state.directionQueue.size() >= 2)
    {
        return;
    }

    state.directionQueue.push_back(candidate);
}

void AdvanceGame(AppState& state)
{
    std::optional<snake::Direction> nextDirection;
    if (!state.directionQueue.empty())
    {
        nextDirection = state.directionQueue.front();
        state.directionQueue.pop_front();
    }

    snake::StepGame(state.game, nextDirection, NextFoodSeed(state));
}

void PaintWindow(HWND hwnd)
{
    PAINTSTRUCT ps{};
    HDC windowDc = BeginPaint(hwnd, &ps);

    RECT clientRect{};
    GetClientRect(hwnd, &clientRect);

    HDC memoryDc = CreateCompatibleDC(windowDc);
    HBITMAP bitmap = CreateCompatibleBitmap(windowDc, clientRect.right, clientRect.bottom);
    HGDIOBJ oldBitmap = SelectObject(memoryDc, bitmap);

    DrawScene(memoryDc, clientRect, GetAppState());

    BitBlt(windowDc, 0, 0, clientRect.right, clientRect.bottom, memoryDc, 0, 0, SRCCOPY);

    SelectObject(memoryDc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memoryDc);
    EndPaint(hwnd, &ps);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    AppState& state = GetAppState();

    switch (message)
    {
    case WM_CREATE:
        RestartGame(state);
        SetTimer(hwnd, kGameTimerId, kTickMilliseconds, nullptr);
        return 0;

    case WM_TIMER:
        if (wParam == kGameTimerId && !state.game.isPaused && !state.game.isGameOver)
        {
            AdvanceGame(state);
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        return 0;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case 'W':
        case VK_UP:
            QueueDirection(state, snake::Direction::Up);
            break;
        case 'S':
        case VK_DOWN:
            QueueDirection(state, snake::Direction::Down);
            break;
        case 'A':
        case VK_LEFT:
            QueueDirection(state, snake::Direction::Left);
            break;
        case 'D':
        case VK_RIGHT:
            QueueDirection(state, snake::Direction::Right);
            break;
        case 'P':
            if (!state.game.isGameOver)
            {
                state.game.isPaused = !state.game.isPaused;
            }
            break;
        case 'R':
            RestartGame(state);
            break;
        }

        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_PAINT:
        PaintWindow(hwnd);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        KillTimer(hwnd, kGameTimerId);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int commandShow)
{
    const wchar_t kClassName[] = L"Snake3DWindow";
    const DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;

    RegisterClassW(&windowClass);

    RECT desiredRect{ 0, 0, kWindowWidth, kWindowHeight };
    AdjustWindowRect(&desiredRect, windowStyle, FALSE);

    HWND hwnd = CreateWindowExW(
        0,
        kClassName,
        L"Snake 3D",
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        desiredRect.right - desiredRect.left,
        desiredRect.bottom - desiredRect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (hwnd == nullptr)
    {
        return 0;
    }

    ShowWindow(hwnd, commandShow);
    UpdateWindow(hwnd);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0))
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
