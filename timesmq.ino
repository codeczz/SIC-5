#include <WiFi.h>
#include <HTTPClient.h>
#include <MQUnifiedsensor.h>
#include <time.h>

const char *ssid = "scawol";
const char *password = "12345678";
const char *serverName = "http://192.168.83.171:5000/submit";

#define Board ("ESP-32")
#define Type ("MQ-2") // Ganti sesuai dengan jenis sensor MQ yang digunakan
#define Voltage_Resolution (3.3) // Tegangan referensi ADC
#define ADC_Bit_Resolution (12) // Resolusi ADC ESP32 (12-bit = 4096)
#define RatioMQ2CleanAir (9.83) // Rasio MQ-2 di udara bersih
#define mqPin (34) // Pin yang terhubung ke sensor MQ

// Inisialisasi objek MQ2
MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, mqPin, Type);

// NTP Server details
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

void setup(void) {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Initialize NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Inisialisasi sensor MQ2
  MQ2.setRegressionMethod(1); // (1) => MQ2 akan menggunakan curve eksponensial
  MQ2.setA(574.25); // Parameter kurva untuk MQ-2 (udara)
  MQ2.setB(-2.222); // Parameter kurva untuk MQ-2 (udara)
  MQ2.init();
  
  Serial.print("Calibrating please wait.");
  float calcR0 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ2.update(); // Baca sensor, bisa digunakan juga untuk pembacaan data
    calcR0 += MQ2.calibrate(RatioMQ2CleanAir);
    Serial.print(".");
  }
  MQ2.setR0(calcR0 / 10);
  Serial.println(" done!.");
  
  if (isnan(MQ2.getR0())) {
    Serial.println("Failed to calibrate MQ-2");
    while (1);
  } else {
    Serial.print("Calibration completed! R0 value is: ");
    Serial.println(MQ2.getR0());
  }
}

void loop(void) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    float udara = readMQ2udara();

    if (udara != -1) {
      // Get current time
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
      }
      char timeStringBuff[50];
      strftime(timeStringBuff, sizeof(timeStringBuff), "%Y-%m-%dT%H:%M:%S%z", &timeinfo);
      String timestamp = String(timeStringBuff);

      // Construct JSON payload
      String jsonPayload = "{\"udara\":";
      jsonPayload += udara;
      jsonPayload += ", \"timestamp\":\"";
      jsonPayload += timestamp;
      jsonPayload += "\"}";

      // Send POST request
      http.begin(serverName);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(jsonPayload);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(httpResponseCode);
        Serial.println(response);
      } else {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    }

    delay(2000); // Send data every minute
  }
}

float readMQ2udara() {
  MQ2.update(); // Baca nilai sensor
  float udara = MQ2.readSensor(); // Membaca konsentrasi udara dalam ppm
  if (isnan(udara)) {
    Serial.println("Failed to read from MQ-2 sensor!");
    return -1;
  } else {
    return udara;
  }
}
