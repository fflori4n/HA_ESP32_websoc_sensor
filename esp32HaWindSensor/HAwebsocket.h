/* Why am I not using the default ESP log? good question. */
/* quasy LOG level for Serial print statements : 0 - disbled, 1 - errors, 2 - warnings, 3 - info*/
#define HA_WEBSOC_DBG 4


#define HAWS_Print(_LOGLEVEL, _MSG) if((0 != HA_WEBSOC_DBG) & (_LOGLEVEL <= HA_WEBSOC_DBG)) do{\
      if(4u > _LOGLEVEL){Serial.print(F("HAWS|"));}\
      if(1u == _LOGLEVEL){Serial.print(F("[ ER ] "));}\
      else if(2u == _LOGLEVEL){Serial.print(F("[ WR ] "));}\
      else if(3u == _LOGLEVEL){Serial.print(F("[ OK ] "));}\
      Serial.print((_MSG));\
    }while(0)

#define HAWS_Println(_LOGLEVEL, _MSG) if((0 != HA_WEBSOC_DBG) & (_LOGLEVEL <= HA_WEBSOC_DBG)) do{\
      if(4u > _LOGLEVEL){Serial.print(F("HAWS|"));}\
      if(1u == _LOGLEVEL){Serial.print(F("[ ER ] "));}\
      else if(2u == _LOGLEVEL){Serial.print(F("[ WR ] "));}\
      else if(3u == _LOGLEVEL){Serial.print(F("[ OK ] "));}\
      Serial.println((_MSG));\
    }while(0)


/*  HAWS_Print(HAWS_info,"Info is logged");
  HAWS_Print(HAWS_warning,"Warning is logged");
  HAWS_Print(HAWS_error,"Er is logged");*/

enum HAWS_LogLvl {
  HAWS_noTag  = 0,
  HAWS_error     = 1,
  HAWS_warning   = 2,
  HAWS_ok        = 3
};




#define HA_WEBSOC_URL "ws://192.168.1.199:8123/api/websocket"
#define HA_TOKEN "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJhNjY4OTk1ZGQ5Mzg0YWRiOGEwZTk1OGUxZmM0YWJlZiIsImlhdCI6MTcwNTI0MjcyOSwiZXhwIjoyMDIwNjAyNzI5fQ.VKxml-fWpsuBZExIHViE2WEg1ZqxDcbNFFKWJIyflhc" /* NOTE: no secrets.h, this is amateur hour*/

#define SOC_TOKEN_AUTH_PAYLOAD "{\"type\":\"auth\",\"access_token\":\"" HA_TOKEN "\"}"
#define SOC_PING_PAYLOAD       "{\"id\":%d,\"type\":\"ping\"}"

#define IN_BUFF_SIZE 2048
#define HAWEBSOC_CLI_OUTBUFFER_SIZE 2048

#define HAWEBSOC_CLIENT_WAIT_FOR_RESPONSE_VERBOSE
#define BUFFER_POLL_TIME_MS 10

char comInBuffer[IN_BUFF_SIZE] = {0};
class HAWebsocClient {

    /* USES: #include "esp_websocket_client.h" */
  private:

    enum tResponse {
      response_positive,
      response_negative,
      response_timeout,
      response_error
    };
  
    static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
    {
      esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
      switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
          HAWS_Println(HAWS_ok, "WEBSOCKET_EVENT_CONNECTED");
          break;
        case WEBSOCKET_EVENT_DISCONNECTED:
          HAWS_Println(HAWS_noTag, "WEBSOCKET_EVENT_DISCONNECTED");
          break;
        case WEBSOCKET_EVENT_DATA:
          HAWS_Println(4, "WEBSOCKET_EVENT_DATA");
         /* HAWS_Print(4,"got: ");
          HAWS_Print(4,data->data_len);
          HAWS_Print(4," Bytes, OPcode: ");
          HAWS_Println(4,data->op_code);
          Serial.write((char *)data->data_ptr, data->data_len);
          Serial.println();*/

            if (comInBuffer[0] == '\0') {
              snprintf(comInBuffer,IN_BUFF_SIZE, (char *)data->data_ptr);
              return;
            }
          break;
        case WEBSOCKET_EVENT_ERROR:
          Serial.println(F("WEBSOCKET_EVENT_ERROR"));
          break;
      }
    }

  public:

    esp_websocket_client_config_t websocket_cfg = {};
    esp_websocket_client_handle_t haSocClient;

    char comOutBuffer[HAWEBSOC_CLI_OUTBUFFER_SIZE] = {0};

    bool inWriteLocked = false; /* a real RTOS semaphore would be better */
    uint16_t idNumber = 1;  /* this will be added into each JSON payload so HA can add it to response, has to be incremented with each msg sent */

    HAWebsocClient() {
      websocket_cfg.uri = HA_WEBSOC_URL;


    }

    tResponse waitForResponse(char* positiveRespKey, char* negativeRespKey, uint16_t timeout = 5000) {

      tResponse result = response_timeout;
      uint16_t responseTime = 0;

      for (; ((response_timeout == result) && (responseTime < (timeout / BUFFER_POLL_TIME_MS))); responseTime++) {

        if (comInBuffer[0] != '\0') {

#ifdef HAWEBSOC_CLIENT_WAIT_FOR_RESPONSE_VERBOSE
          HAWS_Print(HAWS_ok, "recived:");
          Serial.print(F("WS_RESP| "));
          Serial.println(comInBuffer);
#endif

          if ((positiveRespKey != NULL) && (strstr(comInBuffer, positiveRespKey) != NULL)) {
            result = response_positive;
          }
          else if ((negativeRespKey != NULL) && (strstr(comInBuffer, negativeRespKey) != NULL)) {
            result = response_negative;
          }
          comInBuffer[0] = '\0';

        }
        else {
          vTaskDelay(BUFFER_POLL_TIME_MS / portTICK_PERIOD_MS);
        }
      }

#ifdef HAWEBSOC_CLIENT_WAIT_FOR_RESPONSE_VERBOSE
      const char* const dbgResultLabels[] = {"Positive", "Negative", "Timeout", "Error"};
      Serial.print(F("WS_RESP|elapsed time: "));
      Serial.println(responseTime * BUFFER_POLL_TIME_MS);
      Serial.print(F("WS_RESP|Response: "));
      Serial.println(dbgResultLabels[(result & 0x03)]);
      Serial.println();

#endif
      return result;
    }

    int8_t write(char* jsonStringTemplate, uint16_t timeout, char* positiveResp, char* negativeResp) {

      uint16_t payloadSize = snprintf(this->comOutBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, jsonStringTemplate, idNumber);
      idNumber++;
      Serial.print(F("WS_SEND|sent: "));
      Serial.println(this->comOutBuffer);
      esp_websocket_client_send_text(haSocClient, this->comOutBuffer, payloadSize, portMAX_DELAY);

      return waitForResponse(positiveResp, negativeResp, timeout);
    }

    int8_t writeWithRetry(char* jsonStringTemplate, uint16_t timeout, char* positiveResp, char* negativeResp, uint16_t pauseRetry, uint8_t maxNumberOfTries) {

      tResponse haWebsocResponse = response_timeout;

      for (int numOfTx = 0; ((numOfTx < maxNumberOfTries) && (haWebsocResponse != response_positive)); numOfTx++) {

        /*if(false == esp_websocket_client_is_connected(haSocClient)){
          connectAndAuthSocket(2);
        }*/
        
        uint16_t payloadSize = snprintf(this->comOutBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, jsonStringTemplate, idNumber);
        idNumber++;
        Serial.print(F("WS_SEND|sent: "));
        Serial.println(this->comOutBuffer);
        esp_websocket_client_send_text(haSocClient, this->comOutBuffer, payloadSize, portMAX_DELAY);

        haWebsocResponse = waitForResponse(positiveResp, negativeResp, timeout);

        if (response_positive != haWebsocResponse) {
          vTaskDelay(pauseRetry / portTICK_PERIOD_MS);
        }
      }
      return haWebsocResponse;
    }

    int8_t pingPong(uint16_t timeout = 2000) {

      uint16_t payloadSize = snprintf(this->comOutBuffer, HAWEBSOC_CLI_OUTBUFFER_SIZE, SOC_PING_PAYLOAD, idNumber);
      idNumber++;
      /*Serial.print(F("WS_SEND|sent: "));
      Serial.println(this->comOutBuffer);*/
      esp_websocket_client_send_text(haSocClient, this->comOutBuffer, payloadSize, portMAX_DELAY);

      return waitForResponse("pong", NULL, timeout);
    }

    int8_t connectAndAuthSocket( uint8_t numberOfRetries = 10, uint16_t pauseBetweenRetries = 1500) {

      tResponse authenticationResult = response_timeout;
      uint16_t len;

      esp_websocket_client_stop(haSocClient);
      esp_websocket_client_destroy(haSocClient);

      for (int i = 0; ((authenticationResult != response_positive) && (i < numberOfRetries)); i++) {

        websocket_cfg.uri = HA_WEBSOC_URL;
        haSocClient = esp_websocket_client_init(&websocket_cfg);
        /* register for callbacks to trigger websocket_event_handler */
        esp_websocket_register_events(haSocClient, WEBSOCKET_EVENT_ANY, this->event_handler, (void *)haSocClient);
        esp_websocket_client_start(haSocClient);

        /* authenticate using token */
        len = snprintf(this->comOutBuffer,HAWEBSOC_CLI_OUTBUFFER_SIZE, SOC_TOKEN_AUTH_PAYLOAD);
        Serial.print("sending: ");
        Serial.println(this->comOutBuffer);
        esp_websocket_client_send_text(haSocClient, this->comOutBuffer, len, portMAX_DELAY);

        authenticationResult = waitForResponse("auth_ok", NULL, 5000);

        if ((response_positive != authenticationResult)) {
          
          /* Stop and destroy previous connection */
          esp_websocket_client_stop(haSocClient);
          esp_websocket_client_destroy(haSocClient);
          
          Serial.print("delay: ");
          Serial.println(pauseBetweenRetries);
          vTaskDelay(pauseBetweenRetries / portTICK_PERIOD_MS);
        }
      }

      if (response_positive == authenticationResult) {
        idNumber = 1;
      }
      return authenticationResult;
    }

    int8_t sendWithRetry() {
    }

    void stop() {
      esp_websocket_client_stop(haSocClient);
      esp_websocket_client_destroy(haSocClient);
    }

};
