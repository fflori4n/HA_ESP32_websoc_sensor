from websocket import create_connection
import json

token = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhNjY4OTk1ZGQ5Mzg0YWRiOGEwZTk1OGUxZmM0YWJlZiIsImlhdCI6MTcwNTI0MjcyOSwiZXhwIjoyMDIwNjAyNzI5fQ.VKxml-fWpsuBZExIHViE2WEg1ZqxDcbNFFKWJIyflhc"
ha_server_host = "192.168.1.16"
ha_server_port = 8123

# connect to websocket
ws = create_connection('ws://{}:{}/api/websocket'.format(ha_server_host, ha_server_port))
# on connection, server should respond with '{"type":"auth_required","ha_version":"2069.69.69"}'
print(json.loads(ws.recv()))

ws = create_connection('ws://{}:{}/api/websocket'.format(ha_server_host, ha_server_port))
# Try send valid login if token is ok, client shall recieve '{"type":"auth_ok","ha_version":"2024.1.2"}'
ws.send('''{"type":"auth","access_token":"eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhNjY4OTk1ZGQ5Mzg0YWRiOGEwZTk1OGUxZmM0YWJlZiIsImlhdCI6MTcwNTI0MjcyOSwiZXhwIjoyMDIwNjAyNzI5fQ.VKxml-fWpsuBZExIHViE2WEg1ZqxDcbNFFKWJIyflhc"}''')
print(json.loads(ws.recv()))

# if token is invalid, auth will return: {"type":"auth_invalid","message":"Invalid access token or password"}
# NOTE: this will generate and invalid login attempt massage in HA web interface.
# Received '' <-- server returns empty request for some reason, does not allow auth requst, connection must be closed and reopened
# websocket._exceptions.WebSocketConnectionClosedException: Connection to remote host was lost.
ws.send(json.dumps({'id': 1,'type': 'subscribe_entities'}))
print(json.dumps(json.loads(ws.recv()), indent=2))
print(json.dumps(json.loads(ws.recv()), indent=2))
ws.send(json.dumps({'id': 2,'type': 'subscribe_entities', 'entity_ids': ['weather.forecast_haserver', 'sun.sun']}))
print(json.dumps(json.loads(ws.recv()),indent=2))
print(json.dumps(json.loads(ws.recv()),indent=2))
print(json.dumps(json.loads(ws.recv()),indent=2))
print(json.dumps(json.loads(ws.recv()),indent=2))
exit(0)
#socket is now connected and authenticated, we can start asking for data and sending data. id of message will be increased every time.
ws.send(json.dumps({'id': 1, 'type': 'subscribe_events', 'event_type': 'state_changed'}))
result =  ws.recv()
print("Received '%s'" % result)

# NOTE: {"id":1,"type":"result","success":false,"error":{"code":"id_reuse","message":"Identifier values have to increase."}} - websoc will close if id is repeated
#ws.send(json.dumps({'id': 1, 'type': 'subscribe_events', 'event_type': 'state_changed'}))
#result =  ws.recv()
#print("Received '%s'" % result)

ws.send(json.dumps({'id': 2, 'type': 'subscribe_entities'}))
result =  ws.recv()
print("Received '%s'" % result)

# ask for all states of HA lists an apsolute sh!tload of states
#ws.send(json.dumps({'id': 3, 'type': 'get_states'}))
#result =  ws.recv()
#print("Received '%s'" % result)

# play ping-pong with the server to confirm that websocket is active
#ws.send(json.dumps({'id': 4, 'type': 'ping'}))
#result =  ws.recv()
#print("Received '%s'" % result)


#ws.send(json.dumps({'id': 4, 'type': 'get_config'}))
#result =  ws.recv()
#print("Received '%s'" % result)
#
ws.send(json.dumps({'id': 5, 'type': 'mqtt_ESPRTU/get', 'entity_id': "hub_rtu0.box_temp_1"}))
result =  ws.recv()
parsed = json.loads(result)
print(json.dumps(parsed, indent=2, sort_keys=True))
#result =  ws.recv()
print("Received '%s'" % result)

ws.send(json.dumps({'id': 6, 'type': 'subscribe_events', 'event_type': 'state_changed'}))
result =  ws.recv()
print("Received '%s'" % result)

#ws.send(json.dumps({'id': 6, 'type': 'history/stream', 'entity_ids': ["hub_rtu0.box_temp_1"], 'start_time': '2023-12-13T18:46:42.647Z'}))
#result =  ws.recv()

result =  ws.recv()
print("Received '%s'" % result)

ws.close()