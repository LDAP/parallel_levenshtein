#pragma once

#include <iostream>
#include <ostream>

template <class T> void print(std::ostream& out, const T& t, size_t w = 0) {
    if (w != 0)
        out.width(w);
    out << t << " " << std::flush;
}

template <class T, bool print_to_stdout = true> void print_and_log(std::ostream& out, const T& t, size_t w = 0) {
    print(out, t, w);
    if constexpr (print_to_stdout)
        print(std::cout, t, w);
}
