#include "game.hpp"
#include <cstdio>

int main(int /*argc*/, char** /*argv*/) {
    Game game;
    if (!game.init()) {
        std::fprintf(stderr, "Failed to initialize KugelMatch.\n");
        return 1;
    }
    game.run();
    game.shutdown();
    return 0;
}
