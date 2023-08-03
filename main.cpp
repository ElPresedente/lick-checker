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

    program.add_argument("--async")
        .help("асинхронная отправка запросов (ddos защита сайта может заблокировать)")
        .default_value(false)
        .implicit_value(true);

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
        program.get<bool>("--async"),
        error_stream,
        log_stream
    };

    bool result = app.run();

    if(auto err_file = program.present("--error-file")){
        delete error_stream;
    }
    if(auto log_file = program.present("--log-file")){
        delete log_stream;
    }

    return result;
}
