#pragma once

/* Derived from: jsmin.c
2	   2006-05-04
3
4	Copyright (c) 2002 Douglas Crockford  (www.crockford.com)
5
6	Permission is hereby granted, free of charge, to any person obtaining a copy of
7	this software and associated documentation files (the "Software"), to deal in
8	the Software without restriction, including without limitation the rights to
9	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
10	of the Software, and to permit persons to whom the Software is furnished to do
11	so, subject to the following conditions:
12
13	The above copyright notice and this permission notice shall be included in all
14	copies or substantial portions of the Software.
15
16	The Software shall be used for Good, not Evil.
17
18	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
19	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
20	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
21	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
22	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
23	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
24	SOFTWARE.
25	*/

#include <string>

using namespace std;

class Jsmin
{
public:
    Jsmin()
        : theA(0)
        , theB(0)
        , theLookahead(EOF)
        , index_in(0)
        , index_out(0)
        , m_size(0)
        , input_buf(NULL)
        , output_buf(NULL)
    {}

    ~Jsmin()
    {
        if (output_buf)
        {
            free(output_buf);
            output_buf = NULL;
        }
    }

    bool minify(const string& sInput, string& sOutput);

private:
    int    theA;
    int    theB;
    int    theLookahead;
    size_t index_in;
    size_t index_out;
    size_t m_size;

    const char* input_buf;
    char*       output_buf;

    // isAlphanum -- return true if the character is a letter, digit, underscore,
    //  dollar sign, or non-ASCII character
    int isAlphanum(int c)
    {
        return ((c >= 'a'  &&  c <= 'z') || 
                (c >= '0'  &&  c <= '9') || 
                (c >= 'A'  &&  c <= 'Z') || 
                 c == '_'  ||  c == '$'  || 
                 c == '\\' ||  c > 126);
    }

    int get();

    // peek -- get the next character without getting it
    int peek()
    {
        theLookahead = get();
        return theLookahead;
    }

    int next();
    void action(int d);

    int xputc(int c)
    {
        output_buf[index_out++] = static_cast<char>(c);
        return 1;
    }
};
