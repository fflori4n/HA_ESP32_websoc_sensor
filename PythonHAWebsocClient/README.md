# Example py scripts for communicating with HA

While trying to figure out how to connect to Homeassistant websocket, started testing with python for quick and dirty prototypeing.
These are the scripts that were used to send data to HA, might be useful for bulding a python baset HA_websoc_sensor in the future.

> [!NOTE]
These clients will all use an endpoint that needs to be created inside HA, so in my case a custom integration of type `websoc_sensor` handles creating the `laptop.create` endpoint, that takes the clients data in JSON format.

A call for authentification:

`{"type":"auth","access_token":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhNjY4OTk1ZGQ5Mzg0YWRiOGEwZTk1OGUxZmM0YWJlZiIsImlhdCI6MTcwNTI0MjcyOSwiZXhwIjoyMDIwNjAyNzI5fQ.VKxml-fWpsuBZExIHViE2WEg1ZqxDcbNFFKWJIyflhc"}`

A call for setting an entity would tiplically consist of something like:

`{"id": 2, "type": "call_service", "domain": "websoc_sensor", "service": "laptop.create", "service_data": {"name": "download_speed", "state": download, "icon":"mdi:download", "friendly_name":"Download Speed", "timeout":2, "device_class":"data_rate"}}`


## List of files:

### 1. testEchoWebsocServer.py
- simple python websocket for testing client and general stuff related to websocket library

### 2. simpleHAWebsocClient.py
- Establish a WebSocket connection to Home Assistant server using the provided host and port.
- Attempt to authenticate with the server using the provided access token.
- Subscribe to entity state changes and receive server responses.
- Send requests for specific entity data and receive server responses.
- Close the WebSocket connection after all operations are completed.

### 3. websocSpeedTestSender.py
- simple 'sensor' that executes a speedtest on debian-based machine and sends the results to be displayed in HA
- slightly more modular approach of the previous script:
- Contains the class `Ha_websocket_writer` which helps communicate with a Home Assistant server via WebSocket. It does the following:
    - **Initialization**: Sets up with server host, port, and access token.
    - **Websoc Connect**: Establishes a WebSocket connection and authenticates.
    - **Websoc Close**: Closes the WebSocket connection.
    - **Websoc Write JSON Object**: Sends JSON objects, handling reconnection if needed. Used for creating sensor entities for download/upload speeds and ping.
    > [!NOTE]
    > Class handles request 'id' that needs to be incremented with every sent msg after authentification.

- Establish a WebSocket connection to Home Assistant server using the provided host and port.
- Attempt to authenticate with the server using the provided access token.
- Send requests to create sensor entities for download speed, upload speed, and ping.
- Close the WebSocket connection after all operations are completed.




