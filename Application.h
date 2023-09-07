//
// Created by rolark on 02.08.23.
//

#ifndef CHECK_LINKS_REWRITE_APPLICATION_H
#define CHECK_LINKS_REWRITE_APPLICATION_H

#include <string>
#include <iostream>
#include <queue>
#include <chrono>

#include <cpr/cpr.h>
#include <boost/locale.hpp>
#include <boost/url.hpp>


//gumbo-query
#include "Document.h"
#include "Node.h"

#include "Page.h"

class Application {
private:
    enum class request_type{
        Head,
        Get
    };
public:
    explicit Application(std::string encoding, std::string start_url,
                         std::string css_query, std::string user_agent,
                         const unsigned int depth, bool async_mode,
                         std::ostream* error_stream, std::ostream* log_stream)
    : encoding(std::move(encoding)), start_url(std::move(start_url)),
    css_query(std::move(css_query)), user_agent(std::move(user_agent)),
    error_stream(error_stream), log_stream(log_stream),
    depth(depth), async_mode(async_mode)
    {} ;
    ~Application() = default;

    int run();

private:
    void get_child_links(Page& page);
    std::vector<std::string> get_links(std::string page_content);
    std::string fix_encoding(const std::string& input);
    static std::string normalize_url(const std::string& link, const std::string& base_url);

    static bool is_same_domain(const std::string& url1, const std::string& url2);
    static bool is_pdf_file(const std::string& url);

    decltype(cpr::HeadAsync())  async_head_request(std::string url);
    decltype(cpr::GetAsync())   async_get_request(std::string url);
    decltype(cpr::Get())        sync_get_request(std::string url);
    decltype(cpr::Head())       sync_head_request(std::string url);

    static void print_request_log(std::ostream& log_stream, std::ostream& error_stream, request_type req_type, const cpr::Response& response);


    const bool async_mode;
    const std::string encoding;
    const std::string start_url;
    const std::string css_query;
    std::ostream* error_stream;
    std::ostream* log_stream;
    const unsigned int depth;
    const std::string user_agent;
};


#endif //CHECK_LINKS_REWRITE_APPLICATION_H
