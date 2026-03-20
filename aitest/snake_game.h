#pragma once

#include <optional>
#include <vector>

namespace snake
{
struct Point
{
    int x;
    int y;

    bool operator==(const Point& other) const;
};

enum class Direction
{
    Up,
    Down,
    Left,
    Right
};

struct GameState
{
    int width;
    int height;
    std::vector<Point> snake;
    Point food;
    Direction direction;
    int score;
    bool isGameOver;
    bool isPaused;
};

GameState CreateGame(int width, int height, int foodSeed = 0);
bool IsOppositeDirection(Direction current, Direction candidate);
std::optional<Point> FindFoodPosition(const GameState& state, int startIndex);
void StepGame(GameState& state, std::optional<Direction> requestedDirection, int foodSeed);
} // namespace snake
