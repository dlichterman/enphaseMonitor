#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <StreamUtils.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include "creds.h"

#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  300        //Time ESP32 will go to sleep (in seconds)

//Fill in wifi credentials in creds.h
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

//Fill in Enphase credentials in creds.h
const char* appkey   = APPLICATION_KEY;
const char* user_id = USER_ID;
const char* systemid = SYSTEM_ID;

bool debugmode = false; //sends data back over serial connection if enabled

//2.9" V2 Waveshare B/W e-paper
GxEPD2_BW<GxEPD2_290_T94, GxEPD2_290_T94::HEIGHT> display(GxEPD2_290_T94(/*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4)); // GDEM029T94
 
void setup() {
 
  Serial.begin(115200);
  delay(1000);
  display.init();
  updateData(0,0,"","starting up.....",true);
  display.hibernate();
}
 
const char* root_ca= \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
"d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
"ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
"MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
"LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
"RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
"+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
"PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
"xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
"Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
"hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
"EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
"MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
"FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
"nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
"eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
"hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
"Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
"vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
"+OkuE6N36B9K\n" \
"-----END CERTIFICATE-----\n";
 
void loop() {

  WiFi.begin(ssid, password); 
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  
  if ((WiFi.status() == WL_CONNECTED)) //Check the current connection status
  { 
 
    HTTPClient http;
    String url = "https://api.enphaseenergy.com/api/v2/systems/";
    url += systemid;
    url += "/summary?key=";
    url += appkey;
    url += "&user_id=";
    url += user_id;
    url += "&datetime_format=iso8601";
    if(debugmode)
    {
      Serial.println(url);
    }

    http.useHTTP10(true);
    http.begin(url, root_ca); //Specify the URL and certificate
    int httpCode = http.GET();                                                  //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
        //String payload = http.getString();
        Serial.print("HTTP Code: ");
        Serial.println(httpCode);

        DynamicJsonDocument doc(512);
        if(debugmode)
        {
          ReadLoggingStream loggingStream(http.getStream(), Serial);
          Serial.println("");
          deserializeJson(doc, loggingStream);
        }
        else
        {
          deserializeJson(doc, http.getStream());
        }
        // Read values
        //long system_id = doc["system_id"]; // 
        //int modules = doc["modules"]; // 
        //int size_w = doc["size_w"]; // 
        int current_power = doc["current_power"]; // 
        int energy_today = doc["energy_today"]; // 1
        //long energy_lifetime = doc["energy_lifetime"]; // 
        //const char* summary_date = doc["summary_date"]; // "2021-02-07T00:00:00-08:00"
        //const char* source = doc["source"]; // "microinverters"
        const char* status = doc["status"]; // "normal"
        //const char* operational_at = doc["operational_at"]; // "2021-01-12T14:31:35-08:00"
        const char* last_report_at = doc["last_report_at"]; // "2021-02-07T19:57:25-08:00"
        //const char* last_interval_end_at = doc["last_interval_end_at"]; // "2021-02-07T17:34:00-08:00"
        String eng = String(energy_today/1000.000).c_str();
        Serial.println(eng);
        Serial.println(last_report_at);
        String s = last_report_at;
        s = s.substring(0,s.length()-6); //Removing timezone from time string
        updateData(current_power,energy_today,status,s.c_str(),false);
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
    WiFi.disconnect();
  }
  else
  {
    //If not connected, restart to try and connect again.
    Serial.println("No wifi, restarting");
    ESP.restart();
  }

  //Use light sleep to start back at the top of the loop code
  //otherwise the screen init would happen and flash
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_light_sleep_start();
 
}
void updateData(int current_power, int energy_today,const char* statusstr, const char* timestr, bool fullRefresh)
{
  //We need to define some of the strings that show
  char disp_kwh_curr[10] = "Current: ";
  char disp_kwh_today[10] = "Total:   ";
  char disp_status[9] = "Status: ";

  //Take convert to kilowatts
  char kwh_current[15];
  dtostrf((float(current_power)/1000.000),6,3,kwh_current);
  char kwh_today[15];
  dtostrf((float(energy_today)/1000.000),6,3,kwh_today);

  //Rotate on side, set font and color
  display.setRotation(1);
  display.setFont(&FreeMonoBold12pt7b);
  display.setTextColor(GxEPD_BLACK);

  //Calculate starting positions
  int16_t tbx, tby; uint16_t tbw, tbh;
  uint16_t data_start_x, line1_height, currdata_len, status_x;    
  display.getTextBounds(disp_kwh_curr, 0, 0, &tbx, &tby, &data_start_x, &line1_height);
  display.getTextBounds(kwh_current, 0, 0, &tbx, &tby, &currdata_len, &line1_height);
  display.getTextBounds(disp_status, 0, 0, &tbx, &tby, &status_x, &line1_height);

  if(fullRefresh)
  {
    display.setFullWindow();
  }
  
  display.firstPage();
  do
  {
    if(fullRefresh)
    {
      display.fillScreen(GxEPD_WHITE);
    }
    else
    {
      display.setPartialWindow(0,0,296,128);
    }
    
    //line 1
    display.setCursor(0, line1_height);
    display.print(disp_kwh_curr);
    display.setCursor(data_start_x, line1_height);
    display.print(kwh_current);
    display.setCursor(data_start_x+currdata_len, line1_height);
    display.print(" kw");
    //line 2
    display.setCursor(0, (2*line1_height)+20);
    display.print(disp_kwh_today);
    display.setCursor(data_start_x, (2*line1_height)+20);
    display.print(kwh_today);
    display.setCursor(data_start_x+currdata_len, (2*line1_height)+20);
    display.print(" kwh");
    //line 3
    display.setCursor(0, 3*line1_height+40);
    display.print(disp_status);
    display.setCursor(status_x, 3*line1_height+40);
    display.print(statusstr);
    //line 4
    display.setCursor(0, 4*line1_height+60);
    display.print(timestr);
  }
  while (display.nextPage());

  display.hibernate();

}
