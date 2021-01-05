// MyFreeCams JavaScript extensions.

// system includes
#include <map>

// cef includes
#include "include/cef_app.h"

// solution includes
#include <libPlugins/IPCShared.h>

// project includes
#include "MfcJsExtensions.h"

#define _DIRECT_TO_BROADCAST
#ifdef _DIRECT_TO_BROADCAST
extern MFC_Shared_Mem::CMessageManager g_LocalRenderMemManager;
#endif

CMFCJsonExtension g_arrExtensions[] = {{"fcsAPI", "credentials", MSG_TYPE_DOCREDENTIALS},
				       {"fcsAPI", "YetAnotherCredentials", MSG_TYPE_DOCREDENTIALS}};

int g_nExtensionCnt = sizeof(g_arrExtensions) / sizeof(CMFCJsonExtension);

//---------------------------------------------------------------------
// addMFCExtensions
//
// static function to add all mfc extensions
void cefJSExtensionBase::addMFCExtensions(CefRefPtr<CefBrowser> browser,
					  CefRefPtr<CefFrame> frame,
					  CefRefPtr<CefV8Context> context)

{
	for (int i = 0; i < g_nExtensionCnt; i++) {
		CMFCJsonExtension *pExt = g_arrExtensions + i;
		pExt->addExtension(browser, frame, context);
	}
}

bool cefJSExtensionBase::processMFCExtensions(const CefString &name,
					      CefRefPtr<CefV8Value>value,
					      const CefV8ValueList &arguments,
					      CefRefPtr<CefV8Value> &retval)
{
	bool bRv = false;

	for (int i = 0; i < g_nExtensionCnt; i++) {
		CMFCJsonExtension *pExt = g_arrExtensions + i;
		if (name == pExt->getName())
		{
			if (pExt->execute(value, arguments, retval))
			{
				bRv = true;
				break;
			}
		}
	}
	return bRv;
}


bool cefJSExtensionBase::Execute(const CefString &name,
				 CefRefPtr<CefV8Value> object,
				 const CefV8ValueList &arguments,
				 CefRefPtr<CefV8Value> &retval,
				 CefString &exception)
{
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
		global->SetValue(sParent.c_str(), pParent,
				 V8_PROPERTY_ATTRIBUTE_NONE);
	}
	assert(pParent->IsObject());
	std::string sName = getName();

	CefRefPtr<CefV8Value> fn =
		CefV8Value::CreateFunction(sName.c_str(), this);
	pParent->SetValue(sName.c_str(), fn, V8_PROPERTY_ATTRIBUTE_NONE);
}


bool cefJSExtensionBase::executeCallBack(CefRefPtr<CefBrowser> browser,
					 CefProcessId source_process,
					 CefRefPtr<CefProcessMessage> message)
{
	return true;
}


void cefJSExtensionBase::CefV8Array2ListValue(CefRefPtr<CefV8Value> source,
					      CefRefPtr<CefListValue> target)
{
	assert(source->IsArray());

	int arg_length = source->GetArrayLength();
	if (arg_length == 0)
		return;

	// Start with null types in all spaces.
	target->SetSize(arg_length);

	for (int i = 0; i < arg_length; ++i) {
		CefRefPtr<CefV8Value> value = source->GetValue(i);
		if (value->IsBool()) {
			target->SetBool(i, value->GetBoolValue());
		} else if (value->IsInt() || value->IsUInt()) {
			target->SetInt(i, value->GetIntValue());
		} else if (value->IsDouble()) {
			target->SetDouble(i, value->GetDoubleValue());
		} else if (value->IsNull()) {
			target->SetNull(i);
		} else if (value->IsString() || value->IsDate()) {
			target->SetString(i, value->GetStringValue());
		} else if (value->IsArray()) {
			CefRefPtr<CefListValue> new_list =
				CefListValue::Create();
			CefV8Array2ListValue(value, new_list);
			target->SetList(i, new_list);
		} else if (value->IsObject()) {
			CefRefPtr<CefDictionaryValue> new_dictionary =
				CefDictionaryValue::Create();
			CefV8JsonObject2DictionaryValue(value, new_dictionary);
			target->SetDictionary(i, new_dictionary);
		}
	}
}


void cefJSExtensionBase::CefListValue2V8Array(CefRefPtr<CefListValue> source,
					      CefRefPtr<CefV8Value> target)
{
	assert(target->IsArray());

	int arg_length = static_cast<int>(source->GetSize());
	if (arg_length == 0)
		return;

	for (int i = 0; i < arg_length; ++i) {
		CefRefPtr<CefV8Value> new_value;

		CefValueType type = source->GetType(i);
		switch (type) {
		case VTYPE_BOOL:
			new_value = CefV8Value::CreateBool(source->GetBool(i));
			break;
		case VTYPE_DOUBLE:
			new_value =
				CefV8Value::CreateDouble(source->GetDouble(i));
			break;
		case VTYPE_INT:
			new_value = CefV8Value::CreateInt(source->GetInt(i));
			break;
		case VTYPE_STRING:
			new_value =
				CefV8Value::CreateString(source->GetString(i));
			break;
		case VTYPE_NULL:
			new_value = CefV8Value::CreateNull();
			break;
		case VTYPE_LIST: {
			CefRefPtr<CefListValue> list = source->GetList(i);
			new_value = CefV8Value::CreateArray(
				static_cast<int>(list->GetSize()));
			CefListValue2V8Array(list, new_value);
		} break;
		case VTYPE_DICTIONARY: {
			CefRefPtr<CefDictionaryValue> dictionary =
				source->GetDictionary(i);
			new_value = CefV8Value::CreateObject(NULL, NULL);
			CefDictionaryValue2V8JsonObject(dictionary, new_value);
		} break;
		default:
			break;
		}

		if (new_value.get()) {
			target->SetValue(i, new_value);
		} else {
			target->SetValue(i, CefV8Value::CreateNull());
		}
	}
}


void cefJSExtensionBase::CefV8JsonObject2DictionaryValue(
	CefRefPtr<CefV8Value> source, CefRefPtr<CefDictionaryValue> target)
{
	assert(source->IsObject());

	std::vector<CefString> keys;
	source->GetKeys(keys);
	std::vector<CefString>::const_iterator beg = keys.begin();
	std::vector<CefString>::const_iterator end = keys.end();
	for (std::vector<CefString>::const_iterator it = beg; it != end; ++it) {
		const CefString key = *it;
		CefRefPtr<CefV8Value> value = source->GetValue(key);

		if (value->IsBool()) {
			target->SetBool(key, value->GetBoolValue());
		} else if (value->IsDouble()) {
			target->SetDouble(key, value->GetDoubleValue());
		} else if (value->IsInt() || value->IsUInt()) {
			target->SetInt(key, value->GetIntValue());
		} else if (value->IsNull()) {
			target->SetNull(key);
		} else if (value->IsString() || value->IsDate()) {
			target->SetString(key, value->GetStringValue());
		} else if (value->IsArray()) {
			CefRefPtr<CefListValue> listValue =
				CefListValue::Create();
			CefV8Array2ListValue(value, listValue);
			target->SetList(key, listValue);
		} else if (value->IsObject()) {
			CefRefPtr<CefDictionaryValue> dictionaryValue =
				CefDictionaryValue::Create();
			CefV8JsonObject2DictionaryValue(value, dictionaryValue);
			target->SetDictionary(key, dictionaryValue);
		}
	}
}


void cefJSExtensionBase::CefDictionaryValue2V8JsonObject(
	CefRefPtr<CefDictionaryValue> source, CefRefPtr<CefV8Value> target)
{
	assert(target->IsObject());

	CefDictionaryValue::KeyList keys;
	source->GetKeys(keys);
	CefDictionaryValue::KeyList::const_iterator beg = keys.begin();
	CefDictionaryValue::KeyList::const_iterator end = keys.end();

	for (CefDictionaryValue::KeyList::const_iterator it = beg; it != end; ++it) {
		CefRefPtr<CefV8Value> new_value;
		CefString key = *it;
		CefValueType type = source->GetType(key);

		switch (type) {
		case VTYPE_BOOL:
			new_value =
				CefV8Value::CreateBool(source->GetBool(key));
			break;
		case VTYPE_DOUBLE:
			new_value = CefV8Value::CreateDouble(
				source->GetDouble(key));
			break;
		case VTYPE_INT:
			new_value = CefV8Value::CreateInt(source->GetInt(key));
			break;
		case VTYPE_STRING:
			new_value = CefV8Value::CreateString(
				source->GetString(key));
			break;
		case VTYPE_NULL:
			new_value = CefV8Value::CreateNull();
			break;
		case VTYPE_LIST: {
			CefRefPtr<CefListValue> list = source->GetList(key);
			new_value = CefV8Value::CreateArray(
				static_cast<int>(list->GetSize()));
			CefListValue2V8Array(list, new_value);
		} break;
		case VTYPE_DICTIONARY: {
			CefRefPtr<CefDictionaryValue> dictionary =
				source->GetDictionary(key);
			new_value = CefV8Value::CreateObject(NULL, NULL);
			CefDictionaryValue2V8JsonObject(dictionary, new_value);
		} break;
		default:
			break;
		}
		if (new_value.get()) {
			target->SetValue(key, new_value,
					 V8_PROPERTY_ATTRIBUTE_NONE);
		} else {
			target->SetValue(key, CefV8Value::CreateNull(),
					 V8_PROPERTY_ATTRIBUTE_NONE);
		}
	}
}


//---------------------------------------------------------------------------
// CMfcJSLoginCallback
//
// register call back function.
CMfcJSLoginCallback::CMfcJSLoginCallback()
	: cefJSExtensionBase("fcsAPI", "RegisterCallback", 0 )
{
}


CMfcJSLoginCallback::~CMfcJSLoginCallback() {}


//-----------------------------------------------------------------------------
// execute
//
// Javascript calls with 2 parameters.  Callback name and a javascript function
// to be executed (the callback)
//
// this runs in Renderer process.
bool CMfcJSLoginCallback::execute(CefRefPtr<CefV8Value> object,
				  const CefV8ValueList &arguments,
				  CefRefPtr<CefV8Value> &retval)
{
	// parameter 1 is the name
	// parameter 2 is the javascript call back function to be executed.
	if (arguments.size() == 2 && arguments[0]->IsString() &&
	    arguments[1]->IsFunction()) {
		std::string sMsgName = arguments[0]->GetStringValue();
		CefRefPtr<CefV8Context> context =
			CefV8Context::GetCurrentContext();

		int browser_id = context->GetBrowser()->GetIdentifier();

		getCallbackMap().insert(
			std::make_pair(std::make_pair(sMsgName, browser_id),
				       std::make_pair(context, arguments[1])));
		return true;
	}
	return false;
}


bool CMfcJSLoginCallback::executeCallBack(CefRefPtr<CefBrowser> browser,
					  CefProcessId source_process,
					  CefRefPtr<CefProcessMessage> message)
{
	// Execute the registered JavaScript callback if any.
	if (!getCallbackMap().empty()) {
		std::string sMessageName = message->GetName();

		CallbackMap::const_iterator it = getCallbackMap().find(
			std::make_pair(sMessageName, browser->GetIdentifier()));

		if (it != getCallbackMap().end()) {
			// Keep a local reference to the objects. The callback may remove itself
			// from the callback map.
			CefRefPtr<CefV8Context> context = it->second.first;
			CefRefPtr<CefV8Value> callback = it->second.second;

			// Enter the context.
			context->Enter();

			CefV8ValueList arguments;

			// First argument is the message name.
			arguments.push_back(
				CefV8Value::CreateString(sMessageName.c_str()));

			// Second argument is the list of message arguments.
			CefRefPtr<CefListValue> list =
				message->GetArgumentList();
			int nSize = static_cast<int>(list->GetSize());
			CefRefPtr<CefV8Value> args =
				CefV8Value::CreateArray(nSize);
			CefListValue2V8Array(list, args);
			arguments.push_back(args);
			// Execute the callback.
			callback->ExecuteFunction(NULL, arguments);
			// Exit the context.
			context->Exit();
		}
	}
	return true;
}



//----------------------------------------------------------------------------
// CMFCJsonExtension0
//
// handle remote login call.
//
// adds doLogin method to the  javascript DOM
CMFCJsonExtension::CMFCJsonExtension(const char *pObject, const char *pFunction, int nMessageType)
	: cefJSExtensionBase(pObject, pFunction, nMessageType)
{
}


CMFCJsonExtension::~CMFCJsonExtension() {}


//----------------------------------------------------------------------------
// execute
//
// call back when the CMFCJsonExtension api call has been executed.
bool CMFCJsonExtension::execute(CefRefPtr<CefV8Value> object,
				const CefV8ValueList &arguments,
				CefRefPtr<CefV8Value> &retval)
{
	if (arguments.size() == 1) {
		// parameter 1 is a json package of login credentials.
		std::string sJson = arguments[0]->GetStringValue();
		std::string sMsgName = getFunctionName();
#ifdef _DIRECT_TO_BROADCAST
		// direct to broadcast
		if (!g_LocalRenderMemManager.isInitialized())
			g_LocalRenderMemManager.init(false);

		// send message directly to the broadcast plugin,
		g_LocalRenderMemManager.sendMessage(
			MFC_Shared_Mem::CSharedMemMsg(
				ADDR_OBS_BROADCAST_Plugin,
				ADDR_CEF_JSEXTENSION,
				getMessageType(),
				sJson.c_str()));
#else
		// send via the existing cef internal ipc message system.

		// send the name of the extension and the json package.
		// send message to browser plugin!
		CefRefPtr<CefProcessMessage> msg =
			CefProcessMessage::Create(sName.c_str());
		CefRefPtr<CefListValue> args = msg->GetArgumentList();
		args->SetString(0, sJson);
		CefRefPtr<CefV8Context> context =
			CefV8Context::GetCurrentContext();
		context->GetBrowser()->SendProcessMessage(PID_BROWSER, msg);
#endif
	} else {
		assert(!"invalid number of parameters");
		return false;
	}
	return true;
}
