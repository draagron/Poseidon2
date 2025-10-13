
# Source Prioritization Analysis

The current implementation of source prioritization is in adequate, and a new approach is needed. 

To start off with I need the following. 

I need to get an overview of which sources provide which data and how frequently. 

For NMEA 2000 sources, I need to know for each of the PGNs we are interested in, i.e. the ones listed in src/components/NMEA2000Handlers.h, what is the SID of each source that provides this PGN, and how frequent it is provided by each source. 

For NMEA 1083 sentences, I need to know for each of the sentences we are interested in, i.e. the ones listed in src/components/NMEA0183Handler.h, what is the talker/sender ID and how frequently do each talker/sender provide this sentence.

For sake of overview, the the listing should preferably be organized in the same categories as the boatdata itself is structured. 

Building this overview of sources means listening to the incoming messages on NMEA 0183 and NMEA 2000 over a period of time, and this needs to be refreshed regularly, if not contstantly maintained as part of the ongoing processing of messages. 

I image that the outcome of this process is a data structure like this:

BoatData Category  -- 1:N -- Message [ NMEA-2000 PGN | NMEA-0183 Sentence] -- 1:N -- Source [ NMEA-2000-<SID> | NMEA-0183-<TalkerID>], Frequency (Hz), Time since last (ms), isStale (bool)

This overview must be accessible via a websocket endpoint, and there should be a web page created, similar to the one created for streaming interface created for boatdata. The nodejs-boatdata-viewer should be extended to include a separate page with the  overview of which sources provide which data and how frequently. 