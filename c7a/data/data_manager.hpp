/*******************************************************************************
 * c7a/data/data_manager.hpp
 *
 * Part of Project c7a.
 *
 *
 * This file has no license. Only Chuck Norris can compile it.
 ******************************************************************************/

#pragma once
#ifndef C7A_DATA_DATA_MANAGER_HEADER
#define C7A_DATA_DATA_MANAGER_HEADER

#include <c7a/data/block_iterator.hpp>
#include <c7a/common/logger.hpp>
#include <c7a/data/input_line_iterator.hpp>

#include <map>
#include <functional>
#include <string>
#include <stdexcept>
#include <memory> //unique_ptr

#include <c7a/net/channel_multiplexer.hpp>
#include "block_iterator.hpp"
#include "block_emitter.hpp"
#include "buffer_chain.hpp"
#include <c7a/common/logger.hpp>
#include <c7a/data/socket_target.hpp>
#include "input_line_iterator.hpp"
#include <c7a/data/buffer_chain_manager.hpp>

namespace c7a {
namespace data {

struct BufferChain;

//! Identification for DIAs
typedef ChainId DIAId;
using c7a::net::ChannelId;
//! Manages all kind of memory for data elements
//!
//!
//! Provides Channel creation for sending / receiving data from other workers.
class DataManager
{
public:
    DataManager(net::ChannelMultiplexer& cmp) : cmp_(cmp) { }

    DataManager(const DataManager&) = delete;
    DataManager& operator = (const DataManager&) = delete;

    //! returns iterator on requested partition.
    //!
    //! Data can be emitted into this partition even after the iterator was created.
    //!
    //! \param id ID of the DIA - determined by AllocateDIA()
    template <class T>
    BlockIterator<T> GetLocalBlocks(DIAId id)
    {
        if (!dias_.Contains(id)) {
            throw std::runtime_error("target dia id unknown.");
        }
        return BlockIterator<T>(*(dias_.Chain(id)));
    }

    //! Returns iterator on the data that was / will be received on the channel
    //!
    //! \param id ID of the channel - determined by AllocateNetworkChannel()
    template <class T>
    BlockIterator<T> GetRemoteBlocks(ChannelId id)
    {
        if (!cmp_.HasDataOn(id)) {
            throw std::runtime_error("target channel id unknown.");
        }

        return BlockIterator<T>(*(cmp_.AccessData(id)));
    }

    //! Returns a number that uniquely addresses a DIA
    //! Calls to this method alter the data managers state.
    //! Calls to this method must be in deterministic order for all workers!
    DIAId AllocateDIA()
    {
        return dias_.AllocateNext();
    }

    //! Returns a number that uniquely addresses a network channel
    //! Calls to this method alter the data managers state.
    //! Calls to this method must be in deterministic order for all workers!
    ChannelId AllocateNetworkChannel()
    {
        return cmp_.AllocateNext();
    }

    //! Returns an emitter that can be used to fill a DIA
    //! Emitters can push data into DIAs even if an intertor was created before.
    //! Data is only visible to the iterator if the emitter was flushed.
    template <class T>
    BlockEmitter<T> GetLocalEmitter(DIAId id)
    {
        if (!dias_.Contains(id)) {
            throw std::runtime_error("target dia id unknown.");
        }
        return BlockEmitter<T>(dias_.Chain(id));
    }

    template <class T>
    std::vector<BlockEmitter<T> > GetNetworkEmitters(ChannelId id)
    {
        if (!cmp_.HasDataOn(id)) {
            throw std::runtime_error("target channel id unknown.");
        }
        return cmp_.OpenChannel<T>(id);
    }

    //!Returns an InputLineIterator with a given input file stream.
    //!
    //! \param file Input file stream
    //!
    //! \return An InputLineIterator for a given file stream
    InputLineIterator GetInputLineIterator(std::ifstream& file)
    {
        //TODO(ts): get those from networks
        size_t my_id = 0;
        size_t num_work = 1;

        return InputLineIterator(file, my_id, num_work);
    }

private:
    static const bool debug = false;
    net::ChannelMultiplexer& cmp_;

    BufferChainManager dias_;
};

} // namespace data
} // namespace c7a

#endif // !C7A_DATA_DATA_MANAGER_HEADER

/******************************************************************************/
