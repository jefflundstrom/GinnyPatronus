#include <WiFi.h>
#include <WebServer.h>
#include <time.h>
#include <AutoConnect.h>

#define LED_PIN 13 // Change to the GPIO pin you're using
#define FADE_TIME 3000 // Total time for one fade in/out cycle in milliseconds
#define PI 3.14159265


WebServer Server;
AutoConnect       Portal(Server);
AutoConnectConfig Config;       // Enable autoReconnect supported on v0.9.4
AutoConnectAux    Timezone;
static const char *wd[7] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };
char dateTime[40];
time_t  t= time(NULL);
struct tm *tm;
int LEDVALUE = 0;
void rootPage() {
   String content =
    "<html>"
    "<head>"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
    "<script  type=\"text/javascript\">"
    "function toggleLED() {"
    "  var xhr = new XMLHttpRequest();"
    "  xhr.open('GET', '/toggleLED', true);"
    "  xhr.send();"
    "}"
    "function setSchedule() {"
    "  var xhr = new XMLHttpRequest();"
    "  var onTime = document.getElementById('onTime').value;"
    "  var offTime = document.getElementById('offTime').value;"
    "  xhr.open('GET', '/setSchedule?on=' + onTime + '&off=' + offTime, true);"
    "  xhr.send();"
    "}"
    "</script>"
     "<style>"
        "body {"
            "background-color: black;"
            "text-align: center;"
            "color: white;"
        "}"
      "</style>"
      "<Title>Ginny Weasley's Patronus</Title>"
    "</head>"
    "<body>"
    "<center><img src=\"https://th.bing.com/th/id/OIP.SbXooHPcEGXqwVazyV7-rgHaFD?rs=1&pid=ImgDetMain\"><center>"
    "<p>The words Expecto Patronum literally translate to \"I expect a guardian,\" in Latin. The patronus is also a reminder of hope and love as it comes in the form of the wizard or witch's spirit guardian.</p>"
    "<p><a href = \"https://en.wikipedia.org/wiki/Ginny_Weasley\" style=\"color: lightgray;\"><h1>Ginny Weasley's</h1></a> Patronus is a Horse. The Horse Patronus symbolizes Ginny's strong will, independence, and free spirit, much like a wild horse. Horses, throughout history and in many cultures, have been symbolic of power, grace, beauty, nobility, strength, and freedom. All of these attributes match Ginny's character as she grows from a shy young girl into a confident, brave, and key player in the fight against Voldemort.<p>"
    "<center><iframe loading=\"lazy\" width=\"560\" height=\"315\" src=\"https://www.youtube.com/embed/eG6ujqx_Mu0\" title=\"YouTube video player\" frameborder=\"0\" allow=\"accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture; web-share\" allowfullscreen></iframe></center>"
    "<h3 align=\"center\" style=\"color:gray;margin:10px;\">{{DateTime}}</h3>"
    "<p style=\"text-align:center;\">Reload the page to update the time.</p>"
    "<button onclick=\"toggleLED()\">Toggle LED</button>"
    "<p>Set On Time: <input type='time' id='onTime'></p>"
    "<p>Set Off Time: <input type='time' id='offTime'></p>"
    "<button onclick=\"setSchedule()\">Set Schedule</button>"
    "<p style=\"padding-top:15px;text-align:center\">" AUTOCONNECT_LINK(COG_24) "</p>"

    "</body>"
    "</html>";
    t= time(NULL);
    tm = localtime(&t);
    sprintf(dateTime, "%02d/%02d/%04d(%s) %02d:%02d:%02d.",
      tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
      wd[tm->tm_wday],
      tm->tm_hour, tm->tm_min, tm->tm_sec);
    content.replace("{{DateTime}}", String(dateTime));
    Server.send(200, "text/html", content);
}

int baseBrightness = 0;
void performPatronus()
{

  Serial.println("Performing Patronus");

  for(int i = 0; i < 260; i += 20)
  {
    baseBrightness = i;
    unsigned long startTime = millis();
    while (millis() - startTime < FADE_TIME) {
      float time = (millis() - startTime) / (float)FADE_TIME;
      float brightness = (sin(time * 2 * PI - PI/2) + 1) / 2; // Ranges from 0 to 1

      analogWrite(LED_PIN, min(255, (int)(brightness * 255) + baseBrightness));
      delay(10);
    }
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  
  pinMode(LED_PIN, OUTPUT);

  // Enable saved past credential by autoReconnect option,
  // even once it is disconnected.
  Config.autoReconnect = true;
  Portal.config(Config);

  // Behavior a root path of ESP8266WebServer.
  Server.on("/", rootPage);

  // Handler for toggling the LED
  Server.on("/toggleLED", []() {

      Serial.println("Button clicked");

      //If we are going on, we need to perform the Patronus.
      if(LEDVALUE == 0)
      {
        performPatronus();
        LEDVALUE = 255;
      }
      else
      {
        LEDVALUE = 0;
      }

      Server.send(200, "text/plain", "Toggled");
  });

  // Handler for setting the schedule
  Server.on("/setSchedule", []() {
    // Retrieve on and off times
    Serial.println("Schedule Button clicked");
    String onTime = Server.arg("on");
    String offTime = Server.arg("off");

    Serial.println(onTime);
    Serial.println(offTime);

    // Add logic to store and handle these times
    Server.send(200, "text/plain", "Schedule Set");
  });

  // Establish a connection with an autoReconnect option.
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }

   // Retrieve the value of AutoConnectElement with arg function of WebServer class.
  // Values are accessible with the element name.
  configTime(-5 * 3600, 0, "north-america.pool.ntp.org");
}


void loop() {
  Portal.handleClient();
  
  analogWrite(LED_PIN, LEDVALUE);

  t= time(NULL);
  tm = localtime(&t);

  //On the hour we will perform the patronus.
  if(tm->tm_year > 100)
  {
    if(tm->tm_min == 0)
    {
     Serial.println("TOP OF THE HOUR");
     sprintf(dateTime, "%02d/%02d/%04d(%s) %02d:%02d:%02d.",
     tm->tm_mon + 1, tm->tm_mday, tm->tm_year + 1900,
     wd[tm->tm_wday],
     tm->tm_hour, tm->tm_min, tm->tm_sec);
     Serial.println(dateTime);
     performPatronus();
    }
  }
}
