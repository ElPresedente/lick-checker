//
// Created by rolark on 03.07.23.
//

#ifndef CHECK_LINKS_REWRITE_LINK_CHECKER_H
#define CHECK_LINKS_REWRITE_LINK_CHECKER_H

#include <string>
#include <cinttypes>
#include <unordered_set>
#include <memory>
#include <vector>



#include <boost/locale.hpp>

#include <uriparser/Uri.h>

#include "Document.h"
#include "Node.h"

struct link_checker_cleanup;

class link_checker {
    friend struct link_checker_cleanup;
public:
    explicit link_checker(const std::string& url, uint32_t depth = 0);
    void parse();
private:
    static std::ostream* error_stream;
    static std::ostream* log_stream;
    static std::unordered_set<std::string>* checked_links;
    static uint32_t max_depth;
    static bool normalize_url;
    static std::string encoding;
    static std::string links_selector;
    static std::string user_agent;
    static std::string base_url;

    bool is_checkable() const;

    [[nodiscard]] std::string resolve_and_normalize_url(const std::string& query) const;

    uint32_t depth;
    std::string url;
};

struct link_checker_cleanup{
    explicit link_checker_cleanup(uint32_t max_depth, std::string base_url, std::string encoding, std::string links_css_selector, bool normalize_url = true){
        link_checker::max_depth = max_depth;
        link_checker::base_url = std::move(base_url);
        link_checker::checked_links = new std::unordered_set<std::string>{};
        link_checker::normalize_url = normalize_url;
        link_checker::encoding = std::move(encoding);
        link_checker::links_selector = std::move(links_css_selector);
    }

    [[maybe_unused]] void set_error_stream(std::ostream* error_stream){
        link_checker::error_stream = error_stream;
    }
    [[maybe_unused]] void set_log_stream(std::ostream* log_stream){
        link_checker::log_stream = log_stream;
    }
    [[maybe_unused]] void set_user_agent(std::string user_agent){
        link_checker::user_agent = std::move(user_agent);
    }
    ~link_checker_cleanup(){
        link_checker::max_depth = 0;
        link_checker::error_stream = &std::cerr;
        link_checker::log_stream = &std::cout;
        delete link_checker::checked_links;
        link_checker::checked_links = nullptr;
        link_checker::normalize_url = true;
        link_checker::encoding = "utf-8";
        link_checker::links_selector = "";
        link_checker::user_agent = "curl/" LIBCURL_VERSION;
    }
};

#endif //CHECK_LINKS_REWRITE_LINK_CHECKER_H
