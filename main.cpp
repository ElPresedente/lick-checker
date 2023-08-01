#include <iostream>

#include "link_checker.h"

int main() {
    std::ofstream error{"report.txt"};
    link_checker_cleanup cleanup{2, "https://oreluniver.ru", "windows-1251", ".page-content a", true};
    cleanup.set_error_stream(&error);
    cleanup.set_user_agent("Mozilla/5.0");

    link_checker checker{"https://oreluniver.ru/sveden"};
    checker.parse();
    return 0;
}
