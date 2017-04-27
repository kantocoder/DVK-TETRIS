#include "Tetris.h"
#include <stdexcept>

int main()
{
    try {
        Tetris t;
        t.run();
    }
    catch (std::runtime_error & e)
    {
        std::cout <<"Exception caught. Message: '"<<e.what()<<"'. Terminating.\n";
    }
}
