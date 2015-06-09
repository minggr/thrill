/*******************************************************************************
 * examples/wordcount/word_count_simple.cpp
 *
 * Part of Project c7a.
 *
 *
 * This file has no license. Only Chuck Norris can compile it.
 ******************************************************************************/

#include <random>
#include <thread>
#include <string>

#include "word_count_user_program.cpp"

#include <c7a/api/bootstrap.hpp>
#include <c7a/api/dia.hpp>
#include <c7a/common/cmdline_parser.hpp>

int main(int argc, char* argv[]) {

    using c7a::Execute;

    const size_t port_base = 8080;

    c7a::common::CmdlineParser clp;

    clp.SetVerboseProcess(false);

    unsigned int workers = 1;
    clp.AddUInt('n', "workers", "N", workers,
                "Create wordcount example with N workers");

    unsigned int elements = 1;
    clp.AddUInt('s', "elements", "S", elements,
                "Create wordcount example with S generated words");

    if (!clp.Process(argc, argv)) {
        return -1;
    }

    std::vector<std::thread> threads(workers);
    std::vector<char**> arguments(workers);
    std::vector<std::vector<std::string> > strargs(workers);

    for (size_t i = 0; i < workers; i++) {

        arguments[i] = new char*[workers + 3];
        strargs[i].resize(workers + 3);

        for (size_t j = 0; j < workers; j++) {
            strargs[i][j + 3] += "127.0.0.1:";
            strargs[i][j + 3] += std::to_string(port_base + j);
            arguments[i][j + 3] = const_cast<char*>(strargs[i][j + 3].c_str());
        }

        std::function<int(c7a::Context&)> start_func = [elements](c7a::Context& ctx) {
                                                           return word_count_generated(ctx, elements);
                                                       };

        strargs[i][0] = "wordcount";
        arguments[i][0] = const_cast<char*>(strargs[i][0].c_str());
        strargs[i][1] = "-r";
        arguments[i][1] = const_cast<char*>(strargs[i][1].c_str());
        strargs[i][2] = std::to_string(i);
        arguments[i][2] = const_cast<char*>(strargs[i][2].c_str());
        threads[i] = std::thread([=]() { Execute(workers + 3, arguments[i], start_func); });
    }

    for (size_t i = 0; i < workers; i++) {
        threads[i].join();
    }
}

/******************************************************************************/
