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

// MyFreeCams JavaScript extensions.

// system includes
#include <string>
#include <map>

// solution includes
#include "cefJSExtensions.h"
#include <libfcs/Log.h>

bool cefJSExtensionBase::Execute(const CefString& name,
                                 CefRefPtr<CefV8Value> object,
                                 const CefV8ValueList& arguments,
                                 CefRefPtr<CefV8Value>& retval,
                                 CefString& exception)
{
    //_TRACE("Function Execute %s", name);
    return execute(object, arguments, retval);
}

void cefJSExtensionBase::addExtension(CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefV8Context> context)
{
    auto global = context->GetGlobal();

    std::string sParent = getParentName();

    CefRefPtr<CefV8Value> pParent = global->GetValue(sParent.c_str());
    if (pParent->IsUndefined())
    {
        pParent = CefV8Value::CreateObject(NULL, NULL);
        global->SetValue(sParent.c_str(), pParent, V8_PROPERTY_ATTRIBUTE_NONE);

    }
    assert(pParent->IsObject());
    std::string sName = getName();

    auto fn = CefV8Value::CreateFunction(sName.c_str(), this);
    pParent->SetValue(sName.c_str(), fn, V8_PROPERTY_ATTRIBUTE_NONE);
}

bool cefJSExtensionBase::executeCallBack(CefRefPtr<CefBrowser> browser,
                                         CefProcessId source_process,
                                         CefRefPtr<CefProcessMessage> message)
{
    return true;
}

void cefJSExtensionBase::CefV8Array2ListValue(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target)
{
    assert(source->IsArray());

    int arg_length = source->GetArrayLength();
    if (arg_length == 0)
        return;

    // Start with null types in all spaces.
    target->SetSize(arg_length);

    for (int i = 0; i < arg_length; ++i)
    {
        CefRefPtr<CefV8Value> value = source->GetValue(i);
        if (value->IsBool())
        {
            target->SetBool(i, value->GetBoolValue());
        }
        else if (value->IsInt() || value->IsUInt())
        {
            target->SetInt(i, value->GetIntValue());
        }
        else if (value->IsDouble())
        {
            target->SetDouble(i, value->GetDoubleValue());
        }
        else if (value->IsNull())
        {
            target->SetNull(i);
        }
        else if (value->IsString() || value->IsDate())
        {
            target->SetString(i, value->GetStringValue());
        }
        else if (value->IsArray())
        {
            CefRefPtr<CefListValue> new_list = CefListValue::Create();
            CefV8Array2ListValue(value, new_list);
            target->SetList(i, new_list);
        }
        else if (value->IsObject())
        {
            CefRefPtr<CefDictionaryValue> new_dictionary = CefDictionaryValue::Create();
            CefV8JsonObject2DictionaryValue(value, new_dictionary);
            target->SetDictionary(i, new_dictionary);
        }
    }
}

void cefJSExtensionBase::CefListValue2V8Array(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target)
{
    assert(target->IsArray());

    int arg_length = static_cast<int>(source->GetSize());
    if (arg_length == 0)
        return;

    for (int i = 0; i < arg_length; ++i)
    {
        CefRefPtr<CefV8Value> new_value;

        CefValueType type = source->GetType(i);
        switch (type)
        {
            case VTYPE_BOOL:
                new_value = CefV8Value::CreateBool(source->GetBool(i));
                break;
            case VTYPE_DOUBLE:
                new_value = CefV8Value::CreateDouble(source->GetDouble(i));
                break;
            case VTYPE_INT:
                new_value = CefV8Value::CreateInt(source->GetInt(i));
                break;
            case VTYPE_STRING:
                new_value = CefV8Value::CreateString(source->GetString(i));
                break;
            case VTYPE_NULL:
                new_value = CefV8Value::CreateNull();
                break;
            case VTYPE_LIST:
            {
                CefRefPtr<CefListValue> list = source->GetList(i);
                new_value = CefV8Value::CreateArray(static_cast<int>(list->GetSize()));
                CefListValue2V8Array(list, new_value);
                break;
            }
            case VTYPE_DICTIONARY:
            {
                CefRefPtr<CefDictionaryValue> dictionary = source->GetDictionary(i);
                new_value = CefV8Value::CreateObject(NULL,NULL);
                CefDictionaryValue2V8JsonObject(dictionary, new_value);
                break;
            }
            default:
                break;
        }

        if (new_value.get())
            target->SetValue(i, new_value);
        else
            target->SetValue(i, CefV8Value::CreateNull());
    }
}

void cefJSExtensionBase::CefV8JsonObject2DictionaryValue(CefRefPtr<CefV8Value>source, CefRefPtr<CefDictionaryValue> target)
{
    assert(source->IsObject());

    std::vector<CefString> keys;
    source->GetKeys(keys);
    std::vector<CefString>::const_iterator beg = keys.begin();
    std::vector<CefString>::const_iterator end = keys.end();
    for (std::vector<CefString>::const_iterator it = beg; it != end; ++it)
    {
        const CefString key = *it;
        CefRefPtr<CefV8Value> value = source->GetValue(key);

        if (value->IsBool())
        {
            target->SetBool(key, value->GetBoolValue());
        }
        else if (value->IsDouble())
        {
            target->SetDouble(key, value->GetDoubleValue());
        }
        else if (value->IsInt() || value->IsUInt())
        {
            target->SetInt(key, value->GetIntValue());
        }
        else if (value->IsNull())
        {
            target->SetNull(key);
        }
        else if (value->IsString() || value->IsDate())
        {
            target->SetString(key, value->GetStringValue());
        }
        else if (value->IsArray())
        {
            CefRefPtr<CefListValue> listValue = CefListValue::Create();
            CefV8Array2ListValue(value, listValue);
            target->SetList(key, listValue);
        }
        else if (value->IsObject())
        {
            CefRefPtr<CefDictionaryValue> dictionaryValue = CefDictionaryValue::Create();
            CefV8JsonObject2DictionaryValue(value, dictionaryValue);
            target->SetDictionary(key, dictionaryValue);
        }
    }
}

void cefJSExtensionBase::CefDictionaryValue2V8JsonObject(CefRefPtr<CefDictionaryValue> source, CefRefPtr<CefV8Value> target)
{
    assert(target->IsObject());

    CefDictionaryValue::KeyList keys;
    source->GetKeys(keys);
    CefDictionaryValue::KeyList::const_iterator beg = keys.begin();
    CefDictionaryValue::KeyList::const_iterator end = keys.end();

    for (CefDictionaryValue::KeyList::const_iterator it = beg; it != end; ++it)
    {
        CefRefPtr<CefV8Value> new_value;
        CefString key = *it;
        CefValueType type = source->GetType(key);

        switch (type)
        {
            case VTYPE_BOOL:
                new_value = CefV8Value::CreateBool(source->GetBool(key));
                break;
            case VTYPE_DOUBLE:
                new_value = CefV8Value::CreateDouble(source->GetDouble(key));
                break;
            case VTYPE_INT:
                new_value = CefV8Value::CreateInt(source->GetInt(key));
                break;
            case VTYPE_STRING:
                new_value = CefV8Value::CreateString(source->GetString(key));
                break;
            case VTYPE_NULL:
                new_value = CefV8Value::CreateNull();
                break;
            case VTYPE_LIST:
            {
                CefRefPtr<CefListValue> list = source->GetList(key);
                new_value = CefV8Value::CreateArray(static_cast<int>(list->GetSize()));
                CefListValue2V8Array(list, new_value);
                break;
            }
            case VTYPE_DICTIONARY:
            {
                CefRefPtr<CefDictionaryValue> dictionary = source->GetDictionary(key);
                new_value = CefV8Value::CreateObject(NULL,NULL);
                CefDictionaryValue2V8JsonObject(dictionary, new_value);
                break;
            }
            default:
                break;
        }

        if (new_value.get())
            target->SetValue(key, new_value, V8_PROPERTY_ATTRIBUTE_NONE);
        else
            target->SetValue(key, CefV8Value::CreateNull(), V8_PROPERTY_ATTRIBUTE_NONE);
    }
}

//---------------------------------------------------------------------------
// CObsJSLoginCallback
//
// register call back function.
CObsJSLoginCallback::CObsJSLoginCallback()
    : cefJSExtensionBase("fcsAPI", "RegisterCallback")
{}

CObsJSLoginCallback::~CObsJSLoginCallback()
{}

//-----------------------------------------------------------------------------
// execute
//
// Javascript calls with 2 parameters.  Callback name and a javascript function
// to be executed (the callback)
//
// this runs in Renderer process.
bool CObsJSLoginCallback::execute(CefRefPtr<CefV8Value> object,
                                  const CefV8ValueList& arguments,
                                  CefRefPtr<CefV8Value>& retval)
{
    // parameter 1 is the name
    // parameter 2 is the javascript call back function to be executed.
    if (arguments.size() == 2 && arguments[0]->IsString() && arguments[1]->IsFunction())
    {
        std::string sMsgName = arguments[0]->GetStringValue();
        CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();

        int browser_id = context->GetBrowser()->GetIdentifier();

        getCallbackMap().insert(
            std::make_pair(std::make_pair(sMsgName, browser_id),
                std::make_pair(context, arguments[1])));
        return true;
    }
    return false;
}

bool CObsJSLoginCallback::executeCallBack(CefRefPtr<CefBrowser> browser,
                                          CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message)
{
    // Execute the registered JavaScript callback if any.
    if (!getCallbackMap().empty())
    {
        std::string sMessageName = message->GetName();

        _TRACE("Looking up callback %s", sMessageName.c_str());

        CallbackMap::const_iterator it =
            getCallbackMap().find(std::make_pair(sMessageName,browser->GetIdentifier()));

        if (it != getCallbackMap().end())
        {
            // Keep a local reference to the objects. The callback may remove itself
            // from the callback map.
            CefRefPtr<CefV8Context> context = it->second.first;
            CefRefPtr<CefV8Value> callback = it->second.second;

            // Enter the context.
            context->Enter();

            CefV8ValueList arguments;

            // First argument is the message name.
            arguments.push_back(CefV8Value::CreateString(sMessageName.c_str()));

            // Second argument is the list of message arguments.
            CefRefPtr<CefListValue> list = message->GetArgumentList();
            int nSize = static_cast<int>(list->GetSize());
            CefRefPtr<CefV8Value> args = CefV8Value::CreateArray(nSize);
            CefListValue2V8Array(list, args);
            arguments.push_back(args);
            _TRACE("Executing call back");
            // Execute the callback.
            callback->ExecuteFunction(NULL, arguments);
            // Exit the context.
            context->Exit();
        }
        else
        {
            _TRACE("No call back found");
        }
    }
    else
    {
        _TRACE("Callback map is empty");
    }
    return true;
}

/*
JSFunctionBase::JSFunctionBase(const char *pName)
    : m_sName(pName)
{
}

JSFunctionBase::JSFunctionBase(const JSFunctionBase &src)
{
    operator=(src);
}

JSFunctionBase::~JSFunctionBase()
{}

const JSFunctionBase &JSFunctionBase::operator=(const JSFunctionBase &src)
{
    m_sName = src.m_sName;
    return *this;
}

void JSFunctionBase::addExtension(CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame,
                                  CefRefPtr<CefV8Context> context,
                                  CefRefPtr<CefV8Value> &obj,
                                  JSObject *pObj)
{
    std::string s = getName();
    auto fnMFC = CefV8Value::CreateFunction(s.c_str(), pObj);
    obj->SetValue(s.c_str(), fnMFC, V8_PROPERTY_ATTRIBUTE_NONE);
}

CJSDoLogin::CJSDoLogin(const char *pName)
    : JSFunctionBase(pName)
{}

CJSDoLogin::~CJSDoLogin()
{}

bool CJSDoLogin::execute(const char *pName,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval)
{
    return true;
}

JSObject::JSObject(const char *pName)
    : m_sName(pName)
{}

JSObject::~JSObject()
{
    for (mapFunctions::iterator itr = m_map.begin(); itr != m_map.end(); ++itr)
    {
        delete itr->second;
        itr->second = nullptr;
    }
    m_map.clear();
}

//---------------------------------------------------------------------
// addFunction
//
// add a function to the javascript object on the cef web page
bool JSObject::addFunction(JSFunctionBase *f)
{
    mapFunctions::iterator itr  = m_map.find(f->getName());
    if (itr == m_map.end())
    {
        std::string s = f->getName();
        std::transform(s.begin(),s.end(),s.begin(),::tolower);
        m_map[s] = f;
        return true;
    }
    return false;
}

void JSObject::addObject(CefRefPtr<CefBrowser> browser,
                         CefRefPtr<CefFrame> frame,
                         CefRefPtr<CefV8Context> context)
{
    auto global = context->GetGlobal();
    auto MyJsObject = CefV8Value::CreateObject(NULL, NULL);
    global->SetValue(getName().c_str(), MyJsObject, V8_PROPERTY_ATTRIBUTE_NONE);

    for (mapFunctions::iterator itr = getObjectMap().begin(); itr != getObjectMap().end(); ++itr)
    {
        std::string s = itr->first;
        JSFunctionBase *pFN = itr->second;
        pFN->addExtension(browser,frame,context);

        // create login function
        auto fnMFC = CefV8Value::CreateFunction(s.c_str(), MyJsObject);
        MyJsObject->SetValue(s.c_str(), fnMFC, V8_PROPERTY_ATTRIBUTE_NONE);
    }
}

bool JSObject::Execute(const CefString& name,
                       CefRefPtr<CefV8Value> object,
                       const CefV8ValueList& arguments,
                       CefRefPtr<CefV8Value>& retval,
                       CefString& exception)
{
    //_TRACE("Function Execute %s", name);

    std::string s = name;
    std::transform(s.begin(),s.end(),s.begin(),::tolower);
    if (m_map.find(s) != m_map.end())
    {
        JSFunctionBase *pFn = m_map[s];
        pFn->execute(s.c_str(),arguments,retval);
    }

    if (name == "getVersion")
    {
        retval = CefV8Value::CreateString("Version(SemVer) # 0.0.1");
    }
    else if (name == "doLogin")
    {
        //size_t nSize = arguments.size();
        if (arguments.size() == 2)
        {
            // 2 parameters, user id and password
            std::string sName = arguments[0]->GetStringValue();
            std::string sPwd = arguments[1]->GetStringValue();
        }
        retval = CefV8Value::CreateString("ok"); // todo: what do we need here?
    }
    return true;
}

*/
