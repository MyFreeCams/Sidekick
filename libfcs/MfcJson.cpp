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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>

#include <iostream>
#include <sstream>

#include "fcslib_string.h"
#include "JSON_parser.h"
#include "MfcJson.h"
#include "Log.h"

const char* MfcJsonObj::sm_pszHexVals = "0123456789ABCDEF";

void MfcJsonObj::clear(void)
{
    if (m_dwType == JSON_T_ARRAY)
    {
        for (unsigned int n = 0; n < m_vArray.size(); n++)
        {
            MfcJsonObj* pObj = m_vArray[n];

            delete pObj;
        }
    }
    else if (m_dwType == JSON_T_OBJECT)
    {
        while (m_mObj.size() > 0)
        {
            map< string,MfcJsonObj* >::iterator i = m_mObj.begin();

            MfcJsonObj* pObj = i->second;
            m_mObj.erase(i);
            delete pObj;
        }
    }

    m_dwType = JSON_T_NULL;
    m_mObj.clear();
    m_vArray.clear();
    m_nUpdates = 1;
}

MfcJsonObj::MfcJsonObj(JSON_type nType)
{
    _initialize(nType);
}

MfcJsonObj::MfcJsonObj(const MfcJsonObj& src)
{
    _initialize(JSON_T_NULL);
    _copyFrom(src);
}

MfcJsonObj::MfcJsonObj()
{
    _initialize(JSON_T_NULL);
}

#ifdef _MFCDEV_
// Shallow copy swap object method
void MfcJsonObj::swap(MfcJsonObj& js)
{
    // Save the contents of other object in temporary variables
    uint32_t dwType                 = js.m_dwType;
    vector< MfcJsonObj* > vArray    = js.m_vArray;
    map< string,MfcJsonObj* > mObj  = js.m_mObj;
    int64_t nVal                    = js.m_nVal;
    double dVal                     = js.m_dVal;
    bool fVal                       = js.m_fVal;
    string sVal                     = js.m_sVal;
    char* pszFloatPrecisionFmt      = js.m_pszFloatPrecisionFmt;
    string sThisSerialized          = js.m_sThisSerialized;
    size_t nUpdates                 = js.m_nUpdates;

    // Set other object to our current state
    js.m_dwType                     = m_dwType;
    js.m_vArray                     = m_vArray;
    js.m_mObj                       = m_mObj;
    js.m_nVal                       = m_nVal;
    js.m_dVal                       = m_dVal;
    js.m_fVal                       = m_fVal;
    js.m_sVal                       = m_sVal;
    js.m_pszFloatPrecisionFmt       = m_pszFloatPrecisionFmt;
    js.m_sThisSerialized            = m_sThisSerialized;
    js.m_nUpdates                   = m_nUpdates;

    // Set this object to the previous state of other object
    m_dwType                        = dwType;
    m_vArray                        = vArray;
    m_mObj                          = mObj;
    m_nVal                          = nVal;
    m_dVal                          = dVal;
    m_fVal                          = fVal;
    m_sVal                          = sVal;
    m_pszFloatPrecisionFmt          = pszFloatPrecisionFmt;
    m_sThisSerialized               = sThisSerialized;
    m_nUpdates                      = nUpdates;
}

// Detach value under sKey from this object
MfcJsonObj* MfcJsonObj::detach(const string& sKey)
{
    MfcJsonObj* pRet = NULL;
    if (isObject())
    {
        map< string,MfcJsonObj* >::iterator i = m_mObj.find(sKey);
        if (i != m_mObj.end())
        {
            pRet = i->second;
            m_mObj.erase(i);
            m_nUpdates++;
        }
    }
    return pRet;
}

// Detach value under position nPos from this array
MfcJsonObj* MfcJsonObj::detach(size_t nPos)
{
    MfcJsonObj* pRet = NULL;
    if (arrayLen() > nPos)
    {
        pRet = m_vArray.at(nPos);
        m_vArray.erase(m_vArray.begin() + nPos);
        m_nUpdates++;
    }
    return pRet;
}
#endif

void MfcJsonObj::_initialize(JSON_type jsType)
{
    m_dwType = jsType;
    m_nUpdates = 1;

    m_pszFloatPrecisionFmt = NULL;

    m_lastDeserializedKey = "";

    // Initialize basic types to their default values
    switch (m_dwType)
    {
        case JSON_T_BOOLEAN:        m_fVal = false; break;
        case JSON_T_FLOAT:          m_dVal = 0;     break;
        case JSON_T_INTEGER:        m_nVal = 0;     break;
    }
}

MfcJsonObj::MfcJsonObj(int64_t nVal)
{
    _initialize(JSON_T_INTEGER);
    m_nVal = nVal;
}

MfcJsonObj::MfcJsonObj(double dVal)
{
    _initialize(JSON_T_FLOAT);
    m_dVal = dVal;
}

MfcJsonObj::MfcJsonObj(bool fVal)
{
    _initialize(JSON_T_BOOLEAN);
    m_fVal = fVal;
}

MfcJsonObj::MfcJsonObj(const string& sVal)
{
    _initialize(JSON_T_STRING);
    m_sVal = sVal;
}

MfcJsonObj::MfcJsonObj(const char* pszVal)
{
    _initialize(JSON_T_STRING);
    m_sVal = pszVal;
}

const MfcJsonObj& MfcJsonObj::operator=(const MfcJsonObj& src)
{
    if (this != &src)
        _copyFrom(src);

    return *this;
}

void MfcJsonObj::_copyFrom(const MfcJsonObj& src)
{
    clear();

    m_dwType = src.m_dwType;
    static int s_nLevel = -1;

    s_nLevel++;

    string sMargin = string(s_nLevel * 4, ' ');

    switch (m_dwType)
    {
        case JSON_T_NULL:                                break;
        case JSON_T_STRING:         m_sVal = src.m_sVal; break;
        case JSON_T_BOOLEAN:        m_fVal = src.m_fVal; break;
        case JSON_T_FLOAT:          m_dVal = src.m_dVal; break;
        case JSON_T_INTEGER:        m_nVal = src.m_nVal; break;

        case JSON_T_OBJECT:
            for (map< string,MfcJsonObj* >::const_iterator i = src.m_mObj.begin(); i != src.m_mObj.end(); ++i)
                m_mObj[i->first] = new MfcJsonObj(*(i->second));
            break;

        case JSON_T_ARRAY:
            for (size_t n = 0; n < src.m_vArray.size(); n++)
                m_vArray.push_back( new MfcJsonObj( *(src.m_vArray[n]) ) );
            break;

        default:
            _DBG("Error in copy constructor -- node with unknown JSON type: %u", m_dwType);
            m_dwType = JSON_T_NULL;
            break;
    }

    s_nLevel--;

}

void MfcJsonObj::arrayAdd(int64_t nVal)
{
    _makeType(JSON_T_ARRAY);
    m_vArray.push_back(new MfcJsonObj(nVal));
    m_nUpdates++;
}

void MfcJsonObj::arrayAdd(double dVal)
{
    _makeType(JSON_T_ARRAY);
    m_vArray.push_back(new MfcJsonObj(dVal));
    m_nUpdates++;
}

void MfcJsonObj::arrayAdd(bool fVal)
{
    _makeType(JSON_T_ARRAY);
    m_vArray.push_back(new MfcJsonObj(fVal));
    m_nUpdates++;
}

void MfcJsonObj::arrayAdd(const string& sVal)
{
    _makeType(JSON_T_ARRAY);

    MfcJsonObj* pStr = new MfcJsonObj(sVal);
    m_vArray.push_back(pStr);
    m_nUpdates++;
}

void MfcJsonObj::arrayAdd(MfcJsonObj* pObj)
{
    if (pObj)
    {
        _makeType(JSON_T_ARRAY);
        m_vArray.push_back(pObj);
        m_nUpdates++;
    }
}

void MfcJsonObj::arrayAdd(const MfcJsonObj& jsVal)
{
    _makeType(JSON_T_ARRAY);
    m_vArray.push_back(new MfcJsonObj(jsVal));
    m_nUpdates++;
}

void MfcJsonObj::objectRemove(const string& sKey)
{
    map< string,MfcJsonObj* >::iterator i;
    if (isObject())
    {
        if ((i = m_mObj.find(sKey)) != m_mObj.end())
        {
            MfcJsonObj* pObj = i->second;
            m_mObj.erase(i);
            delete pObj;
            m_nUpdates++;
        }
    }
}

bool MfcJsonObj::objectAdd(const string& sKey, int64_t nVal, bool fReplace)
{
    _makeType(JSON_T_OBJECT);

    if (fReplace)
        objectRemove(sKey);

    if (m_mObj.find(sKey) == m_mObj.end())
    {
        m_mObj[sKey] = new MfcJsonObj(nVal);
        m_nUpdates++;

        return true;
    }

    return false;
}

bool MfcJsonObj::objectAdd(const string& sKey, double dVal, bool fReplace)
{
    _makeType(JSON_T_OBJECT);

    if (fReplace)
        objectRemove(sKey);

    if (m_mObj.find(sKey) == m_mObj.end())
    {
        m_mObj[sKey] = new MfcJsonObj(dVal);
        m_nUpdates++;

        return true;
    }

    return false;
}

bool MfcJsonObj::objectAdd(const string& sKey, bool fVal, bool fReplace)
{
    _makeType(JSON_T_OBJECT);

    if (fReplace)
        objectRemove(sKey);

    if (m_mObj.find(sKey) == m_mObj.end())
    {
        m_mObj[sKey] = new MfcJsonObj(fVal);
        m_nUpdates++;

        return true;
    }

    return false;
}

bool MfcJsonObj::objectAdd(const string& sKey, const string& sVal, bool fReplace)
{
    _makeType(JSON_T_OBJECT);

    if (fReplace)
        objectRemove(sKey);

    if (m_mObj.find(sKey) == m_mObj.end())
    {
        MfcJsonObj* pStr = new MfcJsonObj(sVal);
        m_mObj[sKey] = pStr;
        m_nUpdates++;

        return true;
    }

    return false;
}

bool MfcJsonObj::objectAdd(const string& sKey, MfcJsonObj* pObj, bool fReplace)
{
    if (pObj)
    {
        _makeType(JSON_T_OBJECT);

        if (fReplace)
            objectRemove(sKey);

        if (m_mObj.find(sKey) == m_mObj.end())
        {
            m_mObj[sKey] = pObj;
            m_nUpdates++;

            return true;
        }
        else
        {
            //Duplicate key found. Just retain original value.
            return false;
        }
    }
    else
    {
        return false;     //Safety. No object to add.
    }
}

bool MfcJsonObj::objectAdd(const string& sKey, const MfcJsonObj& json, bool fReplace)
{
    _makeType(JSON_T_OBJECT);

    if (fReplace)
        objectRemove(sKey);

    if (m_mObj.find(sKey) == m_mObj.end())
    {
        m_mObj[sKey] = new MfcJsonObj(json);
        m_nUpdates++;

        return true;
    }

    return false;
}

size_t MfcJsonObj::arrayRead(unordered_set< uint32_t >& stVals) const
{
    stVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isInt())
                stVals.insert((uint32_t)m_vArray[n]->m_nVal);

    return stVals.size();
}

size_t MfcJsonObj::arrayRead(unordered_set< int64_t >& stVals) const
{
    stVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isInt())
                stVals.insert(m_vArray[n]->m_nVal);

    return stVals.size();
}

size_t MfcJsonObj::arrayRead(set< uint32_t >& stVals) const
{
    stVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isInt())
                stVals.insert((uint32_t)m_vArray[n]->m_nVal);

    return stVals.size();
}

size_t MfcJsonObj::arrayRead(set< int64_t >& stVals) const
{
    stVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isInt())
                stVals.insert(m_vArray[n]->m_nVal);

    return stVals.size();
}

size_t MfcJsonObj::arrayRead(set< string >& stVals) const
{
    stVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isString())
                stVals.insert(m_vArray[n]->m_sVal);

    return stVals.size();
}

size_t MfcJsonObj::arrayRead(vector< uint32_t >& vVals) const
{
    vVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isInt())
                vVals.push_back((uint32_t)m_vArray[n]->m_nVal);

    return vVals.size();
}
size_t MfcJsonObj::arrayRead(vector< int64_t >& vVals) const
{
    vVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isInt())
                vVals.push_back(m_vArray[n]->m_nVal);

    return vVals.size();
}

size_t MfcJsonObj::arrayRead(strVec& vVals) const
{
    vVals.clear();

    if (isArray())
        for (size_t n = 0; n < m_vArray.size(); n++)
            if (m_vArray[n]->isString())
                vVals.push_back(m_vArray[n]->m_sVal);

    return vVals.size();
}

size_t MfcJsonObj::arrayWrite(const unordered_set< int64_t >& vVals)
{
    for (unordered_set< int64_t >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd(*i);

    return m_vArray.size();
}

size_t MfcJsonObj::arrayWrite(const unordered_set< uint32_t >& vVals)
{
    for (unordered_set< uint32_t >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd((int64_t)*i);

    return m_vArray.size();
}

size_t MfcJsonObj::arrayWrite(const set< int64_t >& vVals)
{
    for (set< int64_t >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd(*i);

    return m_vArray.size();
}

size_t MfcJsonObj::arrayWrite(const set< uint32_t >& vVals)
{
    for (set< uint32_t >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd((int64_t)*i);

    return m_vArray.size();
}

size_t MfcJsonObj::arrayWrite(const set< string >& vVals)
{
    for (set< string >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd(*i);

    return m_vArray.size();
}

size_t MfcJsonObj::arrayWrite(const vector< uint32_t >& vVals)
{
    for (vector< uint32_t >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd((int64_t)*i);

    return m_vArray.size();
}

size_t MfcJsonObj::arrayWrite(const vector< int64_t >& vVals)
{
    for (vector< int64_t >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd(*i);

    return m_vArray.size();
}

size_t MfcJsonObj::arrayWrite(const vector< string >& vVals)
{
    for (vector< string >::const_iterator i = vVals.begin(); i != vVals.end(); ++i)
        arrayAdd(*i);

    return m_vArray.size();
}

// Convert to and from 32bit <-> 64bit to avoid rebuilding many structs
bool MfcJsonObj::objectGetInt(const string& sKey, int32_t& nVal) const
{
    int64_t nVal64 = (int64_t)nVal;
    bool fRet = objectGetInt(sKey, nVal64);
    nVal = (int32_t)nVal64;

    return fRet;
}

bool MfcJsonObj::objectGetInt(const string& sKey, int64_t& nVal) const
{
    if (isObject())
    {
        map< string,MfcJsonObj* >::const_iterator i = m_mObj.find(sKey);
        if (i != m_mObj.end())
        {
            if (i->second->isInt())
            {
                nVal = i->second->m_nVal;
                return true;
            }
        }
    }

    return false;
}

bool MfcJsonObj::objectGetBool(const string& sKey, bool& fVal) const
{
    if (isObject())
    {
        map< string,MfcJsonObj* >::const_iterator i = m_mObj.find(sKey);
        if (i != m_mObj.end())
        {
            if (i->second->isBoolean())
            {
                fVal = i->second->m_fVal;
                return true;
            }
        }
    }

    return false;
}

bool MfcJsonObj::objectGetFloat(const string& sKey, double& dVal) const
{
    if (isObject())
    {
        map< string,MfcJsonObj* >::const_iterator i = m_mObj.find(sKey);
        if (i != m_mObj.end())
        {
            if (i->second->isFloat())
            {
                dVal = i->second->m_dVal;
                return true;
            }
        }
    }

    return false;
}

bool MfcJsonObj::objectGetString(const string& sKey, string& sVal) const
{
    if (isObject())
    {
        map< string,MfcJsonObj* >::const_iterator i = m_mObj.find(sKey);
        if (i != m_mObj.end())
        {
            if (i->second->isString())
            {
                sVal =i->second->m_sVal;
                return true;
            }
        }
    }

    return false;
}

bool MfcJsonObj::objectGetObject(const string& sKey, MfcJsonObj** ppVal) const
{
    if (isObject())
    {
        map< string,MfcJsonObj* >::const_iterator i = m_mObj.find(sKey);
        if (i != m_mObj.end())
        {
            *ppVal = i->second;
            return true;
        }
        //else _MESG("key %s not found!", sKey.c_str());
    }
    //else _MESG("isObject() failed!");

    return false;
}

bool MfcJsonObj::objectGetObject(const string& sKey, MfcJsonObj& jsVal) const
{
    if (isObject())
    {
        map< string,MfcJsonObj* >::const_iterator i = m_mObj.find(sKey);
        if (i != m_mObj.end())
        {
            jsVal = *(i->second);
            return true;
        }
    }

    return false;
}

size_t MfcJsonObj::objectReadKeys(vector< string >& vKeys) const
{
    vKeys.clear();

    if (isObject())
        for (map< string,MfcJsonObj* >::const_iterator i = m_mObj.begin(); i != m_mObj.end(); ++i)
            vKeys.push_back(i->first);

    return vKeys.size();
}

size_t MfcJsonObj::objectRead(map< string,int64_t >& mVals) const
{
    mVals.clear();

    if (isObject())
        for (map< string,MfcJsonObj* >::const_iterator i = m_mObj.begin(); i != m_mObj.end(); ++i)
            if (i->second->isInt())
                mVals[i->first] = i->second->m_nVal;

    return mVals.size();
}

size_t MfcJsonObj::objectRead(map< string,string >& mVals) const
{
    mVals.clear();

    if (isObject())
        for (map< string,MfcJsonObj* >::const_iterator i = m_mObj.begin(); i != m_mObj.end(); ++i)
            if (i->second->isString())
                mVals[i->first] = i->second->m_sVal;

    return mVals.size();
}

size_t MfcJsonObj::objectWrite(const map< string,int64_t >& mVals)
{
    for (map< string,int64_t >::const_iterator i = mVals.begin(); i != mVals.end(); ++i)
        objectAdd(i->first, i->second);

    return m_mObj.size();
}

size_t MfcJsonObj::objectWrite(const map< string,string >& mVals)
{
    for (map< string,string >::const_iterator i = mVals.begin(); i != mVals.end(); ++i)
        objectAdd(i->first, i->second);

    return m_mObj.size();
}

const string& MfcJsonObj::Serialize(int nOpt)
{
    if (m_nUpdates > 0)
    {
        Serialize(m_sThisSerialized, nOpt);
        m_nUpdates = 0;
    }

    return m_sThisSerialized;
}

size_t MfcJsonObj::Serialize(string& str, int nOpt) const
{
    string str2;
    int nCx;

    str.clear();

    if (m_dwType == JSON_T_OBJECT)
    {
        map< string,MfcJsonObj* >::const_iterator i;

        str += "{";

        nCx = 0;
        for (i = m_mObj.begin(); i != m_mObj.end(); ++i)
        {
            if (nCx > 0)
                str += ",";

            if (nOpt >= JSOPT_PRETTY)
            {
                if (nCx > 0)
                    str += " ";
                else
                {
                    str += "\n";
                    for (int nDx = 0; nDx <= nOpt; nDx++)
                        str += "   ";
                }
            }


            // Serialize key name
            str += "\"" + EscapeString(i->first) + "\":";

            if (nOpt >= JSOPT_PRETTY)
                str += " ";             // Space after : in key: value output

            // Serialize value
            str2 = "null";
            i->second->Serialize(str2, nOpt < JSOPT_PRETTY ? nOpt : nOpt + 1);
            str += str2;
            nCx++;
        }

        if (nOpt >= JSOPT_PRETTY)
        {
            str += "\n";
            for (int nDx = 0; nDx < nOpt; nDx++)
                str += "   ";
        }
        str += "}";
    }
    else if (m_dwType == JSON_T_ARRAY)
    {
        str += "[";

        for (nCx = 0; nCx < (int)m_vArray.size(); nCx++)
        {
            if (nCx > 0)
                str += ",";

            if (nOpt >= JSOPT_PRETTY)
            {
                if (nCx > 0)
                    str += " ";
                else
                {
                    str += "\n";
                    for (int nDx = 0; nDx <= nOpt; nDx++)
                        str += "   ";
                }
            }

            // Serialize value
            str2 = "null";
            m_vArray[nCx]->Serialize(str2, nOpt < JSOPT_PRETTY ? nOpt : nOpt + 1);
            str += str2;
        }

        if (nOpt >= JSOPT_PRETTY)
        {
            str += "\n";
            for (int nDx = 0; nDx < nOpt; nDx++)
                str += "   ";
        }
        str += "]";
    }
    else if (m_dwType == JSON_T_INTEGER)
    {
        str += stdprintf("%" INT64_FMT, m_nVal);
    }
    else if (m_dwType == JSON_T_FLOAT)
    {
        str += stdprintf(m_pszFloatPrecisionFmt ? m_pszFloatPrecisionFmt : "%.2f", m_dVal);
    }
    else if (m_dwType == JSON_T_BOOLEAN)
    {
        str += (m_fVal ? "true" : "false");
    }
    else if (m_dwType == JSON_T_STRING)
    {
        if (nOpt == JSOPT_RAW)
            str += m_sVal;
        else
            str += "\"" + EscapeString(m_sVal) + "\"";
    }
    else if (m_dwType == JSON_T_NULL)
    {
        str += "null";
    }

    return str.size();
}

bool MfcJsonObj::Deserialize(const BYTE* pData, size_t nLen)
{
    struct JSON_parser_struct* jc = NULL;
    JSON_config config;
    MfcJsonStack jsStack;
    bool fRet = false;
    size_t nCx = 0;

    // init local object map to empty
    clear();

    // add ourselves to stack
    jsStack.push(this);

    if (nLen > 0)
    {
        init_JSON_config(&config);

        config.depth                  = 20;
        config.callback               = MfcJsonObj::_processJson;
        config.allow_comments         = 1;
        config.handle_floats_manually = 1;
        config.callback_ctx             = (void*)&jsStack;

        jc = new_JSON_parser(&config);

        for (nCx = 0; nCx < nLen; nCx++)
        {
            int nNextChar = (int)pData[nCx];
            if (nNextChar <= 0)
                break;

            if (!JSON_parser_char(jc, nNextChar))
                break;
        }

        delete_JSON_parser(jc);

        if (nCx < nLen)
        {
            _MESG("Error in json decode, nCx[%d] < nLen[%d]: data: '%s'", (int)nCx, (int)nLen, string((const char*)pData, nLen).c_str());
            fRet = false;
        }
        else fRet = true;
    }

    return fRet;
}

int MfcJsonObj::_processJson(void* pCtx, int nType, const JSON_value* pValue)
{
    MfcJsonStack* pStack = (MfcJsonStack*)pCtx;

    if (pStack == NULL || pStack->size() < 1)
    {
        _MESG("Unable to process, null MfcJsonStack ptr or size 0");
        return 0;
    }

    MfcJsonObj* pCur = pStack->top();
    int nRet = 1;

    // handle non-value state changes first

    if (nType == JSON_T_ARRAY_END || nType == JSON_T_OBJECT_END)
    {
        if (pStack->size() > 1)         // pop off top from stack as long as that won't remove root object
            pStack->pop();

        return nRet;
    }

    if (nType == JSON_T_KEY)
    {
        if(pCur->isObject())
        {
            if(pValue->vu.str.value != NULL)
            {
                pCur->m_lastDeserializedKey = pValue->vu.str.value;
            }
            else
            {
                _MESG("Unable to save JSON key. Key is NULL.");
                pCur->m_lastDeserializedKey = "";
                nRet = 0;
            }
        }
        else
        {
            if(pValue->vu.str.value != NULL)
                _MESG("Unable to save JSON key '%s', parent not an object.", pValue->vu.str.value);
            else
                _MESG("Unable to save JSON key, parent not an object (also, key is NULL).");

            pCur->m_lastDeserializedKey = "";
            nRet = 0;
        }

        return nRet;
    }

    //
    // Create new value
    //
    MfcJsonObj* pChild = new MfcJsonObj((JSON_type)nType);

    if (nType == JSON_T_ARRAY_BEGIN || nType == JSON_T_OBJECT_BEGIN)
    {
        // If array or object and pCur->isNull, use root note instead of creating new instance
        if (pCur->isNull() && pStack->size() == 1)
        {
            delete pChild;                                  // delete child we previously created, we're using parent container
            pCur->_makeType((JSON_type)nType);              // make parent container of our new type
            pChild = pCur;                                  // set child to parent container ptr
        }
        else
        {
            pStack->push(pChild);                           // not root, so add child to stack
        }
    }

    //
    // Or else set value of non-container types
    //
    else if (nType == JSON_T_INTEGER)
        pChild->setInt((int64_t)pValue->vu.integer_value);

    else if (nType == JSON_T_FLOAT)
        pChild->setFloat(atof(pValue->vu.str.value));

    else if (nType == JSON_T_NULL)
        pChild->setNull();

    else if (nType == JSON_T_TRUE)
        pChild->setBoolean(true);

    else if (nType == JSON_T_FALSE)
        pChild->setBoolean(false);

    else if (nType == JSON_T_STRING)
        pChild->setString(string(pValue->vu.str.value));

    else
    {
        _MESG("Unable to set %s value, unhandled type", MapJsonType(nType));
        nRet = 0;
    }

    //
    // Add new type (if successfully set as container or value, and not the root node already) to parent container
    //
    if (nRet == 1 && pCur != pChild)
    {
        if (pCur->isObject())                                   // Add value to container object
        {
            if (pCur->m_lastDeserializedKey == "")              // make sure we have a key to add it under
            {
                _MESG("Unable to save %s value, no key to associate with in parent object!", MapJsonType(nType));
                nRet = 0;
            }
            else
            {
                bool success = pCur->objectAdd(pCur->m_lastDeserializedKey, pChild);    // add value with given key

                if(!success)
                    _MESG("Unable to save duplicate key '%s'; parent already had key defined.", pCur->m_lastDeserializedKey.c_str());

                pCur->m_lastDeserializedKey = "";
            }
        }

        else if (pCur->isArray())                               // .. or add to container vector
            pCur->arrayAdd(pChild);

        else                                                    // is parent not a container?
        {
            _MESG("Unable to save %s value, parent type %s is not a container!", MapJsonType(nType), MapJsonType(pCur->m_dwType));
            nRet = 0;
        }
    }

    if (nRet == 0)                                              // some problem with new value, remove from stack (if present) and delete, return error
    {
        if (pStack->top() == pChild)
            pStack->pop();

        delete pChild;
        pChild = NULL;
    }

    return nRet;
}

size_t MfcJsonObj::objectQueryString(string& sOut, bool fIncludeQuestionMark)
{
    sOut.clear();

    if (isObject())
    {
        for (map< string,MfcJsonObj* >::const_iterator i = m_mObj.begin(); i != m_mObj.end(); ++i)
        {
            if (i->second->isArray() == false && i->second->isObject() == false)
            {
                string sFirst, sSecond;

                if (sOut.size() > 0)                // string already has some data? speerate with '&'
                    sOut += "&";
                else if (fIncludeQuestionMark)      // beginning of string? if question mark requested, add it
                    sOut += "?";

                sOut += encodeURIComponent(i->first, sFirst);
                sOut += "=";
                sOut += encodeURIComponent(i->second->Serialize(JSOPT_RAW), sSecond);
            }
        }
    }

    return sOut.size();
}
