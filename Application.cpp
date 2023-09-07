//
// Created by rolark on 02.08.23.
//

#include "Application.h"


int Application::run() {

    (*log_stream) << "Start in " << (async_mode ? "async" : "sync") << " mode" << std::endl;

    Page start_page{start_url, 0};

    std::queue<Page> process_queue;
    process_queue.emplace(start_page);

    while(!process_queue.empty()){
        Page current_page = process_queue.front();
        process_queue.pop();
        if(current_page.depth >= depth)
            continue;
        get_child_links(current_page);
        (*log_stream) << "URL: " << current_page.url << " links:" << std::endl;
        auto begin = std::chrono::steady_clock::now();
        if(async_mode){
            std::vector<cpr::AsyncResponse> head_responses;

            for(auto&& link : current_page.child_urls){
                head_responses.emplace_back(async_head_request(link));
            }
            for(unsigned int i = 1; auto&& resp_promise : head_responses){
                resp_promise.wait();
                auto response = resp_promise.get();
                (*log_stream) << '['<<i++<<'/'<<current_page.child_urls.size()<<']';
                print_request_log((*log_stream), (*error_stream), request_type::Head, response);
                if(response.status_code == 200 ){
                    const std::string& current_url = response.url.str();
                    try{
                        if(is_same_domain(current_page.url, current_url)
                           && !is_pdf_file(current_url))
                        {
                            process_queue.emplace(response.url.str(), current_page.depth + 1);
                        }
                    }
                    catch(std::exception& err){
                        (*error_stream) << "ERROR at page " << current_page.url << "; url "
                            << response.url.str() << " - " << err.what() << std::endl;
                    }
                }
            }
        }
        else{
            for(unsigned int i = 0; auto&& link : current_page.child_urls){
                auto response = sync_head_request(link);
                (*log_stream) << '['<<i++<<'/'<<current_page.child_urls.size()<<']';
                print_request_log((*log_stream), (*error_stream), request_type::Head, response);
                if(response.status_code == 200 ){
                    const std::string& current_url = response.url.str();
                    try{
                        if(is_same_domain(current_page.url, current_url)
                           && !is_pdf_file(current_url))
                        {
                            process_queue.emplace(response.url.str(), current_page.depth + 1);
                        }
                    }
                    catch(std::exception& err){
                        (*error_stream) << "ERROR at page " << current_page.url << "; url "
                            << response.url.str() << " - " << err.what() << std::endl;
                    }
                }
            }
        }
        auto end = std::chrono::steady_clock::now();
        (*log_stream) << "Elapsed time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
    }
    return 0;
}

void Application::get_child_links(Page &page) {
    auto resp = sync_get_request(page.url);

    print_request_log((*log_stream), (*error_stream), request_type::Get, resp);

    std::string page_content = fix_encoding(resp.text);

    auto raw_links = get_links(page_content);
    // чиним полученные ссылки

    for(auto&& link : raw_links){
        try{
            page.child_urls.insert(normalize_url(link, page.url));
        }
        catch(std::runtime_error& err){
            (*error_stream) << "ERROR at page " << page.url << "; url "
                << link << " - " << err.what() << std::endl;
        }
    }
    (*log_stream) << "Found " << page.child_urls.size() << " unique links" << std::endl;
}

std::string Application::fix_encoding(const std::string& input) {
    return boost::locale::conv::to_utf<char>(input, encoding);
}

std::vector<std::string> Application::get_links(std::string page_content) {
    CDocument document;
    document.parse(std::move(page_content));

    CSelection raw_links = document.find(css_query);

    std::vector<std::string> links;

    for(auto i = 0u; i < raw_links.nodeNum(); i++){
        links.emplace_back(raw_links.nodeAt(i).attribute("href"));
    }
    return links;
}

std::string Application::normalize_url(const std::string& url, const std::string& base_url) {
    std::function<bool(char)> charset = [](char ch){
        const std::string chars {
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz"
            "0123456789"
            "-_.~"
            ":@/?&=#+[]!$()*,;"
        };
        return chars.contains(ch);
    };
    boost::urls::encoding_opts opts;
    opts.space_as_plus = true;
    auto fixed_string = boost::urls::encode(url, charset);
    boost::urls::url parsed_link {fixed_string};
    auto fixed_base_link = boost::urls::encode(base_url, charset);
    boost::urls::url parsed_base_link {fixed_base_link};

    parsed_link.normalize();
    parsed_base_link.normalize();

    parsed_base_link.resolve(parsed_link);

    parsed_base_link.remove_fragment();

    auto ret = parsed_base_link.buffer();
    return ret;
}


decltype(cpr::HeadAsync()) Application::async_head_request(std::string url) {
    return cpr::HeadAsync(cpr::Url{std::move(url)},
                          cpr::Header{{"User-Agent", user_agent}});
}

decltype(cpr::GetAsync()) Application::async_get_request(std::string url) {
    return cpr::GetAsync(cpr::Url{std::move(url)},
                         cpr::Header{{"User-Agent", user_agent}});
}

decltype(cpr::Get()) Application::sync_get_request(std::string url) {
    return cpr::Get(cpr::Url{std::move(url)},
                    cpr::Header{{"User-Agent", user_agent}});
}

decltype(cpr::Head()) Application::sync_head_request(std::string url) {
    return cpr::Head(cpr::Url{std::move(url)},
                     cpr::Header{{"User-Agent", user_agent}});
}

void Application::print_request_log(std::ostream &log_stream, std::ostream &error_stream, Application::request_type req_type, const cpr::Response& response) {
    switch(req_type){
        case request_type::Head:
            log_stream << "HEAD ";
            break;
        case request_type::Get:
            log_stream << "GET ";
            break;
    }
    log_stream << "request to " << response.url << std::endl
            << "status - " << response.status_code << ", elapsed time - " << response.elapsed << std::endl
            << "redirect count - " << response.redirect_count << std::endl;
    if(response.status_code != 200){
        error_stream << "FAILED ";
        switch(req_type){
            case request_type::Head:
                error_stream << "HEAD ";
                break;
            case request_type::Get:
                error_stream << "GET ";
                break;
        }
        error_stream << "request to " << response.url
            << " status code - " << response.status_code << std::endl;
    }
}

bool Application::is_same_domain(const std::string& url1, const std::string& url2) {
    auto parsed_url1 = boost::urls::parse_uri(url1);
    auto parsed_url2 = boost::urls::parse_uri(url2);
    return (parsed_url1->host() == parsed_url2->host());
}

bool Application::is_pdf_file(const std::string& url) {
    auto parsed_url = boost::urls::parse_uri(url);
    return parsed_url->path().ends_with(".pdf");
}
