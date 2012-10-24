twiliomax
=========

A work in progress. This is the source code for an external object that enables communication with the Twilio SMS API from inside [Max 6][max-link].

[max-link]:http://cycling74.com/products/max/

The object takes two arguments for initialization, your Twilio Account SID and auth token. It requires at least one inbound number be active on your (paid) account.

![Example patch](http://new.tinygrab.com/d791f091e4699a3c36112bfaa7bf8c7a5ae054a614.png)

**The object supports two messages into its inlet:**

* sendsms - Fairly straightforward, this one sends the content of the second argument via SMS to the phone number in the first argument.


		sendsms "4085551234" "Hello there"


* receivesms - This one doesn't take any arguments, but does a lot of stuff. 

1. Starts a local web server 

2. Sets up a tunnel to that server using [localtunnel][lt-website] 

3. Sets the SMS URL of the incoming number in your twilio account to the external URL of the localtunnel


**The object currently only sends one message out of its only outlet:**

* sms

		sms "4089961010" "How are you"

This indicates that an inbound SMS was received at your Twilio number. The first argument is the phone number the SMS originated from, and the second is its content.

**As you can see, it does a lot of stuff, and there's a lot that can go wrong. It currently handles exactly none of it gracefully. Which leads us to...**

[lt-website]: http://progrium.com/localtunnel/

TODO
----
* Error handling plumbed up through Max GUI
* Figure out if I'm really using libcurl in a thread-safe way
* Feedback/status out to Max patch - message indicating when receive connection is actually established etc
* Plug into supposedly-forthcoming error handling in clocaltunnel ;-)


Dependencies
-------------

Builds against the MaxMSP-6.0.4 SDK in Xcode 4.5 on Mac OS X.

Embeds [JSMN][jsmn-link] and [Mongoose][mongoose-link]

Uses [Clocaltunnel][clocaltunnel-link].

[clocaltunnel-link]:https://github.com/bengl3rt/clocaltunnel
[mongoose-link]:https://github.com/valenok/mongoose
[jsmn-link]:http://zserge.bitbucket.org/jsmn.html