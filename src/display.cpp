// Original authors:
// - Florian Duruz
// - Aellen Quentin

#include <iostream>
#include <regex>
#include <utility>

#ifdef _WIN32

#include <windows.h>

int enableVirtualTerminalProcessing() {

    HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (stdOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(stdOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(stdOut, mode)) {
                return 0;
            }
        }
    }

    return GetLastError();
}

#endif

#include "libdio/display.h"

using namespace std;

const string TOP_LEFT_PIPE = "╔";
const string TOP_RIGHT_PIPE = "╗";
const string HORIZONTAL_PIPE = "═";
const string HORIZONTAL_UP_PIPE = "╩";
const string HORIZONTAL_DOWN_PIPE = "╦";
const string VERTICAL_PIPE = "║";
const string VERTICAL_LEFT_PIPE = "╣";
const string VERTICAL_RIGHT_PIPE = "╠";
const string BOT_LEFT_PIPE = "╚";
const string BOT_RIGHT_PIPE = "╝";
const string CROSS_PIPE = "╬";

namespace Display {

    string nonANSI(const string &s);

    DString::DString(Display::Color c) {
        setColor(c);
    }
    DString &DString::setColor(Color color) {
        out += "\x1b[38;5;" + to_string(int(color)) + "m";
        return *this;
    }

    DString &DString::operator+=(const DString &ds) {
        out += ds.out;
        return *this;
    }
    DString operator+(DString left, const DString &right) {
        return left += right;
    }

    DString &DString::operator+=(const string &s) {
        out += s;
        return *this;
    }
    DString operator+(DString left, const string &right) {
        return left += right;
    }

    DString &DString::operator<<(const DString &obj) {
        out += obj.out;
        return *this;
    }
    DString &DString::operator<<(const std::string &obj) {
        out += obj;
        return *this;
    }
    DString &DString::operator<<(const char obj[]) {
        out += obj;
        return *this;
    }
    DString &DString::operator<<(const Color &col) {
        setColor(col);
        return *this;
    }

    void DString::print() {
        cout << out;
        reset();
    }

    void DString::reset(Color color) {
        out = "";
        setColor(color);
    }

    DString &DString::saveCursorPosition() {
        out += "\x1b[s";
        return *this;
    }
    DString &DString::goBackToCursorPosition() {
        out += "\x1b[u";
        return *this;
    }
    DString &DString::clearScreen() {
        out += "\x1b[2J\x1b[H";
        return *this;
    }

    size_t DString::max_width() {
        size_t max_width = 0;
        for (auto line: split(out, "\n")) {
            max_width = max(max_width, nonANSI(line).size());
        }
        return max_width;
    }

    size_t DString::count_lines() {
        return size_t(std::count_if(out.begin(), out.end(), [](char c) -> bool { return c == '\n'; }));
    }

    ostream &operator<<(ostream &os, const DString &obj) {
        os << obj.out;
        return os;
    }

    vector<string> split(string s, string sep, size_t max_sep) {
        vector<string> ret;
        size_t last = 0;
        size_t next = 0;
        while ((next = s.find(sep, last)) != string::npos &&
               (ret.size() < max_sep || max_sep == 0)) {
            ret.push_back(s.substr(last, next - last));
            last = next + 1;
        }
        ret.push_back(s.substr(last));
        return ret;
    }

    // Returns the string without any ANSI sequences
    string nonANSI(const string &s) {
        return regex_replace(s, regex("\x1b.*?[a-zA-Z]"), "");
    }

    void init() {
#ifdef _WIN32
        enableVirtualTerminalProcessing();
        SetConsoleOutputCP(CP_UTF8);
#endif
    }

    void systemClearScreen() {
#ifdef _WIN32
        system("cls");
#elif __unix__
        system("clear");
#endif
    }

    void saveCursorPosition() {
        DString().saveCursorPosition().print();
    }
    void restoreCursorPosition() {
        DString().goBackToCursorPosition().print();
    }
    void clearScreen() {
        DString().clearScreen().print();
    }

    string drawFrame(string first, string middle, string last, string space, size_t cells, size_t width) {
        string line = first;
        string spaces;
        for (size_t i = 0; i <= width + 1; i++) {
            spaces += space;
        }
        for (size_t i = 1; i < cells; i++) {
            line += spaces + middle;
        }
        return line + spaces + last;
    }

    string drawInBetween(size_t cells, size_t width) {
        return drawFrame(VERTICAL_RIGHT_PIPE, CROSS_PIPE, VERTICAL_LEFT_PIPE, HORIZONTAL_PIPE, cells, width);
    }

    string drawTopFrame(size_t cells, size_t width) {
        return drawFrame(TOP_LEFT_PIPE, HORIZONTAL_DOWN_PIPE, TOP_RIGHT_PIPE, HORIZONTAL_PIPE, cells, width);
    }

    string drawBottomFrame(size_t cells, size_t width) {
        return drawFrame(BOT_LEFT_PIPE, HORIZONTAL_UP_PIPE, BOT_RIGHT_PIPE, HORIZONTAL_PIPE, cells, width);
    }

    template<>
    DString displayGrid<DString>(const vector<vector<DString>> &grid, bool show_grid) {
        size_t max_width = 0, max_height = 0;
        for (const auto &line: grid) {
            for (auto cell: line) {
                max_width = max(max_width, cell.max_width());
                max_height = max(max_height, cell.count_lines());
            }
        }
        size_t cells = grid[0].size();

        DString out;

        if (show_grid) {
            out << drawTopFrame(cells, max_width) + "\n";
        }

        for (size_t y = 0; y < grid.size(); ++y) {
            if (show_grid) {
                if (y != 0) {
                    out << drawInBetween(cells, max_width) + "\n";
                }
                out << Color::WHITE << VERTICAL_PIPE << " ";
            }
            for (size_t x = 0; x < grid.at(0).size(); ++x) {
                DString cell = grid.at(y).at(x);
                while (cell.count_lines() < max_height) {
                    cell << "\n";
                }
                while (cell.max_width() < max_width) {
                    cell << " ";
                }
                out << cell;

                out << " ";
                if (show_grid && x != grid.at(0).size() - 1) {
                    out << Color::WHITE << VERTICAL_PIPE << " ";
                }
            }
            if (show_grid) {
                out << Color::WHITE << VERTICAL_PIPE;
            }
            out << "\n";
        }
        if (show_grid) {
            out << drawBottomFrame(cells, max_width) + "\n";
        }
        return out;
    }

    template<>
    DString displayGrid<string>(const vector<vector<string>> &grid, bool show_grid) {
        return displayGrid<string>(
                grid, [](const string &val) -> DString { return DString() << val; }, show_grid);
    }
}// namespace Display
