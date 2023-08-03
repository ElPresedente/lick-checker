#include <iostream>
#include <fstream>
#include <argparse/argparse.hpp>

#include "Application.h"

int main(int argc, char** argv) {
    argparse::ArgumentParser program{"link-checker", "0.0.1"};

    program.add_argument("url")
        .required()
        .help("проверяемая ссылка")
        .nargs(1);

    program.add_argument("--encoding")
        .default_value(std::string{"utf-8"})
        .help("кодировка сканируемых страниц")
        .nargs(1);

    program.add_argument("--query")
        .default_value(std::string{"a"})
        .help("CSS-селектор для поиска ссылок на странице")
        .nargs(1);

    program.add_argument("--error-file")
        .help("файл вывода ошибок")
        .nargs(1);

    program.add_argument("--log-file")
        .help("при указании выводит логи в файл, а не в консоль")
        .nargs(1);

    program.add_argument("--depth")
        .default_value(3u)
        .help("максимальная глубина поиска ссылок")
        .nargs(1)
        .scan<'i', unsigned int>();

    program.add_argument("--user-agent")
        .default_value("Mozilla/5.0")
        .help("использовать нестандартный user-agent")
        .nargs(1);

    try{
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err){
        std::cerr << err.what() << std::endl;
        std::cerr << program << std::endl;
        std::exit(1);
    }

    std::cout << program.get<std::string>("--encoding") << std::endl
        << program.get<std::string>("--query") << std::endl;

    std::ostream* error_stream, *log_stream;

    if(auto err_file = program.present("--error-file")){
        error_stream = new std::ofstream{*err_file};
    }
    else{
        error_stream = &std::cerr;
    }
    if(auto log_file = program.present("--log-file")){
        log_stream = new std::ofstream{*log_file};
    }
    else{
        log_stream = &std::cout;
    }
    Application app{
        program.get<std::string>("--encoding"),
        program.get<std::string>("url"),
        program.get<std::string>("--query"),
        program.get<std::string>("--user-agent"),
        program.get<unsigned int>("--depth"),
        error_stream,
        log_stream
    };


    return app.run();
//    std::ofstream error{"report.txt"};
//    link_checker_cleanup cleanup{2, "https://oreluniver.ru", "windows-1251", ".page-content a", true};
//    cleanup.set_error_stream(&error);
//    cleanup.set_user_agent("Mozilla/5.0");
//
//    link_checker checker{"https://oreluniver.ru/sveden"};
//    checker.parse();
//    return 0;
}
