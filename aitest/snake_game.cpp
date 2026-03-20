#include "snake_game.h"

namespace snake
{
namespace
{
Point NextHeadPosition(const Point& head, Direction direction)
{
    switch (direction)
    {
    case Direction::Up:
        return { head.x, head.y - 1 };
    case Direction::Down:
        return { head.x, head.y + 1 };
    case Direction::Left:
        return { head.x - 1, head.y };
    case Direction::Right:
    default:
        return { head.x + 1, head.y };
    }
}

bool ContainsPoint(const std::vector<Point>& points, const Point& point, std::size_t limit)
{
    for (std::size_t index = 0; index < limit; ++index)
    {
        if (points[index] == point)
        {
            return true;
        }
    }

    return false;
}
} // namespace

bool Point::operator==(const Point& other) const
{
    return x == other.x && y == other.y;
}

GameState CreateGame(int width, int height, int foodSeed)
{
    GameState state{};
    state.width = width;
    state.height = height;
    state.direction = Direction::Right;
    state.score = 0;
    state.isGameOver = false;
    state.isPaused = false;

    const int centerX = width / 2;
    const int centerY = height / 2;
    state.snake = {
        { centerX, centerY },
        { centerX - 1, centerY },
        { centerX - 2, centerY }
    };

    const auto food = FindFoodPosition(state, foodSeed);
    state.food = food.value_or(Point{ -1, -1 });
    return state;
}

bool IsOppositeDirection(Direction current, Direction candidate)
{
    return (current == Direction::Up && candidate == Direction::Down)
        || (current == Direction::Down && candidate == Direction::Up)
        || (current == Direction::Left && candidate == Direction::Right)
        || (current == Direction::Right && candidate == Direction::Left);
}

std::optional<Point> FindFoodPosition(const GameState& state, int startIndex)
{
    const int cellCount = state.width * state.height;
    if (static_cast<int>(state.snake.size()) >= cellCount)
    {
        return std::nullopt;
    }

    int index = startIndex % cellCount;
    if (index < 0)
    {
        index += cellCount;
    }

    for (int offset = 0; offset < cellCount; ++offset)
    {
        const int candidateIndex = (index + offset) % cellCount;
        Point candidate{ candidateIndex % state.width, candidateIndex / state.width };

        if (!ContainsPoint(state.snake, candidate, state.snake.size()))
        {
            return candidate;
        }
    }

    return std::nullopt;
}

void StepGame(GameState& state, std::optional<Direction> requestedDirection, int foodSeed)
{
    if (state.isGameOver || state.isPaused)
    {
        return;
    }

    if (requestedDirection.has_value() && !IsOppositeDirection(state.direction, requestedDirection.value()))
    {
        state.direction = requestedDirection.value();
    }

    const Point nextHead = NextHeadPosition(state.snake.front(), state.direction);
    const bool willGrow = nextHead == state.food;
    const std::size_t collisionLength = willGrow ? state.snake.size() : state.snake.size() - 1;

    if (nextHead.x < 0 || nextHead.y < 0 || nextHead.x >= state.width || nextHead.y >= state.height
        || ContainsPoint(state.snake, nextHead, collisionLength))
    {
        state.isGameOver = true;
        return;
    }

    state.snake.insert(state.snake.begin(), nextHead);

    if (willGrow)
    {
        ++state.score;
        const auto food = FindFoodPosition(state, foodSeed);
        if (food.has_value())
        {
            state.food = food.value();
        }
        else
        {
            state.food = { -1, -1 };
            state.isGameOver = true;
        }
    }
    else
    {
        state.snake.pop_back();
    }
}
} // namespace snake
