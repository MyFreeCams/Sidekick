/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
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


#include <mfc_ipc.h>

#include <boost/algorithm/string.hpp>

namespace MFCIPC
{
//---------------------------------------------------------------------
// isEqualk
//
// don't want to have to include string.hpp because of namespace
// issues.  Keep the include file isolated. 
bool IPCUtil::isEqual(std::string &s, std::string &s2)
{
    boost::algorithm::to_lower(s);
    boost::algorithm::to_lower(s2);
    return s == s2;
}


}

void proxy_blog(int nLevel, const char* pszMsg)
{
}

