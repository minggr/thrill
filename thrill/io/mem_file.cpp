/*******************************************************************************
 * thrill/io/mem_file.cpp
 *
 * Copied and modified from STXXL https://github.com/stxxl/stxxl, which is
 * distributed under the Boost Software License, Version 1.0.
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 * Copyright (C) 2013-2014 Timo Bingmann <tb@panthema.net>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <cassert>
#include <cstring>
#include <limits>

#include <thrill/io/iostats.hpp>
#include <thrill/io/mem_file.hpp>

namespace thrill {
namespace io {

void mem_file::serve(void* buffer, offset_type offset, size_type bytes,
                     request::request_type type) {
    std::unique_lock<std::mutex> lock(m_mutex);

    if (type == request::READ)
    {
        stats::scoped_read_timer read_timer(bytes);
        memcpy(buffer, m_ptr + offset, bytes);
    }
    else
    {
        stats::scoped_write_timer write_timer(bytes);
        memcpy(m_ptr + offset, buffer, bytes);
    }
}

const char* mem_file::io_type() const {
    return "memory";
}

mem_file::~mem_file() {
    free(m_ptr);
    m_ptr = nullptr;
}

void mem_file::lock() {
    // nothing to do
}

file::offset_type mem_file::size() {
    return m_size;
}

void mem_file::set_size(offset_type newsize) {
    std::unique_lock<std::mutex> lock(m_mutex);
    assert(newsize <= std::numeric_limits<offset_type>::max());

    m_ptr = (char*)realloc(m_ptr, (size_t)newsize);
    m_size = newsize;
}

void mem_file::discard(offset_type offset, offset_type size) {
    std::unique_lock<std::mutex> lock(m_mutex);
#ifndef STXXL_MEMFILE_DONT_CLEAR_FREED_MEMORY
    // overwrite the freed region with uninitialized memory
    LOG << "discard at " << offset << " len " << size;
    void* uninitialized = malloc(STXXL_BLOCK_ALIGN);
    while (size >= STXXL_BLOCK_ALIGN) {
        memcpy(m_ptr + offset, uninitialized, STXXL_BLOCK_ALIGN);
        offset += STXXL_BLOCK_ALIGN;
        size -= STXXL_BLOCK_ALIGN;
    }
    assert(size <= std::numeric_limits<offset_type>::max());
    if (size > 0)
        memcpy(m_ptr + offset, uninitialized, (size_t)size);
    free(uninitialized);
#else
    common::THRILL_UNUSED(offset);
    common::THRILL_UNUSED(size);
#endif
}

} // namespace io
} // namespace thrill

/******************************************************************************/
