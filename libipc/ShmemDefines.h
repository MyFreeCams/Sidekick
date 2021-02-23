/*
 * Copyright (c) 2013-2020 MFCXY, Inc. <mfcxy@mfcxy.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#ifndef _WIN32
#include <semaphore.h>
#endif
// boost files
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/vector.hpp>
//#include <boost/interprocess/containers/string.hpp>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

#include <boost/algorithm/string.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/posix_time_io.hpp>

namespace MFCIPC
{

// boost typedefs for readability
typedef boost::interprocess::interprocess_mutex													ShmemMutex;
typedef boost::interprocess::interprocess_semaphore                                             ShmemSemaphore;
typedef boost::lock_guard<ShmemMutex>															ShmemLockGuard;

// client record list and allocator
//typedef boost::interprocess::allocator<SHMEM_PROCESS_RECORD, boost::interprocess::managed_shared_memory::segment_manager>	ShmemClientListAllocator;
//typedef boost::interprocess::list<SHMEM_PROCESS_RECORD, ShmemClientListAllocator>							ShmemClientList;

// String allocator
typedef boost::interprocess::allocator<char, boost::interprocess::managed_shared_memory::segment_manager> ShmemCharAllocator;
typedef boost::container::basic_string<char, std::char_traits<char>, ShmemCharAllocator> ShmemString;

boost::interprocess::managed_shared_memory * getShmemManagerInstance(); // defined CShmemManager
}