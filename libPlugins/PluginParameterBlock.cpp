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

// System Includes
#include <string>
#include <iostream>
#include <fstream>

#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/config-file.h>

// mfc includes
#include "libfcs/fcslib_string.h"
#include "libfcs/Log.h"
#include "libfcs/MfcJson.h"

// solution includes
#include "MFCConfigConstants.h"
#include  <libPlugins/ObsServicesJson.h>

// project includes
#include "ObsUtil.h"
#include "PluginParameterBlock.h"

using std::string;

