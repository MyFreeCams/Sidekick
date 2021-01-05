/*
    This is part of the libb64 project, and has been placed in the public domain.
    For details, see http://sourceforge.net/projects/libb64
*/

#include <string>
#include <stdlib.h>
#include "Compat.h"
#include "fcs_b64.h"

//const int CHARS_PER_LINE = 72;

void base64_init_encodestate(base64_encodestate* state_in)
{
    state_in->step = step_A;
    state_in->result = 0;
    state_in->stepcount = 0;
}

char base64_encode_value(char value_in)
{
    static const char* encoding = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    if (value_in > 63) return '=';
    return encoding[(int)value_in];
}

int base64_encode_block(const char* plaintext_in, int length_in, char* code_out, base64_encodestate* state_in)
{
    const char* plainchar = plaintext_in;
    const char* const plaintextend = plaintext_in + length_in;
    char* codechar = code_out;
    char result;
    char fragment;
    
    result = state_in->result;
    
    switch (state_in->step)
    {
        while (1)
        {
        case step_A:
            if (plainchar == plaintextend)
            {
                state_in->result = result;
                state_in->step = step_A;
                return codechar - code_out;
            }
            fragment =* plainchar++;
            result = (fragment & 0x0fc) >> 2;
            *codechar++ = base64_encode_value(result);
            result = (fragment & 0x003) << 4;
        case step_B:
            if (plainchar == plaintextend)
            {
                state_in->result = result;
                state_in->step = step_B;
                return codechar - code_out;
            }
            fragment =* plainchar++;
            result |= (fragment & 0x0f0) >> 4;
            *codechar++ = base64_encode_value(result);
            result = (fragment & 0x00f) << 2;
        case step_C:
            if (plainchar == plaintextend)
            {
                state_in->result = result;
                state_in->step = step_C;
                return codechar - code_out;
            }
            fragment =* plainchar++;
            result |= (fragment & 0x0c0) >> 6;
            *codechar++ = base64_encode_value(result);
            result  = (fragment & 0x03f) >> 0;
            *codechar++ = base64_encode_value(result);

            /*          
            ++(state_in->stepcount);
            if (state_in->stepcount == CHARS_PER_LINE/4)
            {
                *codechar++ = '\n';
                state_in->stepcount = 0;
            }
            */
        }
    }
    /* control should not reach here */
    return codechar - code_out;
}

int base64_encode_blockend(char* code_out, base64_encodestate* state_in)
{
    char* codechar = code_out;
    
    switch (state_in->step)
    {
    case step_B:
        *codechar++ = base64_encode_value(state_in->result);
        *codechar++ = '=';
        *codechar++ = '=';
        break;
    case step_C:
        *codechar++ = base64_encode_value(state_in->result);
        *codechar++ = '=';
        break;
    case step_A:
        break;
    }
    *codechar++ = '\0';
    
    return codechar - code_out;
}

int base64_decode_value(char value_in)
{
    static const char decoding[] = {62,-1,-1,-1,63,52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-2,-1,-1,-1,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,-1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51};
    static const char decoding_size = sizeof(decoding);
    value_in -= 43;
    if (value_in < 0 || value_in > decoding_size) return -1;
    return decoding[(int)value_in];
}

void base64_init_decodestate(base64_decodestate* state_in)
{
    state_in->step = step_a;
    state_in->plainchar = 0;
}

int base64_decode_block(const char* code_in, const int length_in, char* plaintext_out, base64_decodestate* state_in)
{
    const char* codechar = code_in;
    char* plainchar = plaintext_out;
    char fragment;
    
   * plainchar = state_in->plainchar;
    
    switch (state_in->step)
    {
        while (1)
        {
    case step_a:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_a;
                    state_in->plainchar =* plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
           * plainchar    = (fragment & 0x03f) << 2;
    case step_b:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_b;
                    state_in->plainchar =* plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
           * plainchar++ |= (fragment & 0x030) >> 4;
           * plainchar    = (fragment & 0x00f) << 4;
    case step_c:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_c;
                    state_in->plainchar =* plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
           * plainchar++ |= (fragment & 0x03c) >> 2;
           * plainchar    = (fragment & 0x003) << 6;
    case step_d:
            do {
                if (codechar == code_in+length_in)
                {
                    state_in->step = step_d;
                    state_in->plainchar =* plainchar;
                    return plainchar - plaintext_out;
                }
                fragment = (char)base64_decode_value(*codechar++);
            } while (fragment < 0);
           * plainchar++   |= (fragment & 0x03f);
        }
    }
    /* control should not reach here */
    return plainchar - plaintext_out;
}

namespace base64
{
    size_t encodeData(const BYTE* pIn, size_t nSz, BYTE* pOut)
    {
        base64_encodestate _state;
        size_t nEncSz = 0;

        base64_init_encodestate(&_state);
        nEncSz += (size_t) base64_encode_block( (char*)pIn, (int)nSz, (char*)pOut, &_state);
        nEncSz += (size_t) base64_encode_blockend( (char*)pOut + nEncSz, &_state);

        return nEncSz;
    }

    // Allocate enough memory to encode binary data to b64, return string with base64 encoded contents
    size_t encodeString(const BYTE* pIn, size_t nSz, std::string& sOut)
    {
        // Calculate approximate size needed to store pIn in sOut
        //BYTE chData[((nSz * 4) / 3) + (nSz / 96) + 6];
        size_t nAllocSz = (((nSz * 4) / 3) + (nSz / 96) + 6);
        BYTE* pchData = (BYTE*)malloc(nAllocSz);
        size_t nOutSz;

        sOut.clear();
   
        // Encode to chData, if encoded OK copy to sOut string and return size 
        if ((nOutSz = encodeData(pIn, nSz, pchData)) <= nAllocSz)
            sOut = std::string((const char*)pchData, nOutSz);

        free(pchData);
        return sOut.size();
    }

    size_t decodeData(const BYTE* pIn, size_t nSz, BYTE* pOut)
    {
        base64_decodestate _state;
        base64_init_decodestate(&_state);
        return (size_t) base64_decode_block( (char*)pIn, (int)nSz, (char*)pOut, &_state );
    }

} // namespace base64

