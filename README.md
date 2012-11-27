twiliomax
=========

A work in progress. This is the source code for an external object that enables communication with the Twilio SMS API from inside [Max 6][max-link].

[max-link]:http://cycling74.com/products/max/

The object takes two arguments for initialization, your Twilio Account SID and auth token. It requires at least one inbound number be active on your (paid) account.

![Example patch](http://miazmatic.com/data/uploads/max-objects/twiliomax/twiliomax-screen.png)

**The object supports two messages into its inlet:**

* sendsms - Fairly straightforward, this one sends the content of the second argument via SMS to the phone number in the first argument.


		sendsms "4085551234" "Hello there"


* receivesms - This one doesn't take any arguments, but does a lot of stuff. 

1. Starts a local web server 

2. Sets up a tunnel to that server using [localtunnel][lt-website] 

3. Sets the SMS URL of the incoming number in your twilio account to the external URL of the localtunnel

**The object currently sends three messages out of its only outlet:**

* sms

		sms "4089961010" "How are you"

This indicates that an inbound SMS was received at your Twilio number. The first argument is the phone number the SMS originated from, and the second is its content.

* receiving
		
This indicates that the setup required to receive incoming SMS messages completed on the low-priority Max thread and you should now see incoming messges. If you send "receivesms" and do not get a "receiving" message within a few seconds, check your Max window for errors. It's a complicated process and there's a lot that can go wrong!

* sent

		sent "4089961010"

This tells you that an SMS was sent, and tells you the number it was sent to. If you send "sendsms" and don't get a corresponding "sent" message, check your Max window for errors.

[lt-website]: http://progrium.com/localtunnel/

TODO
----
* Figure out if I'm really using libcurl in a thread-safe way (enable multiple instances of receivesms in the same patch)
* Error messages that tell you when your twilio credentials are wrong are not appearing. Figure out why.

Dependencies
-------------

Builds against the MaxMSP-6.0.4 SDK in Xcode 4.5 on Mac OS X.

Embeds [JSMN][jsmn-link] and [Mongoose][mongoose-link]

Uses [Clocaltunnel][clocaltunnel-link].

[clocaltunnel-link]:https://github.com/bengl3rt/clocaltunnel
[mongoose-link]:https://github.com/valenok/mongoose
[jsmn-link]:http://zserge.bitbucket.org/jsmn.html