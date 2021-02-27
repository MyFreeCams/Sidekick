/*
 * Copyright (c) 2013-2021 MFCXY, Inc. <mfcxy@mfcxy.com>
 * Created by Todd Anderson on 2019-04-05
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

#ifndef CEF_JS_EXTENSIONS_H_
#define CEF_JS_EXTENSIONS_H_

#include "include/cef_app.h"
#include "include/cef_v8.h"
#include "include/views/cef_browser_view.h"

#include <string>

class cefJSExtensionBase : public CefV8Handler
{
public:
    cefJSExtensionBase(const char* pParent, const char* pName)
        : m_sParent(pParent)
        , m_sName(pName)
    {}

    ~cefJSExtensionBase() OVERRIDE = default;

    std::string getName() { return m_sName; }
    void setName(const char* p) { m_sName = p; }
    void setName(const std::string& s) { m_sName = s; }

    std::string getParentName() { return m_sParent; }
    void setParentName(const char* p) { m_sParent = p; }
    void setParentName(const std::string& s) { m_sParent = s; }

    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) OVERRIDE;

    virtual bool execute(CefRefPtr<CefV8Value> object,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval) = 0;

    virtual bool executeCallBack(CefRefPtr<CefBrowser> browser,
                                 CefProcessId source_process,
                                 CefRefPtr<CefProcessMessage> message);

    virtual void addExtension(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefV8Context> context);

    // helper function
    virtual void CefV8Array2ListValue(CefRefPtr<CefV8Value> source, CefRefPtr<CefListValue> target);
    virtual void CefListValue2V8Array(CefRefPtr<CefListValue> source, CefRefPtr<CefV8Value> target);
    virtual void CefV8JsonObject2DictionaryValue(CefRefPtr<CefV8Value>source, CefRefPtr<CefDictionaryValue> target);
    virtual void CefDictionaryValue2V8JsonObject(CefRefPtr<CefDictionaryValue> source, CefRefPtr<CefV8Value> target);

private:
    std::string m_sParent;
    std::string m_sName;

    IMPLEMENT_REFCOUNTING(cefJSExtensionBase);
};


// map key is the call back name/the browser id
// value is the context and function value.
typedef std::map<std::pair<std::string, int>, std::pair<CefRefPtr<CefV8Context>, CefRefPtr<CefV8Value> > > CallbackMap;

class CObsJSLoginCallback : public cefJSExtensionBase
{
public:
    CObsJSLoginCallback();
    ~CObsJSLoginCallback() OVERRIDE;

    bool execute(CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval) OVERRIDE;

    bool executeCallBack(CefRefPtr<CefBrowser> browser,
                         CefProcessId source_process,
                         CefRefPtr<CefProcessMessage> message) OVERRIDE;

    CallbackMap& getCallbackMap() { return m_mapCallback; }

private:
    CallbackMap m_mapCallback;
};


#if 0
class JSFunctionBase : public CefV8Handler
{
public:
    JSFunctionBase(const char* pName);
    JSFunctionBase(const JSFunctionBase& src);
    ~JSFunctionBase() OVERRIDE;

    const JSFunctionBase& operator=(const JSFunctionBase& src);

    std::string getName() { return m_sName; }
    void setName(const char* p) { m_sName = p;}
    void setName(const std::string& s) {  m_sName = s; }

    virtual bool execute(const char *pName,
                         const CefV8ValueList& arguments,
                         CefRefPtr<CefV8Value>& retval) = 0;

    virtual void addExtension(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefV8Context> context,
                              CefRefPtr<CefV8Value> &obj);

private:
    std::string m_sName;
};


class CJSDoLogin : public JSFunctionBase
{
public:
    CJSDoLogin(const char* pName);
    ~CJSDoLogin() OVERRIDE;

    bool execute(const char* pName,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval) OVERRIDE;
};

typedef std::map<std::string, JSFunctionBase*> mapFunctions;

class JSObject : public CefV8Handler
{
public:
    JSObject(const char* pName);

    ~JSObject() OVERRIDE;

    bool addFunction(JSFunctionBase* pF );
    std::string getName() { return m_sName; }
    void setName(const char* p) { m_sName = p;}
    void setName(const std::string& s) {  m_sName = s; }


    bool Execute(const CefString& name,
                 CefRefPtr<CefV8Value> object,
                 const CefV8ValueList& arguments,
                 CefRefPtr<CefV8Value>& retval,
                 CefString& exception) OVERRIDE;

    virtual void addObject(CefRefPtr<CefBrowser> browser,
                           CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefV8Context> context);

    mapFunctions & getObjectMap() { return m_map; }

private:
    std::string m_sName;
    mapFunctions m_map;

    IMPLEMENT_REFCOUNTING(JSObject);
};

#endif

#endif  // CEF_JS_EXTENSIONS_H_