#ifndef TETRIS_POSITION_H
#define TETRIS_POSITION_H

#include <atomic>

struct Position
{
    std::atomic<int> x, y;

    Position() : x(0), y(0) {}

    Position(int x_, int y_){
      x.store(x_); y.store(y_);
    }

    Position (const Position & rhs){
      x.store(rhs.x.load()); y.store(rhs.y.load());
    }
    
    Position & operator = (Position rhs)
    {
      x.store(rhs.x.load()); y.store(rhs.y.load());
      return *this;
    }
};

#endif  // TETRIS_POSITION_H
