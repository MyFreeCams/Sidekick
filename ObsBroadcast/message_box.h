#pragma once

#ifndef _WIN32
#define IDOK        1
#define IDCANCEL    2
#define MB_OK       0
#endif


int message_box( const char* header, const char* message, unsigned long message_type = 0 );
