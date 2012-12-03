MQTT to KVPUBSUB Router
=======================
Toby Jaffey <toby@sensemote.com>

This is a router to convert between MQTT's pubsub semantics and a 1:N unicast
protocol.

The 1-N unicast protocol is a line oriented, text based protocol called
KVPUBSUB. In KVPUBSUB, there are 3 commands from client to server (PUB, SUB,
UNSUB) and 4 responses from server to client (PUBACK, SUBACK, UNSUBACK, INF).

The router was originally designed to help with connecting RS232 radio modems
(ZigBee, Bluetooth, etc) to MQTT. Though, it can also be used from the command
line directly.


Quickstart
----------

# Build it
make
# Connect to console and MQTT broker, subscribe two clients to a topic
./router -v -d - -s test.mosquitto.org -p 1883 -t prefix/
SUB alice power id
SUB bob power id


# Publish data to the broker and see it routed to the clients
mosquitto_pub -h test.mosquitto.org -t "prefix/power" -m "1.21GW"

(You will see)
INF alice power 1.21GW
INF bob power 1.21GW


A simple publish example
------------------------

An energy monitor with address 0xCAFE reports power data to a radio gateway.
The gateway is connected via a serial line to the router.

A web service is subscribed to the MQTT topic PREFIX/power

* Energy monitor reports power of 69W to gateway
* Gateway writes "PUB 0xCAFE power 69W forgetmenot" to the router
* Router publishes "power" = "69W" to MQTT broker
* Router receives publish acknowledgement from MQTT broker
* Router writes "PUBACK 0xCAFE forgetmenot" back to gateway

The serial line between gateway and router sees:

-> PUB 0xCAFE power 69W forgetmenot
<- PUBACK 0xCAFE forgetmenot

All subscribed clients of the MQTT broker see "69W" on PREFIX/power.

A simple subscribe example
--------------------------

-> SUB alice power token
<- SUBACK alice token

-> SUB bob power token
<- SUBACK bob token

-> PUB carol power 1.21GW token
<- PUBACK carol token
-> INF alice power 1.21GW
-> INF bob power 1.21GW

Now, a publish to the MQTT topic PREFIX/power will result in INF messages to
both alice and bob.


Commands
========

A client of the router (eg. a wireless gateway) sends commands to the router
and receives responses. Responses are asynchronous and may arrive out of order.
The INF message may arrive at the client at any time.

Cmd: PUB addr key val id
Rsp: PUBACK addr id

Cmd: SUB addr key id
Rsp: SUBACK addr id

Cmd: UNSUB addr key id
Rsp: UNSUBACK addr id

Rsp: INF addr key val

Format
======

All commands are single lines of "\r\n" terminated ASCII text with arguments
separated by spaces.

The following escape sequences are accepted:
    '\ ' -> ' '
    '\n' -> newline
    '\r' -> CR
    '"'  -> "
    '\\' -> '\'

Eg. to publish data containing spaces:
PUB myaddr mykey This\ is\ all\ one\ argument myid


Caveats
=======

The router doesn't yet support MQTT wildcards, using # and + in key names will
likely cause problems.

The data structures used for internal dispatch of INF will not scale well to thousands of clients.

Subscriptions never timeout.

