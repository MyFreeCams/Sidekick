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
#pragma once

#include "Compat.h"

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <map>
#include <set>
#include <unordered_set>
#include <stack>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

#include "JSON_parser.h"
#include "fcslib_string.h"
#include "jsmin.h"

// The JSON code we make use of is more granular, so we'll pick some of its low level
// defines for use in our own data structures but renamed to make more sense

#ifndef JSON_T_OBJECT
#define JSON_T_OBJECT   JSON_T_OBJECT_BEGIN
#endif
#ifndef JSON_T_ARRAY
#define JSON_T_ARRAY    JSON_T_ARRAY_BEGIN
#endif
#ifndef JSON_T_BOOLEAN
#define JSON_T_BOOLEAN  JSON_T_TRUE
#endif

class MfcJsonObj;

// used to track state information about where in a json tree we are during decoding
typedef stack< MfcJsonObj* > MfcJsonStack;

// used for walking through m_mObj to enum all nodes in a json object container (only for JSON_T_OBJECT types)
// a const interator used, client should not attempt editing tree, for reading data only
typedef map< string,MfcJsonObj* >::const_iterator MfcJsonIter;

typedef MfcJsonObj* MfcJsonPtr;

class MfcJsonObj
{
public:
    static const int JSOPT_RAW      = -2;
    static const int JSOPT_NORMAL   = -1;
    static const int JSOPT_PRETTY   =  0;

    static const char* sm_pszHexVals;               // "0123456789ABCDEF"

    static const char* MapJsonType(uint32_t dwType)
    {
        static char s_szType[32];

        switch (dwType)
        {
            case JSON_T_NULL:        return "JSON_T_NULL";
            case JSON_T_STRING:      return "JSON_T_STRING";
            case JSON_T_BOOLEAN:     return "JSON_T_BOOLEAN";
            case JSON_T_FLOAT:       return "JSON_T_FLOAT";
            case JSON_T_INTEGER:     return "JSON_T_INTEGER";
            case JSON_T_OBJECT:      return "JSON_T_OBJECT";
            case JSON_T_ARRAY:       return "JSON_T_ARRAY";
        }

        snprintf(s_szType, sizeof(s_szType), "JSON_T_unknown_%u", dwType);
        return s_szType;
    }

    MfcJsonObj();

    MfcJsonObj(const MfcJsonObj& src);              // Initializes an object copied from src
    MfcJsonObj(JSON_type nType);                    // Initializes an emtpy/null object of nType (defaults type to 0/false/""/empty container)

    MfcJsonObj(int64_t nVal);                       // Initializes an integer val
    MfcJsonObj(double dVal);                        // Initializes a double precision floating point val
    MfcJsonObj(bool fVal);                          // Initializes a boolean val
    MfcJsonObj(const string& sVal);                 // Initializes a string val
    MfcJsonObj(const char* pszVal);                 // Initializes a psz string val

    // swap & detach methods experimental - test before production use
#ifdef _MFCDEV_
    void swap(MfcJsonObj& js);                      // Swap contents with another instance
    MfcJsonObj* detach(const string& sKey);         // Detach value under sKey from this object
    MfcJsonObj* detach(size_t nPos);                // Detach value under position nPos from this array
#endif

    const MfcJsonObj& operator=(const MfcJsonObj& src);
    const MfcJsonObj& operator=(uint64_t nVal)    { setInt(nVal);     return *this; }
    const MfcJsonObj& operator=(int64_t nVal)     { setInt(nVal);     return *this; }
    const MfcJsonObj& operator=(uint32_t dwVal)   { setInt(dwVal);    return *this; }
    const MfcJsonObj& operator=(uint16_t wVal)    { setInt(wVal);     return *this; }
    const MfcJsonObj& operator=(int nVal)         { setInt(nVal);     return *this; }
    const MfcJsonObj& operator=(string sVal)      { setString(sVal);  return *this; }
    const MfcJsonObj& operator=(double dVal)      { setFloat(dVal);   return *this; }
    const MfcJsonObj& operator=(bool fVal)        { setBoolean(fVal); return *this; }

    virtual ~MfcJsonObj()
    {
        clear();
        free(m_pszFloatPrecisionFmt);               // stores optional override for floating point format precision
        m_pszFloatPrecisionFmt = NULL;
    }

    static MfcJsonObj* newType(JSON_type jsType)
    {
        return new MfcJsonObj(jsType);
    }

    void clear(void);                               // frees all memory associated with obj, all children, sets to null type

    void clearArray(void)                           // clears and sets to empty array
    {
        clear();
        _makeType(JSON_T_ARRAY);
    }

    void clearObject(void)                          // clears and sets to empty object
    {
        clear();
        _makeType(JSON_T_OBJECT);
    }


    bool Deserialize(const uint8_t* pchData, size_t nLen); // Decode byte stream with JSON_parser into this object with json lib code
    bool Deserialize(const string& sData)               // Decode string with JSON_parser into this object with json lib code
    {
        return Deserialize( (const uint8_t*)sData.c_str(), sData.size() );
    }

    bool loadFromFile(const string& sFilename)
    {
        string sConfig, sMin;
        bool retVal = false;
        Jsmin jsmin;

        if (stdGetFileContents(sFilename, sConfig))     // Read text from file on disk into sConfig
            if (jsmin.minify(sConfig, sMin))            // We minify the code to get rid of comments.
                retVal = Deserialize(sMin);             // Return true if deserialize succeeds

        return retVal;
    }

    bool objectHas(const string& sKey) const
    {
        if (isObject())
            if (m_mObj.find(sKey) != m_mObj.end())
                return true;

        return false;
    }

    bool isNull(void) const                 { return m_dwType == JSON_T_NULL;                       }
    bool isInt(void) const                  { return m_dwType == JSON_T_INTEGER;                    }
    bool isFloat(void) const                { return m_dwType == JSON_T_FLOAT;                      }
    bool isArray(void) const                { return m_dwType == JSON_T_ARRAY;                      }
    bool isObject(void) const               { return m_dwType == JSON_T_OBJECT;                     }
    bool isString(void) const               { return m_dwType == JSON_T_STRING;                     }
    bool isBoolean(void) const              { return m_dwType == JSON_T_BOOLEAN;                    }

    void setInt(uint64_t nVal)              { _makeType(JSON_T_INTEGER); m_nVal = (int64_t)nVal;    }
    void setInt(int64_t nVal)               { _makeType(JSON_T_INTEGER); m_nVal = nVal;             }
    void setInt(uint32_t dwVal)             { _makeType(JSON_T_INTEGER); m_nVal = (int64_t)dwVal;   }
    void setInt(uint16_t wVal)              { _makeType(JSON_T_INTEGER); m_nVal = (int64_t)wVal;    }
    void setInt(int nVal)                   { _makeType(JSON_T_INTEGER); m_nVal = (int64_t)nVal;    }

    void setString(const string& sVal)      { _makeType(JSON_T_STRING);  m_sVal = sVal;             }
    void setBoolean(bool fVal)              { _makeType(JSON_T_BOOLEAN); m_fVal = fVal;             }
    void setFloat(double dVal)              { _makeType(JSON_T_FLOAT);   m_dVal = dVal;             }
    void setNull(void)                      { _makeType(JSON_T_NULL);                               }

    size_t arrayLen(void) const             { return (isArray() ? m_vArray.size() : 0);             }
    size_t objectLen(void) const            { return (isObject() ? m_mObj.size() : 0);              }

    // Clears existing data if not JSON_T_ARRAY, adds a value to container m_vArray
    void arrayAdd(int64_t nVal);
    void arrayAdd(double dVal);
    void arrayAdd(bool fVal);
    void arrayAdd(const string& sVal);
    void arrayAdd(const char* pszVal) { arrayAdd(string(pszVal)); }

    void arrayAdd(MfcJsonObj* pObj);
    void arrayAdd(const MfcJsonObj& jsVal);
    void arrayAdd(int32_t nVal) { arrayAdd((int64_t)nVal); }
    void arrayAdd(uint32_t nVal) { arrayAdd((int64_t)nVal); }

#ifndef _WIN32
    // on win32, size_t will be the same as uint32_t most likely
    void arrayAdd(size_t nVal) { arrayAdd((int64_t)nVal); }
#endif

    // Clears existing data if not JSON_T_OBJECT, then adds a key:value pair to container m_mObj, deleting old value first,
    // if found and fReplace == true.  the MfcJsonObj* version of objectAdd() assumes it now OWNS pObj, so it will delete
    // pObj when it is done using it.  Do not pass in objects that arent created with new operator or referenced elsewhere.
    // Return true if add was successful (this is used when fReplace =  false to detect duplicate keys).
    bool objectAdd(const string& sKey, int64_t nVal, bool fReplace = true);
    bool objectAdd(const string& sKey, double dVal, bool fReplace = true);
    bool objectAdd(const string& sKey, bool fVal, bool fReplace = true);
    bool objectAdd(const string& sKey, const string& sVal, bool fReplace = true);
    bool objectAdd(const string& sKey, const char* pszVal, bool fReplace = true)
    {
        return objectAdd(sKey, string( pszVal ), fReplace);
    }

    bool objectAdd(const string& sKey, MfcJsonObj* pObj, bool fReplace = true);
    bool objectAdd(const string& sKey, const MfcJsonObj& json, bool fReplace = true);
#ifndef _WIN32
    bool objectAdd(const string& sKey, time_t   nVal, bool fReplace = true) { return objectAdd(sKey, (int64_t)nVal, fReplace); }
#endif
    bool objectAdd(const string& sKey, int32_t  nVal, bool fReplace = true) { return objectAdd(sKey, (int64_t)nVal, fReplace); }
    bool objectAdd(const string& sKey, uint32_t nVal, bool fReplace = true) { return objectAdd(sKey, (int64_t)nVal, fReplace); }
    bool objectAdd(const string& sKey, uint64_t nVal, bool fReplace = true) { return objectAdd(sKey, (int64_t)nVal, fReplace); }

#ifdef _WIN32
    bool objectAdd(const string& sKey, unsigned long nVal, bool fReplace = true) { return objectAdd(sKey, (int64_t)nVal, fReplace); }
#endif

    void objectRemove(const string& sKey);

    // Sets val argument to typed values from m_mObj, if we are an object.
    // returns true if key found and value set, otherwise false


    bool objectGetInt(const string& sKey, int32_t& nVal) const;
    bool objectGetInt(const string& sKey, int64_t& nVal) const;
    bool objectGetBool(const string& sKey, bool& fVal) const;
    bool objectGetFloat(const string& sKey, double& dVal) const;
    bool objectGetString(const string& sKey, string& sVal) const;
    bool objectGetObject(const string& sKey, MfcJsonObj** ppVal) const;
    bool objectGetObject(const string& sKey, MfcJsonObj& jsVal) const;

    bool objectGetInt(const string& sKey, uint32_t& dwVal) const { return objectGetInt(sKey, (int32_t&)dwVal); }
    bool objectGetInt(const string& sKey, uint64_t& qwVal) const { return objectGetInt(sKey, (int64_t&)qwVal); }

    // Returns node under key sKey if this instance is an object
    MfcJsonObj* objectGet(const string& sKey) const
    {
        MfcJsonObj* pRet = NULL;
        MfcJsonIter iObj;

        if (isObject())
            if ((iObj = m_mObj.find(sKey)) != m_mObj.end())
                pRet = iObj->second;

        return pRet;
    }

    // Returns node at position nPos if this instance is an array
    MfcJsonObj* arrayAt(size_t nPos) const
    {
        return ( (isArray() && m_vArray.size() > nPos) ? m_vArray[nPos] : NULL );
    }

    //-- Specialized read/write stream helper functions ---------------------------------------------------------
    //
    // If JSON_T_ARRAY or JSON_T_OBJECT, read a stream of int64_ts or strings from container.
    // xxxRead() methods return the # of elements read, xxxWrite() methods return the new
    // size of the object after writing to it from input container.
    //
    // uint32_t/uint32_t methods added for easier compatability with legacy friends/ignore/etc lists
    //
    size_t arrayRead(set< uint32_t >& vVals) const;             // Read array of uint32_ts into a set
    size_t arrayRead(set< int64_t >& vVals) const;              // Read array of ints into a set
    size_t arrayRead(set< string >& vVals) const;               // Read array of strings into a set
    size_t arrayRead(vector< uint32_t >& vVals) const;          // Read array of uint32_ts into a vector
    size_t arrayRead(vector< int64_t >& vVals) const;           // Read array of ints into a vector
    size_t arrayRead(strVec& vVals) const;                      // Read array of strings into a vector
    size_t arrayRead(unordered_set< uint32_t >& vVals) const;   // Read array of uint32_ts into an unordered_set
    size_t arrayRead(unordered_set< int64_t >& vVals) const;    // Read array of ints into an unordered_set

    size_t arrayWrite(const set< uint32_t >& vVals);            // Write array of uint32_ts to node from a set
    size_t arrayWrite(const set< int64_t >& vVals);             // Write array of ints to node from a set
    size_t arrayWrite(const set< string >& vVals);              // Write array of strings to node from a set
    size_t arrayWrite(const vector< uint32_t >& vVals);         // Write array of uint32_ts to node from a vector
    size_t arrayWrite(const vector< int64_t >& vVals);          // Write array of ints to node from a vector
    size_t arrayWrite(const strVec& vVals);                     // Write array of strings to node from a vector
    size_t arrayWrite(const unordered_set< uint32_t >& vVals);  // Write array of uint32_ts to node from an unordered set
    size_t arrayWrite(const unordered_set< int64_t >& vVals);   // Write array of ints to node from an unordered set

    size_t objectRead(map< string,int64_t >& mVals) const;      // Read object of key:value ints into map
    size_t objectRead(map< string,string >& mVals) const;       // Read object of key:value strings into map
    size_t objectReadKeys(strVec& vKeys) const;                 // Read all keys at this level of this object

    /* Iterate through items in a JSON_T_OBJECT:
     *
     * MfcJsonIter iObj = js.objectEnum();
     * while (!js.objectEnd(iObj))
     * {
     *     MfcJsonObj* pVal = js.objectAt(iObj);
     *     cout << pVal->Serialize() << endl;
     *     iObj++;
     * }
     */

    MfcJsonIter objectEnum(void) const                          // Begin enumeration of JSON_T_OBJECT
    {
        return isObject() ? m_mObj.begin() : m_mObj.end();
    }

    bool objectEnd(MfcJsonIter& iObj) const                     // Test if iterator of m_mObj is at end
    {
        return (!isObject() || iObj == m_mObj.end() ? true : false);
    }

    MfcJsonObj* objectAt(MfcJsonIter& iObj) const               // Return MfcJsonObj* for value of key at iObj iter position
    {
        return iObj->second;
    }

    size_t objectWrite(const map< string,uint32_t >& mVals);    // Write object of key:value uint32_ts to node from map
    size_t objectWrite(const map< string,int64_t >& mVals);     // Write object of key:value ints to node from map
    size_t objectWrite(const map< string,string >& mVals);      // Write object of key:value strings to node from map

    //------------------------------------------------------------------------------------------------------------

    void setDoublePrecision(char* pszFmt)
    {
        if (pszFmt)
        {
            free(m_pszFloatPrecisionFmt);
            m_pszFloatPrecisionFmt = strdup(pszFmt);
        }
    }

    // Serialize a map of native values (no arrays or objects in children) to querystring arg, escaped
    size_t objectQueryString(string& sOut, bool fIncludeQuestionMark);

    string objectQueryString(bool fIncludeQuestionMark = false)
    {
        string s;
        objectQueryString(s, fIncludeQuestionMark);
        return s;
    }

    // Serialize this container or value to string.
    //
    // Pass JSOPT_NORMAL for regular serialization (quote encapsulate & escape strings)
    // Pass JSOPT_RAW to NOT quote encapsulate and escape string data,
    // Pass JSOPT_PRETTY to pad string with whitespace and carriage returns
    //
    // Returns size of string written to
    //
    size_t Serialize(string& str, int nOpt = JSOPT_NORMAL) const;

    // Wrapper for Serialize that returns string reference to m_sThisSerialized
    const string& Serialize(int nOpt = JSOPT_NORMAL);

    string prettySerialize(void) const
    {
        string s;
        Serialize(s, JSOPT_PRETTY);
        return s;
    }

    // Escapes delimiters and special characters that are used by the json format spec.
    static string EscapeString(const string& input)
    {
        ostringstream ss;
        for(string::const_iterator iter = input.begin(); iter != input.end(); iter++)
        {
            switch (*iter)
            {
                case '\\': ss << "\\\\"; break;
                case '"': ss << "\\\""; break;
                case '/': ss << "\\/"; break;
                case '\b': ss << "\\b"; break;
                case '\f': ss << "\\f"; break;
                case '\n': ss << "\\n"; break;
                case '\r': ss << "\\r"; break;
                case '\t': ss << "\\t"; break;
                default: ss << *iter; break;
            }
        }
        return ss.str();
    }

    // Adapted from chrome's V8 implementation of encodeUriComponent: http://v8.googlecode.com/svn/trunk/src/uri.js
    // ECMA-262 - 15.1.3.4 - URIEncodeComponent
    static bool encodeChar(unsigned char ch)
    {
        unsigned int n = (unsigned int)ch;
        // some internation characters in upper-ascii range that are ok for strings
//NO! Messes up utf8        if (n >= 188 && n <= 255)           return false;
        if (isalpha(n) || isdigit(n))       return false;       // A-Z  a-z  0-9
        if (n == 33 || n == 95 || n == 126) return false;       // !  _  ~
        if (39 <= n && n <= 42)             return false;       // ' () *
        if (45 <= n && n <= 46)             return false;       // - .

        return true;
    }

    // Used build valid querystrings.
    static const char* encodeURIComponent(const string& s, string& sOut)
    {
        size_t n = s.length();
        char szEnc[4] = { '\0' };

        sOut.clear();
        sOut.reserve(n * 3);

        for (size_t i = 0; i < n; ++i)
        {
            if (i + 1 == n && s[i] == '\0')
            {
                break;
            }
            else
            {
                if (encodeChar(((unsigned char)s[i])))
                {
                    szEnc[0] = '%';
                    szEnc[1] = sm_pszHexVals[((unsigned char)s[i]) / 16];
                    szEnc[2] = sm_pszHexVals[((unsigned char)s[i]) % 16];
                    sOut += szEnc;
                }
                else sOut += s[i];
            }
        }

        return sOut.c_str();
    }

    static string encodeURIComponent(const string& s)
    {
        string sOut;
        encodeURIComponent(s, sOut);
        return sOut;
    }

    //This is mostly for legacy compatibility.
    //Use 'void decodeURIComponent(string& inputString)' directly instead.
    static void decodeURIComponent(const string& s, string& sOut)
    {
        sOut = s;
        decodeURIComponent(sOut);
    }

    //Decode all URL style encodings (percent-encodings) in inputString directly. Replaces %XX with
    //the actual hex number in inputString. Useful for things such as converting escaped utf-8 and
    //other special characters. Encodings appear as an ascii string such as espa%C3%B1ol for español.
    //Technically, string's erase() can throw out_of_range, but no try block is used here since
    //the logic precludes the possibly of having that situation.
    static void decodeURIComponent(string& inputString)
    {
        size_t pos = 0;
        while((pos = inputString.find_first_of('%', pos)) != string::npos)
        {
            if(pos+2 <= inputString.length()-1) // There should be room for 2 ascii hex digits.
            {
                // Convert in place (and erase ascii hex representation).
                unsigned int digit1 = asciiHexDigitToInt(inputString[pos+1]);
                unsigned int digit2 = asciiHexDigitToInt(inputString[pos+2]);

                if(digit1 <= 15 && digit2 <= 15)
                {
                    inputString[pos] = static_cast< char >((digit1 << 4) | digit2); // Replace the % char with the real hex value.
                    inputString.erase(pos+1,2);                                     // Get rid of the 2 ascii hex chars.
                }
            }

            ++pos; // Move past % location.
        }
    }

    //Fast conversion of single ascii hex digit to binary.
    //Note that no check is done to ensure c is in a valid range.
    //The caller can verify by making sure the returned value is <= 15.
    static unsigned int asciiHexDigitToInt(char& c)
    {
        //The following is just adjusting an ascii 8-bit digit to its binary position
        //in successive steps to account for upper and lower case encodings.
        unsigned int digit = c - 48;            // assume 0 - 9

        if(digit > 15)                          // assume A - F
            digit -= (55-48);

        if(digit > 15)                          // assume a - f
            digit -= (87-55);

        return digit;
    }

    // convert hex values to ascii values, i.e. for '%20', pass in '2' and '0' and returns ' '
    //(see asciiHexDigitToInt() above for a faster alternative.)
    static char hexToChar(char ch1, char ch2)
    {
        int nVal1 = 0, nVal2 = 0;
        char CH1 = static_cast<char>(toupper(ch1));
        char CH2 = static_cast<char>(toupper(ch2));

        if (CH1 >= '0' && CH1 <= '9')
            nVal1 = CH1 - '0';
        else if (CH1 >= 'A' && CH1 <= 'F')
            nVal1 = CH1 - ('A' - 10);
        else
            return '.';

        if (CH2 >= '0' && CH2 <= '9')
            nVal2 = CH2 - 48;
        else if (CH2 >= 'A' && CH2 <= 'F')
            nVal2 = CH2 - ('A' - 10);
        else
            return '.';

        return (char)((nVal1 * 16) + nVal2);
    }

    uint32_t m_dwType;                          // Type of data this object represents (JSON_T_OBJECT, JSON_T_INTEGER, etc)

    //
    // data this object could represent
    //
    int64_t m_nVal;                             // Integer number
    double m_dVal;                              // Floating point number
    bool m_fVal;                                // Boolean value
    string m_sVal;                              // String data
    vector< MfcJsonObj* > m_vArray;             // Vector of other json data items (array)
    map< string,MfcJsonObj* > m_mObj;           // Map of other json data items (child object)

protected:

    void _makeType(uint32_t dwType)             // force an object to a given type, clearing it if the type changes
    {
        if (m_dwType != dwType)
        {
            clear();
            m_dwType = dwType;
        }
    }

    // static callback for JSON library code to call back into during deserialization when new value or state occurs
    static int _processJson(void* pCtx, int nType, const JSON_value* pValue);

    void _copyFrom(const MfcJsonObj& src);      // Copy one MfcJsonObj to another (recursive deep copy)
    void _initialize(JSON_type jsType);         // Initialize empty or zere/false type var

    char* m_pszFloatPrecisionFmt;               // if non-null, use this instead of %f for floating point precision in snprintf

    string m_lastDeserializedKey;               // Stores key for each entry specified in a json object during deserialization.

    size_t m_nUpdates;                          // Count of updates to object since last Serialize() (newly constructed objects start with 1)
    string m_sThisSerialized;                   // This json object serialized to a string, reference returned in Serialize()
};
