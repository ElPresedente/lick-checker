//
// Created by rolark on 03.07.23.
//

#include "link_checker.h"

link_checker::link_checker(const std::string& url, uint32_t depth) :
    url(url),
    depth(depth)
{}

std::ostream* link_checker::error_stream = &std::cerr;
std::ostream* link_checker::log_stream = &std::cout;

std::unordered_set<std::string>* link_checker::checked_links = nullptr;

uint32_t link_checker::max_depth = 0;

std::string link_checker::encoding = "utf-8";
std::string link_checker::links_selector = "";
std::string link_checker::base_url = "";
std::string link_checker::user_agent = "curl/" LIBCURL_VERSION;

bool link_checker::normalize_url = true;

void link_checker::parse() {
    if(!is_checkable())
        return;
    // request part
    auto response = cpr::Get(cpr::Url{url},
                             cpr::Header{{"User-Agent", user_agent}});
    if(response.status_code == 0) {
        (*error_stream) << "GET:" << url << " - " << response.error.message << std::endl;
        (*log_stream) << "GET:" << url << " - " << response.error.message << std::endl;
    }
    else if(response.status_code >= 400){
        (*error_stream) << "GET:" << url << " - [" << response.status_code << "] " << response.error.message << std::endl;
        (*log_stream) << "GET:" << url << " - [" << response.status_code << "] " << response.error.message << std::endl;
    }
    else {
        (*log_stream) << url << " - completed by " << response.elapsed
            << ", code " << response.status_code
            << ", depth " << depth
            << ", redirect count " << response.redirect_count << std::endl;
    }
    // change encoding
    std::string document_text = boost::locale::conv::to_utf<char>(response.text, encoding);

    CDocument document;
    document.parse(document_text);

    CSelection links = document.find(links_selector);
    (*log_stream) << links.nodeNum() << " links found" << std::endl;

    // make multi request
    cpr::MultiPerform multi_request;

    // send HEAD requests to check pages
    std::vector<std::shared_ptr<cpr::Session>> urls;
    for(auto i = 0u, links_count = links.nodeNum(); i < links_count; i++){
        auto&& session =  urls.emplace_back(std::make_shared<cpr::Session>());
        std::string raw_link = links.nodeAt(i).attribute("href");
        std::string normalized_link = resolve_and_normalize_url(raw_link);
        session->SetUrl(normalized_link);
        session->SetHeader(cpr::Header{{"User-Agent", user_agent}});
        multi_request.AddSession(session, cpr::MultiPerform::HttpMethod::HEAD_REQUEST);
    }
    auto head_response = multi_request.Head();
    for(auto&& resp : head_response){
        if(resp.status_code == 0) {
            (*error_stream) << "HEAD:" << url << " - " << response.error.message << std::endl;
            (*log_stream) << "HEAD:" << url << " - " << response.error.message << std::endl;
        }
        else if(resp.status_code >= 400){
            (*error_stream) << "HEAD:" << url << " - [" << response.status_code << "] " << response.error.message << std::endl;
            (*log_stream) << "HEAD:" << url << " - [" << response.status_code << "] " << response.error.message << std::endl;
        }
        else{
            link_checker checker{resp.url.str(), depth + 1};
            checker.parse();
        }
    }

}

std::string link_checker::resolve_and_normalize_url(const std::string& query) const {
    std::string result_url;
    UriUriA base, rel, result;
    const char* error;
    if (uriParseSingleUriA(&base, url.c_str(), &error) != URI_SUCCESS)
        return std::string{};

    if (uriParseSingleUriA(&rel, query.c_str(), &error) != URI_SUCCESS)
        return std::string{};

    if (uriAddBaseUriA(&result, &rel, &base) != URI_SUCCESS)
        return std::string{};

    if(normalize_url)
        if(uriNormalizeSyntaxA(&result) != URI_SUCCESS)
            return std::string{};

    int chars_required;
    if (uriToStringCharsRequiredA(&result, &chars_required) != URI_SUCCESS)
        return std::string{};

    chars_required++;
    std::unique_ptr<char[]> result_str = std::make_unique<char[]>(chars_required);
    int bytes_written;
    if (uriToStringA(result_str.get(), &result, chars_required, &bytes_written) != URI_SUCCESS)
        return std::string{};

    return std::string{result_str.get()};
}

bool link_checker::is_checkable() const{
    if(checked_links == nullptr){
        std::cerr << "Declare link_checker_cleanup before using link_checker!" << std::endl;
        return false;
    }
    if(base_url != "" && url.find(base_url) != 0)
        return false;
    if(depth >= max_depth){
        return false;
    }
    if(link_checker::checked_links->find(url) == link_checker::checked_links->end()){
        link_checker::checked_links->insert(url);
    }
    else{
        (*log_stream) << url << " - repeated, skip" << std::endl;
        return false;
    }
    if(url.find_last_of(".pdf") == url.length() - 1){
        // skip loading pdf docs
        (*log_stream) << url << " - skip pdf" << std::endl;
        return false;
    }
    return true;
}
