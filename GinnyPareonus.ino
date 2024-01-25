#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <AutoConnect.h>
#include <FS.h>
#include <ArduinoJson.h> 
#include <ESPmDNS.h>

#define LED_PIN 13
#define FADE_TIME 3000
#define PI 3.14159265

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig Config("JennysWand", "olivia00");
AutoConnectAux Timezone;
static const char *wd[7] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };
char dateTime[40];
time_t t= time(NULL);
struct tm *tm;
int LEDVALUE = 0;

String onTime = "1700", offTime = "2359";
bool hourlyflash = true, useSchedule = true;

void rootPage() {
  String content =
    "<!DOCTYPE html><html><head>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Ginny Weasley's Patronus</title>"
    "<style>"
    "body{background-color:black;color:white;font-family:Arial,sans-serif;margin:0;padding:0;text-align:center;}"
    ".container{margin:auto;max-width:800px;padding:20px;}"
    ".time-input{font-size:20px;padding:0;width:200px;height:40px;}"
    "h1,h3{color:lightgray;margin:10px 0;}"
    "p{color:white;margin:10px 0;}"
    "img,iframe{max-width:100%;height:auto;}"
    "button{margin:10px;padding:10px 20px;font-size:16px;}"
    "</style>"
    "<script type='text/javascript'>"
    "function toggleLED(){var xhr=new XMLHttpRequest();xhr.open('GET','/toggleLED',true);xhr.send();}"
    "function setSchedule(){var xhr=new XMLHttpRequest();var onTime=document.getElementById('onTime').value;"
    "var offTime=document.getElementById('offTime').value;var hourlyflash=document.getElementById('patronusHourly').checked;"
    "var useSchedule=document.getElementById('useSchedule').checked;"
    "xhr.open('GET','/setSchedule?onTime='+onTime+'&offTime='+offTime+'&hourlyflash='+hourlyflash+'&useSchedule='+useSchedule,true);xhr.send();}"
    "</script>"
    "</head><body>"
    "<div class='container'>"
    "<img src='https://th.bing.com/th/id/OIP.SbXooHPcEGXqwVazyV7-rgHaFD?rs=1&pid=ImgDetMain' alt='Patronus Image'>"
    "<p>The words Expecto Patronum translate to 'I expect a guardian,' in Latin. The patronus is a reminder of hope and love as it comes in the form of the wizard or witch's spirit guardian.</p>"
    "<p><a href='https://en.wikipedia.org/wiki/Ginny_Weasley' style='color:lightgray;'><h1>Ginny Weasley's</h1></a>"
    "Patronus is a Horse. The Horse Patronus symbolizes Ginny's strong will, independence, and free spirit, much like a wild horse. Horses, throughout history and in many cultures, have been symbolic of power, grace, beauty, nobility, strength, and freedom. All of these attributes match Ginny's character as she grows from a shy young girl into a confident, brave, and key player in the fight against Voldemort.</p>"
    "<iframe width='500' style='height:282px;' frameborder='0' loading='lazy' src='https://www.youtube.com/embed/eG6ujqx_Mu0' title='YouTube video player' allow='autoplay;' allowfullscreen></iframe>"
    "<h3>{{DateTime}}</h3>"
    "<button onclick='toggleLED()'>Toggle Light</button>"
    "<p><label for='patronusHourly'>Patronus Hourly:</label>"
    "<input type='checkbox' id='patronusHourly' name='patronusHourly' {{HOURLYCHECK}}></p>"
    "<p><label for='useSchedule'>Use Scheduled Times Below:</label>"
    "<input type='checkbox' id='useSchedule' name='useSchedule' {{USESCHEDULE}}></p>"
    "<p>Set On Time: <input type='time' id='onTime' class='time-input' value='{{ONTIME}}'></p>"
    "<p>Set Off Time: <input type='time' id='offTime' class='time-input' value='{{OFFTIME}}'></p>"
    "<button onclick='setSchedule()'>Set Schedule</button>"
    "<p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p></body></html>";

  t = time(NULL);
  tm = localtime(&t);
  sprintf(dateTime, "%02d/%02d/%04d(%s) %02d:%02d:%02d.", tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900, wd[tm->tm_wday], tm->tm_hour, tm->tm_min, tm->tm_sec);
  content.replace("{{DateTime}}", String(dateTime));
  content.replace("{{HOURLYCHECK}}", hourlyflash ? "Checked" : "");
  content.replace("{{USESCHEDULE}}", useSchedule ? "Checked" : "");
  String timeToDisplay = onTime.substring(0, 2) + ":" + onTime.substring(2, 4);
  content.replace("{{ONTIME}}",  timeToDisplay);
  timeToDisplay = offTime.substring(0, 2) + ":" + offTime.substring(2, 4); 
  content.replace("{{OFFTIME}}", timeToDisplay);
  Server.send(200, "text/html", content);
}

int baseBrightness = 0;

void performPatronus() {
  Serial.println("Performing Patronus");
  for(int i = 0; i < 260; i += 20) {
    Portal.handleClient();
    baseBrightness = i;
    unsigned long startTime = millis();
    while (millis() - startTime < FADE_TIME) {
      float time = (millis() - startTime) / (float)FADE_TIME;
      float brightness = (sin(time * 2 * PI - PI/2) + 1) / 2; 
      analogWrite(LED_PIN, min(255, (int)(brightness * 255) + baseBrightness));
      delay(10);
    }
  }
}

void readAndParseJSON() {
  File file = SPIFFS.open("/mysettings.json", "r");
  if (!file) return;
  const size_t bufferSize = 1000;
  DynamicJsonDocument jsonDoc(bufferSize);
  DeserializationError error = deserializeJson(jsonDoc, file);
  if (error) return;
  String onTimeValue = jsonDoc["timeOn"];
  onTime = onTimeValue;
  String offTimeValue = jsonDoc["timeOff"];
  offTime = offTimeValue;
  hourlyflash = jsonDoc["hourly"];
  useSchedule = jsonDoc["timer"];
  file.close();
}

void saveDataToJSON() {
  StaticJsonDocument<1000> jsonDoc;
  jsonDoc["timeOn"] = onTime;
  jsonDoc["timeOff"] = offTime;
  jsonDoc["hourly"] = hourlyflash;
  jsonDoc["timer"] = useSchedule;
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  File file = SPIFFS.open("/mysettings.json", "w");
  if (file) {
    file.println(jsonString);
    file.close();
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  
  pinMode(LED_PIN, OUTPUT);
  if (SPIFFS.begin(true)) readAndParseJSON();
  Config.autoReconnect = true;
  Portal.config(Config);
  Server.on("/", rootPage);
  Server.on("/toggleLED", []() {
    if (LEDVALUE == 0) {
      performPatronus();
      LEDVALUE = 255;
    } else {
      LEDVALUE = 0;
    }
    Server.send(200, "text/plain", "Toggled");
  });
  Server.on("/setSchedule", []() {
    String newTime = Server.arg("onTime");
    if (newTime.length() >= 5 && newTime.indexOf(':') != -1)
      onTime = newTime.substring(0, 2) + newTime.substring(3);

    newTime = Server.arg("offTime");
    if (newTime.length() >= 5 && newTime.indexOf(':') != -1)
      offTime = newTime.substring(0, 2) + newTime.substring(3);

    hourlyflash = Server.arg("hourlyflash") == "true";
    useSchedule = Server.arg("useSchedule") == "true";
    saveDataToJSON();
    Server.send(200, "text/plain", "Schedule Set");
  });
  if (Portal.begin()) Serial.println("WiFi connected: " + WiFi.localIP().toString());
  configTime(-5 * 3600, 0, "north-america.pool.ntp.org");

  if (!MDNS.begin("mywand")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("MDNS responder started");
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS service added");
  }

}

void loop() {
  Portal.handleClient();
  analogWrite(LED_PIN, LEDVALUE);
  if (hourlyflash || useSchedule) {
    t = time(NULL);
    tm = localtime(&t);
    if (tm->tm_year > 100) {
      sprintf(dateTime, "%02d/%02d/%04d(%s) %02d:%02d:%02d.",
        tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
        wd[tm->tm_wday],
        tm->tm_hour, tm->tm_min, tm->tm_sec);

      //The schedule trumps the hourly.
      if (useSchedule) {
        int currentHour = tm->tm_hour;
        int currentMinute = tm->tm_min;
        int currentTime = (currentHour * 100) + currentMinute;
        int ontime = atoi(onTime.c_str());
        int offtime = atoi(offTime.c_str());
        bool changed = false;

        if (currentTime == ontime) {
          performPatronus();
          LEDVALUE = 255;
          changed = true;
        } 
        else if (currentTime == offtime) 
        {
          LEDVALUE = 0;
          changed = true;
        }

        if(changed)
        {
          analogWrite(LED_PIN, LEDVALUE);
          //Wait 60 seconds as we dont want to keep spamming the change.
          delay(60000);
        }
      }
      else if (hourlyflash && tm->tm_min == 0) {
        performPatronus();
      } 
    }
  }
}
