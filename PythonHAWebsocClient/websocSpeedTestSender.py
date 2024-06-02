from websocket import create_connection
import json
import time

_TOKEN = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhNjY4OTk1ZGQ5Mzg0YWRiOGEwZTk1OGUxZmM0YWJlZiIsImlhdCI6MTcwNTI0MjcyOSwiZXhwIjoyMDIwNjAyNzI5fQ.VKxml-fWpsuBZExIHViE2WEg1ZqxDcbNFFKWJIyflhc"
_HOST = "192.168.1.199"
_PORT = 8123
_SENSOR_INSTANCE_NAME = "laptop"

#sudo pip install speedtest-cli

class Ha_websocket_writer:

    HA_ACCESS_TOKEN = None
    HA_SERVER_HOST = ""
    HA_SERVER_PORT = 8123

    id_number = 0
    isConnected = False
    ha_websoc = None

    def __init__(self, host, port, token):
        self.HA_ACCESS_TOKEN = token
        self.HA_SERVER_HOST = host
        self.HA_SERVER_PORT = port

    def websoc_connect(self):

        self.id_number = 1

        self.ha_websoc = create_connection('ws://{}:{}/api/websocket'.format(self.HA_SERVER_HOST, self.HA_SERVER_PORT))
        #self.ha_websoc = create_connection('ws://192.168.1.103:8080')

        #TODO: if some reason server does not respont script will hang here, add timeout.
        if("auth_required" in self.ha_websoc.recv()):
            print("server is online and waiting for auth.")

        print("sending:", json.dumps({'type': 'auth', 'access_token': self.HA_ACCESS_TOKEN}))
        self.ha_websoc.send(json.dumps({'type': 'auth', 'access_token': self.HA_ACCESS_TOKEN}))

        raw_response =  self.ha_websoc.recv()
        if ("auth_ok" in raw_response):
            self.isConnected = True
            
    def websoc_close(self):
        self.isConnected = False
        self.ha_websoc.close()

    def websoc_write_json_obj(self, user_specified_json):

        for i in range(0,5):
            if(self.isConnected == True):
                break
            self.websoc_connect()
            time.sleep(500)
                
        if self.isConnected == False:
            print("[ ER ] couldn't connect to HA server.")
            return
    
        id_num_json = json.dumps({'id': self.id_number})
        self.id_number = self.id_number + 1

        json_merged = {**json.loads(id_num_json), **json.loads(user_specified_json)}
        merged_payload = json.dumps(json_merged)

        print("Sent to HA | ", merged_payload)
        self.ha_websoc.send(merged_payload)

        raw_response =  self.ha_websoc.recv()
        #"success":false
        try:
            resp_json = json.loads(raw_response)
            print(json.dumps(resp_json, indent=2))
        except:
            print(raw_response)


from subprocess import check_output
import math 

def convert_size(size_bytes):
   if size_bytes == 0:
       return "0B"
   size_name = ("B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB")
   i = int(math.floor(math.log(size_bytes, 1024)))
   p = math.pow(1024, i)
   s = round(size_bytes / p, 2)
   return "%s %s" % (s, size_name[i])
        

try:
    speedtest_output = check_output(["speedtest-cli", "--json"])
    speedtest_json_obj = json.loads(speedtest_output)

    h_download = convert_size(int(speedtest_json_obj['download']))
    H_upload = convert_size(int(speedtest_json_obj['upload']))
    H_ping = str(round(float(speedtest_json_obj['ping']), 2))

    download = round(int(speedtest_json_obj['download']))
    upload = round(int(speedtest_json_obj['upload']))
    ping = round(float(speedtest_json_obj['ping']), 2)
except Exception as e:
    print(e) 

#download = 10
#upload = 11
#ping = 15

ha_server_com = Ha_websocket_writer(_HOST, _PORT, _TOKEN)

ha_server_com.websoc_connect()

ha_server_com.websoc_write_json_obj(json.dumps({"type": "call_service", "domain": "websoc_sensor", "service": (_SENSOR_INSTANCE_NAME + "." + "create"), "service_data": {"name": "download_speed", "state": download, "icon":"mdi:download", "friendly_name":"Download Speed", "timeout":2, "device_class":"data_rate"}}))
ha_server_com.websoc_write_json_obj(json.dumps({"type": "call_service", "domain": "websoc_sensor", "service": (_SENSOR_INSTANCE_NAME + "." + "create"), "service_data": {"name": "upload_speed", "state": upload, "icon":"mdi:upload", "friendly_name":"Upload Speed", "timeout":2, "state_class":"measurement", "wrn":"hello world!", "device_class":"data_rate"}}))
ha_server_com.websoc_write_json_obj(json.dumps({"type": "call_service", "domain": "websoc_sensor", "service": (_SENSOR_INSTANCE_NAME + "." + "create"), "service_data": {"name": "ping", "state": ping, "icon":"mdi:lan-connect", "friendly_name":"Ping", "timeout":"1", "inf":"hello world!"}}))

#ha_server_com.websoc_write_json_obj(json.dumps({'type': 'subscribe_events', 'event_type': 'state_changed'}))

# get all entitiesrest
#ha_server_com.websoc_write_json_obj(json.dumps({'type': 'subscribe_entities', 'entity_ids': ['weather.forecast_haserver']}))
ping = 25
#get only selected ones
#ha_server_com.websoc_write_json_obj(json.dumps({'type': 'subscribe_entities', 'entity_ids': ['hub_rtu0.debug_json']}))("sol_bat_temp","Battery temp.",    True,     "temperature",  "°C",       'mdi:thermometer', "Unknown"),
#ha_server_com.websoc_write_json_obj(json.dumps({'type': 'get_states'}))
ha_server_com.websoc_write_json_obj(json.dumps({"type": "call_service", "domain": "websoc_sensor", "service": (_SENSOR_INSTANCE_NAME + "." + "update"), "service_data": {"name": "ping", "state": ping }}))

ha_server_com.websoc_write_json_obj(json.dumps({"type": "call_service", "domain": "notify", "service": "notify", "service_data": {"message": "kakka"}}))

ha_server_com.websoc_close()


