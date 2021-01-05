#pragma once

#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"


class cefJSExtensionBase : public CefV8Handler {
public:
	cefJSExtensionBase(const char *pParent, const char *pName, int nMessageType)
		: m_sParent(pParent), m_sName(pName), m_nMessageType(nMessageType)
	{
	}

	virtual ~cefJSExtensionBase() {}

	static void addMFCExtensions(CefRefPtr<CefBrowser> browser,
				     CefRefPtr<CefFrame> frame,
				     CefRefPtr<CefV8Context> context);

	static bool processMFCExtensions(const CefString &name,
					 CefRefPtr<CefV8Value> value,
					 const CefV8ValueList &arguments,
					 CefRefPtr<CefV8Value> &retval
	);


	std::string getName() { return m_sName; }
	void setName(const char *p) { m_sName = p; }
	void setName(const std::string &s) { m_sName = s; }

	std::string getParentName() { return m_sParent; }
	void setParentName(const char *p) { m_sParent = p; }
	void setParentName(const std::string &s) { m_sParent = s; }

    int getMessageType() { return m_nMessageType; }
    void setMessageType(int n) { m_nMessageType = n; }

	virtual bool Execute(const CefString &name,
			     CefRefPtr<CefV8Value> object,
			     const CefV8ValueList &arguments,
			     CefRefPtr<CefV8Value> &retval,
			     CefString &exception) OVERRIDE;

	virtual bool executeCallBack(CefRefPtr<CefBrowser> browser,
				     CefProcessId source_process,
				     CefRefPtr<CefProcessMessage> message);

	virtual bool execute(CefRefPtr<CefV8Value> object,
			     const CefV8ValueList &arguments,
			     CefRefPtr<CefV8Value> &retval) = 0;

	virtual void addExtension(CefRefPtr<CefBrowser> browser,
				  CefRefPtr<CefFrame> frame,
				  CefRefPtr<CefV8Context> context);

	// helper function
	virtual void CefV8Array2ListValue(CefRefPtr<CefV8Value> source,
					  CefRefPtr<CefListValue> target);
	virtual void CefListValue2V8Array(CefRefPtr<CefListValue> source,
					  CefRefPtr<CefV8Value> target);
	virtual void
	CefV8JsonObject2DictionaryValue(CefRefPtr<CefV8Value> source,
					CefRefPtr<CefDictionaryValue> target);
	virtual void
	CefDictionaryValue2V8JsonObject(CefRefPtr<CefDictionaryValue> source,
					CefRefPtr<CefV8Value> target);

	std::string getObjectName() { return m_sParent; }
	std::string getFunctionName() { return m_sName; }

private:
	std::string m_sParent;
	std::string m_sName;
    int m_nMessageType;
	IMPLEMENT_REFCOUNTING(cefJSExtensionBase);
};

// map key is the call back name/the browser id
// value is the context and function value.
typedef std::map<std::pair<std::string, int>,
		 std::pair<CefRefPtr<CefV8Context>, CefRefPtr<CefV8Value>>>
	CallbackMap;

//---------------------------------------------------------------------------
// CMFCJSLoginCallback
//
//
// base class for a call back from c++ to the javascript dom.
//
// currently not used.
class CMfcJSLoginCallback : public cefJSExtensionBase {
public:
	CMfcJSLoginCallback();
	virtual ~CMfcJSLoginCallback();

	virtual bool execute(CefRefPtr<CefV8Value> object,
			     const CefV8ValueList &arguments,
			     CefRefPtr<CefV8Value> &retval);

	bool executeCallBack(CefRefPtr<CefBrowser> browser,
			     CefProcessId source_process,
			     CefRefPtr<CefProcessMessage> message);

	CallbackMap &getCallbackMap() { return m_mapCallback; }

private:
	CallbackMap m_mapCallback;
};

//---------------------------------------------------------------------------
// CMFCJsonExtension
//
// add extension to the pages javascript object.  When executed
// a json package is sent to broadcast plugin.
//
class CMFCJsonExtension : public cefJSExtensionBase {
public:
	CMFCJsonExtension(const char *pObjectName, const char *pFunction, int nMessageType);
	virtual ~CMFCJsonExtension();

	virtual bool execute(CefRefPtr<CefV8Value> object,
			     const CefV8ValueList &arguments,
			     CefRefPtr<CefV8Value> &retval);
};
