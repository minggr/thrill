/*******************************************************************************
 * thrill/io/serving_request.cpp
 *
 * Copied and modified from STXXL https://github.com/stxxl/stxxl, which is
 * distributed under the Boost Software License, Version 1.0.
 *
 * Part of Project Thrill - http://project-thrill.org
 *
 * Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 * Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 * All rights reserved. Published under the BSD-2 license in the LICENSE file.
 ******************************************************************************/

#include <thrill/common/state.hpp>
#include <thrill/io/file.hpp>
#include <thrill/io/request_interface.hpp>
#include <thrill/io/request_with_state.hpp>
#include <thrill/io/serving_request.hpp>

#include <iomanip>

namespace thrill {
namespace io {

serving_request::serving_request(
    const completion_handler& on_cmpl,
    file* f,
    void* buf,
    offset_type off,
    size_type b,
    request_type t)
    : request_with_state(on_cmpl, f, buf, off, b, t) {
#ifdef STXXL_CHECK_BLOCK_ALIGNING
    // Direct I/O requires file system block size alignment for file offsets,
    // memory buffer addresses, and transfer(buffer) size must be multiple
    // of the file system block size
    check_alignment();
#endif
}

void serving_request::serve() {
    check_nref();
    LOG << "serving_request::serve(): "
        << m_buffer << " @ ["
        << m_file << "|" << m_file->get_allocator_id() << "]0x"
        << std::hex << std::setfill('0') << std::setw(8)
        << m_offset << "/0x" << m_bytes
        << ((m_type == request::READ) ? " READ" : " WRITE");

    try
    {
        m_file->serve(m_buffer, m_offset, m_bytes, m_type);
    }
    catch (const io_error& ex)
    {
        error_occured(ex.what());
    }

    check_nref(true);

    completed(false);
}

const char* serving_request::io_type() const {
    return m_file->io_type();
}

} // namespace io
} // namespace thrill

/******************************************************************************/
