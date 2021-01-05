#basic definitions
set(HEADER_FILE "${HEADER_DIR}/build_number.h")
set(CACHE_FILE "${HEADER_DIR}/BuildNumberCache.txt")

#Reading data from file + incrementation
IF(EXISTS ${CACHE_FILE})
    file(READ ${CACHE_FILE} INCREMENTED_VALUE)
    math(EXPR INCREMENTED_VALUE "${INCREMENTED_VALUE}+1")
ELSE()
    set(INCREMENTED_VALUE "1")
ENDIF()

#Update the cache
file(WRITE ${CACHE_FILE} "${INCREMENTED_VALUE}")

#Create the header
file(WRITE ${HEADER_FILE} "#pragma once\n\n#define MFC_BUILD_NUMBER ${INCREMENTED_VALUE}\n\n")
