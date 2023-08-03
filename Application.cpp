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
        (*log_stream) << "checking child links"
        if(async_mode){
            std::vector<cpr::AsyncResponse> head_responses;

            for(auto&& link : current_page.child_urls){
                head_responses.emplace_back(async_head_request(link));
            }
            for(auto&& resp_promise : head_responses){
                resp_promise.wait();
                auto response = resp_promise.get();
                print_request_log((*log_stream), (*error_stream), request_type::Head, response);
                if(response.status_code == 200 ){
                    const std::string& current_url = response.url.str();
                    if(is_same_domain(current_page.url, current_url)
                       && !is_pdf_file(current_url))
                    {
                        process_queue.emplace(response.url.str(), current_page.depth + 1);
                    }
                }
            }
        }
        else{
            for(auto&& link : current_page.child_urls){
                auto response = sync_head_request(link);
                print_request_log((*log_stream), (*error_stream), request_type::Head, response);
                if(response.status_code == 200 ){
                    const std::string& current_url = response.url.str();
                    if(is_same_domain(current_page.url, current_url)
                       && !is_pdf_file(current_url))
                    {
                        process_queue.emplace(response.url.str(), current_page.depth + 1);
                    }
                }
            }
        }
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
        page.child_urls.insert(normalize_url(link, page.url));
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

std::string Application::normalize_url(const std::string& link, const std::string& base_url) {
    UriUriA url, base, resolved_url;
    const char* error;

    if(uriParseSingleUriA(&url, link.c_str(), &error) != URI_SUCCESS)
        throw std::runtime_error("normalize_url: error while parsing link");
    if(uriParseSingleUriA(&base, base_url.c_str(), &error)) {
        uriFreeUriMembersA(&url);
        throw std::runtime_error("normalize_url: error while parsing base link");
    }

    //переходим от относительной ссылки к абсолютной
    if(uriAddBaseUriA(&resolved_url, &url, &base) != URI_SUCCESS) {
        uriFreeUriMembersA(&url);
        uriFreeUriMembersA(&base);
        throw std::runtime_error("normalize_url: error while resolving relative link");
    }
    //нормализация ссылки
    if(uriNormalizeSyntaxA(&resolved_url) != URI_SUCCESS){
        uriFreeUriMembersA(&url);
        uriFreeUriMembersA(&base);
        uriFreeUriMembersA(&resolved_url);
        throw std::runtime_error("normalize_url: error while normalization");
    }
    //удаляем фрагмент
    //библа не умеет в это, так что будем делать некрасиво
    const char* first_fragment_ptr = resolved_url.fragment.first;
    const char* afterLast_fragment_ptr = resolved_url.fragment.afterLast;

    char** nonconst_first = const_cast<char**>(&resolved_url.fragment.first);
    char** nonconst_afterLast = const_cast<char**>(&resolved_url.fragment.afterLast);

    *nonconst_first = nullptr;
    *nonconst_afterLast = nullptr;

    int chars_required;
    if (uriToStringCharsRequiredA(&resolved_url, &chars_required) != URI_SUCCESS){
        *nonconst_first = const_cast<char*>(first_fragment_ptr);
        *nonconst_afterLast = const_cast<char*>(afterLast_fragment_ptr);
        uriFreeUriMembersA(&url);
        uriFreeUriMembersA(&base);
        uriFreeUriMembersA(&resolved_url);
        throw std::runtime_error("normalize_url: error while building url string");
    }
    chars_required++;

    std::unique_ptr<char[]> result_str = std::make_unique<char[]>(chars_required);
    int bytes_written;
    if (uriToStringA(result_str.get(), &resolved_url, chars_required, &bytes_written) != URI_SUCCESS){
        *nonconst_first = const_cast<char*>(first_fragment_ptr);
        *nonconst_afterLast = const_cast<char*>(afterLast_fragment_ptr);
        uriFreeUriMembersA(&url);
        uriFreeUriMembersA(&base);
        uriFreeUriMembersA(&resolved_url);
        throw std::runtime_error("normalize_url: error while building url string");
    }


    //возвращаем указатели на фрагмент для нормальной очистки памяти
    *nonconst_first = const_cast<char*>(first_fragment_ptr);
    *nonconst_afterLast = const_cast<char*>(afterLast_fragment_ptr);
    uriFreeUriMembersA(&url);
    uriFreeUriMembersA(&base);
    uriFreeUriMembersA(&resolved_url);
    return std::string{result_str.get()};
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
        error_stream << "FAILED";
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
    UriUriA uri1, uri2;
    const char* error;
    if(uriParseSingleUriA(&uri1, url1.c_str(), &error) != URI_SUCCESS){
        throw std::runtime_error("is_same_domain: url1 parsing error");
    }
    if(uriParseSingleUriA(&uri2, url2.c_str(), &error) != URI_SUCCESS){
        uriFreeUriMembersA(&uri1);
        throw std::runtime_error("is_same_domain: url2 parsing error");
    }

    bool result = true;
    size_t url1_domain_length = uri1.hostText.afterLast - uri1.hostText.first;
    size_t url2_domain_length = uri2.hostText.afterLast - uri2.hostText.first;

    if(url1_domain_length != url2_domain_length){
        result = false;
    }
    else{
        result = (memcmp(uri1.hostText.first,
                        uri2.hostText.first,
                        std::min(url1_domain_length, url2_domain_length)) == 0);
    }

    uriFreeUriMembersA(&uri1);
    uriFreeUriMembersA(&uri2);
    return result;
}

bool Application::is_pdf_file(const std::string& url) {
    UriUriA uri;
    const char* error;
    if(uriParseSingleUriA(&uri, url.c_str(), &error) != URI_SUCCESS){
        throw std::runtime_error("is_pdf_file: error while parsing url");
    }
    bool result = false;
    size_t path_tail_length = uri.pathTail->text.afterLast - uri.pathTail->text.first;
    if(path_tail_length > 4){
        std::unique_ptr<char[]> test_str = std::make_unique<char[]>(5);
        std::strncpy(test_str.get(), uri.pathTail->text.afterLast - 4, 4);
        result = (std::strcmp(test_str.get(), ".pdf") == 0);
    }
    uriFreeUriMembersA(&uri);
    return result;
}

