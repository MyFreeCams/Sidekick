/* jsmin.c
   2011-01-22

Copyright (c) 2002 Douglas Crockford  (www.crockford.com)

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

The Software shall be used for Good, not Evil.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsmin.h"

/* get -- return the next character from stdin. Watch out for lookahead. If
        the character is a control character, translate it to a space or
        linefeed.
*/

int Jsmin::get()
{
    int c = theLookahead;
    theLookahead = EOF;
    if ( index_in >= m_size ) {
        return EOF;
    }
    if (c == EOF) {
        c = input_buf[index_in++];
    }
    if (c >= ' ' || c == '\n' || c == EOF) {
        return c;
    }
    if (c == '\r') {
        return '\n';
    }
    return ' ';
}

/* next -- get the next character, excluding comments. peek() is used to see
        if a '/' is followed by a '/' or '*'.
*/
int Jsmin::next()
{
    int c = get();
    if  (c == '/') {
        switch (peek()) {
        case '/':
            for (;;) {
                c = get();
                if (c <= '\n') {
                    return c;
                }
            }
        case '*':
            get();
            for (;;) {
                switch (get()) {
                case '*':
                    if (peek() == '/') {
                        get();
                        return ' ';
                    }
                    break;
                case EOF:
                    throw("!Unterminated comment");
                }
            }
        default:
            return c;
        }
    }
    return c;
}


/* action -- do something! What you do is determined by the argument:
        1   Output A. Copy B to A. Get the next B.
        2   Copy B to A. Get the next B. (Delete A).
        3   Get the next B. (Delete B).
   action treats a string as a single character. Wow!
   action recognizes a regular expression if it is preceded by ( or , or =.
*/

void Jsmin::action(int d)
{
    switch (d) {
    case 1:
        xputc(theA);
    case 2:
        theA = theB;
        if (theA == '\'' || theA == '"') {
            for (;;) {
                xputc(theA);
                theA = get();
                if (theA == theB) {
                    break;
                }
                if (theA == '\\') {
                    xputc(theA);
                    theA = get();
                }
                if (theA == EOF) {
                    throw("!Unterminated string literal");
                }
            }
        }
    case 3:
        theB = next();
        if (theB == '/' && (theA == '(' || theA == ',' || theA == '=' ||
                            theA == ':' || theA == '[' || theA == '!' ||
                            theA == '&' || theA == '|' || theA == '?' ||
                            theA == '{' || theA == '}' || theA == ';' ||
                            theA == '\n')) {
            xputc(theA);
            xputc(theB);
            for (;;) {
                theA = get();
                if (theA == '[') {
                    for (;;) {
                        xputc(theA);
                        theA = get();
                        if (theA == ']') {
                            break;
                        }
                        if (theA == '\\') {
                            xputc(theA);
                            theA = get();
                        }
                        if (theA == EOF) {
                            throw("!Unterminated set in Regular Expression literal");
                        }
                    }
                } else if (theA == '/') {
                    break;
                } else if (theA =='\\') {
                    xputc(theA);
                    theA = get();
                }
                if (theA == EOF) {
                    throw("!Unterminated Regular Expression literal");
                }
                xputc(theA);
            }
            theB = next();
        }
    }
}


/* minify -- Copy the input to the output, deleting the characters which are
        insignificant to JavaScript. Comments will be removed. Tabs will be
        replaced with spaces. Carriage returns will be replaced with linefeeds.
        Most spaces and linefeeds will be removed.
*/

//char* Jsmin::minify(const char *original)
bool Jsmin::minify(const string& sInput, string& sOutput)
{
    bool retVal = false;
    input_buf = sInput.c_str();
    index_in = 0;
    index_out = 0;

    if (output_buf != NULL)
    {
      free(output_buf);
      output_buf = NULL;
    }

    m_size = sInput.size();
    output_buf = (char *)malloc(sizeof(char) * (m_size+1));

    try
    {
        //theA = ';';
        theA = get();
        action(3);
        while (theA != EOF) {
            switch (theA) {
            case ' ':
                if (isAlphanum(theB)) {
                    action(1);
                } else {
                    action(2);
                }
                break;
            case '\n':
                switch (theB) {
                case '{':
                case '[':
                case '(':
                case '+':
                case '-':
                    action(1);
                    break;
                case ' ':
                    action(3);
                    break;
                default:
                    if (isAlphanum(theB)) {
                        action(1);
                    } else {
                        action(2);
                    }
                }
                break;
            default:
                switch (theB) {
                case ' ':
                    if (isAlphanum(theA)) {
                        action(1);
                        break;
                    }
                    action(3);
                    break;
                case '\n':
                    switch (theA) {
                    case '}':
                    case ']':
                    case ')':
                    case '+':
                    case '-':
                    case '"':
                    case '\'':
                        action(1);
                        break;
                    default:
                        if (isAlphanum(theA)) {
                            action(1);
                        } else {
                            action(3);
                        }
                    }
                    break;
                default:
                    action(1);
                    break;
                }
            }
        }
    
    
        output_buf[index_out] = 0;
        sOutput = output_buf;
        free(output_buf);
        output_buf = NULL;
        retVal = true;
    }
    catch (char const* pszErr)
    {
        sOutput = pszErr;
        retVal = false;        
    }

    return retVal;
}

/*
extern "C" {
  char* minify(const char *in)
  {
    char *out;
    Jsmin m;
    try {
      out = strdup(m.minify(in));
    }
    catch (char const *e) {
      out = strdup(e);
    }
    return out;
  }
}*/
