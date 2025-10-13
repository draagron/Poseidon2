## websocket interface for streaming boatdata to web client

Add an web-socket endpoint that a client, e.g. web front-end, can use to get real-time (or at least frequent updated) boatdata.  

Add also a http url, e.g. /stream on the ESP32 that a browser can use to fetch a html page with javascript code that will connect to ESP32 web-socket to stream data to the web-page.  The /stream url should  return a static html page from the local littlefs storage. 

Create also a basic version of the static html page with javascript included to display the different categories of boat data neatly on the page. Data should be grouped similarly to how it is grouped in the boatdata structure, and each data element should have short name, simiarl to the names used in the boatdata structure. 




