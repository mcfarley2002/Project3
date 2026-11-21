#include "scene.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#endif

static bool interactive_stdin() {
#if defined(__unix__) || defined(__APPLE__)
    return isatty(0) != 0;
#else
    return false;
#endif
}

static void help() {
    std::cout <<
        "COMMANDS (names are single words; coordinates are integers)\n"
        "  add PARENT NAME X Y WIDTH HEIGHT\n"
        "  move NAME DX DY       worldpos NAME\n"
        "  intersects A B        swap A B\n"
        "  erase NAME            tree\n"
        "  select NAME           info\n"
        "  delete-selected       cut\n"
        "  paste PARENT          clear-clipboard\n"
        "  unselect              help / quit\n";
}

int main() {
    Editor editor;
    std::string line;
    const bool prompt = interactive_stdin();
    if (prompt) { help(); std::cout << "scene> " << std::flush; }
    while (std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::vector<std::string> v;
        for (std::string token; input >> token;) {
            if (token[0] == '#') break; // trailing comments are allowed
            v.push_back(token);
        }
        if (v.empty()) { if (prompt) std::cout << "scene> " << std::flush; continue; }
        const auto& command = v.front();
        if (command == "quit" || command == "exit") break;
        try {
            if (command == "help" && v.size() == 1) help();
            else if (command == "tree" && v.size() == 1) editor.print_tree(std::cout);
            else if (command == "add" && v.size() == 7)
                std::cout << (editor.add(v[1], v[2], Rect{std::stoi(v[3]), std::stoi(v[4]),
                    std::stoi(v[5]), std::stoi(v[6])}) ? "OK" : "ERR") << '\n';
            else if (command == "move" && v.size() == 4)
                std::cout << (editor.move_node(v[1], std::stoi(v[2]), std::stoi(v[3])) ? "OK" : "ERR") << '\n';
            else if (command == "worldpos" && v.size() == 2) {
                if (auto p = editor.world_position(v[1])) std::cout << "WORLD " << p->x << ' ' << p->y << '\n';
                else std::cout << "ERR\n";
            }
            else if (command == "intersects" && v.size() == 3) {
                auto result = editor.intersects(v[1], v[2]);
                std::cout << (!result ? "ERR" : (*result ? "YES" : "NO")) << '\n';
            }
            else if (command == "swap" && v.size() == 3)
                std::cout << (editor.swap_positions(v[1], v[2]) ? "OK" : "ERR") << '\n';
            else if (command == "erase" && v.size() == 2)
                std::cout << (editor.erase(v[1]) ? "OK" : "ERR") << '\n';
            else if (command == "select" && v.size() == 2)
                std::cout << (editor.select(v[1]) ? "OK" : "ERR") << '\n';
            else if (command == "info" && v.size() == 1) std::cout << editor.info() << '\n';
            else if (command == "delete-selected" && v.size() == 1)
                std::cout << (editor.delete_selected() ? "OK" : "ERR") << '\n';
            else if (command == "cut" && v.size() == 1)
                std::cout << (editor.cut() ? "OK" : "ERR") << '\n';
            else if (command == "paste" && v.size() == 2)
                std::cout << (editor.paste(v[1]) ? "OK" : "ERR") << '\n';
            else if (command == "unselect" && v.size() == 1) { editor.unselect(); std::cout << "OK\n"; }
            else if (command == "clear-clipboard" && v.size() == 1) {
                editor.clear_clipboard(); std::cout << "OK\n";
            }
            else std::cout << "ERR usage (type help)\n";
        } catch (const std::invalid_argument&) { std::cout << "ERR integer argument\n"; }
          catch (const std::out_of_range&) { std::cout << "ERR integer out of range\n"; }
        if (prompt) std::cout << "scene> " << std::flush;
    }
}
