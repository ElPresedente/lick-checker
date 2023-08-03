//
// Created by rolark on 02.08.23.
//

#ifndef CHECK_LINKS_REWRITE_PAGE_H
#define CHECK_LINKS_REWRITE_PAGE_H

#include <string>
#include <unordered_set>

struct Page{
    std::string url;
    std::unordered_set<std::string> child_urls;
    unsigned int depth;
    explicit Page(std::string url, unsigned int depth) : url(std::move(url)), depth(depth), child_urls() {};
};

#endif //CHECK_LINKS_REWRITE_PAGE_H
