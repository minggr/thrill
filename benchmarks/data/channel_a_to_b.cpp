/*******************************************************************************
 * benchmarks/data/channel_a_to_b.cpp
 *
 * Part of Project c7a.
 *
 * Copyright (C) 2015 Tobias Sturm <mail@tobiassturm.de>
 *
 * This file has no license. Only Chuck Norris can compile it.
 ******************************************************************************/

#include <c7a/api/context.hpp>
#include <c7a/common/cmdline_parser.hpp>
#include <c7a/common/logger.hpp>
#include <c7a/common/thread_pool.hpp>
#include <c7a/core/job_manager.hpp>
#include <c7a/data/manager.hpp>

#include "data_generators.hpp"

#include <iostream>
#include <random>
#include <string>

using namespace c7a; // NOLINT

//! Creates two threads that work with two context instances
//! one worker sends elements to the other worker.
//! Number of elements depends on the number of bytes.
//! one RESULT line will be printed for each iteration
//! All iterations use the same generated data.
//! Variable-length elements range between 1 and 100 bytes
template <typename Type>
void ConductExperiment(uint64_t bytes, int iterations, api::Context& ctx1, api::Context& ctx2, const std::string& type_as_string) {
    using namespace c7a::common;

    auto data = generate<Type>(bytes, 1, 100);
    ThreadPool pool;
    for (int i = 0; i < iterations; i++) {
        StatsTimer<true> write_timer;
        pool.Enqueue([&data, &ctx1, &write_timer]() {
                         auto channel = ctx1.data_manager().GetNewChannel();
                         auto writers = channel->OpenWriters();
                         assert(writers.size() == 2);
                         write_timer.Start();
                         auto& writer = writers[1];
                         for (auto& s : data) {
                             writer(s);
                         }
                         writer.Close();
                         writers[0].Close();
                         write_timer.Stop();
                     });

        StatsTimer<true> read_timer;
        pool.Enqueue([&ctx2, &read_timer]() {
                         auto channel = ctx2.data_manager().GetNewChannel();
                         auto readers = channel->OpenReaders();
                         assert(readers.size() == 2);
                         auto& reader = readers[0];
                         read_timer.Start();
                         while (reader.HasNext()) {
                             reader.Next<Type>();
                         }
                         read_timer.Stop();
                     });
        pool.LoopUntilEmpty();
        std::cout << "RESULT"
                  << " datatype=" << type_as_string
                  << " size=" << bytes
                  << " write_time=" << write_timer
                  << " read_time=" << read_timer
                  << std::endl;
    }
}

int main(int argc, const char** argv) {
    core::JobManager jobMan1, jobMan2;
    c7a::common::ThreadPool connect_pool;
    std::vector<std::string> endpoints;
    endpoints.push_back("127.0.0.1:8000");
    endpoints.push_back("127.0.0.1:8001");
    connect_pool.Enqueue([&jobMan1, &endpoints]() {
                             jobMan1.Connect(0, net::Endpoint::ParseEndpointList(endpoints), 1);
                         });

    connect_pool.Enqueue([&jobMan2, &endpoints]() {
                             jobMan2.Connect(1, net::Endpoint::ParseEndpointList(endpoints), 1);
                         });
    connect_pool.LoopUntilEmpty();

    api::Context ctx1(jobMan1, 0);
    api::Context ctx2(jobMan2, 0);
    common::NameThisThread("benchmark");

    common::CmdlineParser clp;
    clp.SetDescription("c7a::data benchmark for disk I/O");
    clp.SetAuthor("Tobias Sturm <mail@tobiassturm.de>");
    int iterations;
    uint64_t bytes;
    std::string type;
    clp.AddBytes('b', "bytes", bytes, "number of bytes to process");
    clp.AddParamInt("n", iterations, "Iterations");
    clp.AddParamString("type", type,
                       "data type (int, string, pair, triple)");
    if (!clp.Process(argc, argv)) return -1;

    if (type == "int")
        ConductExperiment<int>(bytes, iterations, ctx1, ctx2, type);
    if (type == "string")
        ConductExperiment<std::string>(bytes, iterations, ctx1, ctx2, type);
    if (type == "pair")
        ConductExperiment<std::pair<std::string, int> >(bytes, iterations, ctx1, ctx2, type);
    if (type == "triple")
        ConductExperiment<std::tuple<std::string, int, std::string> >(bytes, iterations, ctx1, ctx2, type);
}

/******************************************************************************/