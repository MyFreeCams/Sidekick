# Quick Start

see the design document "Design/IPC Event Status.docx"

## Include

A single include brings in the event system.

```
	#include <mfc_ipc.h>
```	

## Builds

We share boost and QT dependencies with obs-studio. So I placed mfc\_ipc in the same directory as my obs-studio build and based the build scripts on the same one we use for obs-studio plugins. (Boost 1.69, QT 5.14 or 5.10)

### Windows

Run the script Scripts\build.cmd from project root directory.

The solution is build64\mfcipc.sln

### Mac

Run the script Scripts\build.sh from the project root directory.

## Finding Events

1. Find all records that are to &quot;George&quot;

    ```
    CEventList el;
    el.addTo(&quot;George&quot;);
    el.fetch();

2. Find all events that are to George OR are of type post it. Since we called the addTo function twice, the criteria will be combine with an or.
```
    CEventList el;
    el.addTom(&quot;George&quot;);
    el.addEventType(EVT\_POST\_IT);
    el.fetch();
```
3. Find all events addressed to George AND that are of type post it.
    ```
    CEventList el;
    CFilter flt;
    flt.addTo(&quot;George&quot;);
    flt.addEventType(EVT\_POST\_IT);
    el.getFilters().push\_back(flt).

4. Alternative way to perform AND operation:
```    CEventList el;
    el.addTo(&quot;George&quot;);
    el.getFilters().back().setEventType(EVT\_POST\_IT);
    el.fetch();
```
5. Use a regex on any text field by starting the string with &quot;/:&quot;
    ```
    CEventList el;
    el.addTo(&quot;/:.\*&quot;);
    el.fetch();

## Add/Modify Event

Use the CEvent object to add new events. You can also modify any event object return from EventList.
```
    CEvent evt(&quot;serenity now!&quot; // topic
    , "George: // to
    , "Jerry" // from
    , EVT\_GENERIC. // event type
    , boost::posix\_time::seconds(60\*60\*48) // expire
    , "simple event payload."" // payload
    );
    // write the event to shared memory,
    evt.update();
```
The method CEvent::update will create a new event or it will update an existing event in shared memory.

## Simple event handler

Event handlers have call backs for event add/removal and Process add/removal.

You can derive a class from CSimpleEventHandler and implement your own handlers or use CSimpleEventHandler directly with a lambda.

### Sample

Below is a simple application that will process a &quot;post it&quot; event. The event is configured to be sent every time the target logins in.
```
	#include \&lt;MFC\_IPC.h\&gt;
	int main(int argc, char \*argv[])
	{
		// set the logging
		IPCUtil::setupLogPath(); // defined in IPCUtil.h
		// setup the router!
		// get the singleton router instance
		CRouter \*pR = CRouter::getInstance();
		assert(pR);

		// set the router id. Each process has a unique
		// string id. You want the router id to be the same
		// between runs.
		pR->setRouterID(&quot;Unique string&quot;);

		// starts the router background thread. You don&#39;t
		// have to call this, but I usually like to in unit tests.

		pR-\&gt;Start();

		// start by seeing if our post it event already exists.
		// by fetching all events with a matching topic.

		CEventList el;
		el.addTopic(sLogin.c\_str());

		// get matching event.
		el.fetch();
		// check the events we got back.
		bool bFnd = false;
		for(auto e : el)
		{
			// verify the topic. We really
			// should have only gotten
			// one event, but you never know
			// what was left in the que from a
			// previous run.

			if (sLogin == e.getTopic())
			{
				bFnd = true;
				assert(e.getReadFrequency() == EVT\_READ\_LOGIN);
				break;
			}
		}
		if (! bFnd)
		{
			//event doesn&#39;t&#39; exist create it.
			CEvent evt(sLogin.c\_str() // topic
				, EVT\_HANDLER\_ADDR // to
				, &quot;test&quot; // from
				, EVT\_POST\_IT. // event type
				, boost::posix\_time::seconds(60\*60\*48)
				, &quot;this message should be sent on every login. &quot;

			);
			// set the event to be always be sent at login.
			evt.setReadFrequency(EVT\_READ\_LOGIN);
			// just for testing, mark it as read. We should get
			// the record if it&#39;s been read or not.
			evt.markAsRead(EVT\_HANDLER\_ADDR);

			// write the event to shared memory, but don&#39;t worry
			// about triggering.
			evt.update(false);
			// now that the event exists, re-run the app
		}
		else
		{
		    // the event was found, so create an event handler.
	    	// create an event handler with a unique identifier string.
    		CSimpleEventHandler el(EVT\_HANDLER\_ADDR);
		
		    // this event handler will subscribe to all events using

		    // a regex wildcard match on the &quot;To&quot; field.
    		el.getSubscriptions().addTo(&quot;/:.\*&quot;);

		    // create a lambda function to handle the call back.
    		// You could also derive
    		// a class from SimpleEventHandler and
    		// override OnIncomingEvent
        	//
    		el.setIncomingFunc([&amp;](CEvent &amp;e){
            	if (sLogin == e.getTopic())
        		{
            		bGotLoginEVt = true;
        		}
    		});

		// create a 5 second duration..
		boost::posix\_time::time\_duration duration = boost::posix\_time::duration\_from\_string(&quot;00:00:05&quot;);
		int nCnt = 500;
		// loop until we get our event or our count expires
		while (! bGotLoginEVt &amp;&amp; nCnt-- \&gt; 0)
		{
    		boost::this\_thread::sleep(duration);
    		if (bGotLoginEVt)
    		{
        		_TRACE(&quot;Got login message.&quot;);
    		}
    		else
            {
    		    _TRACE(&quot;FAIL! Didn&#39;t get login message!&quot;);
            }
		}

		return 0;

	}

```