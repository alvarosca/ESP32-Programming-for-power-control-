#include <WiFi.h>

#define PWM_CHANNEL 4
#define PWM_FREQUENCY 1
#define PWM_RESOUTION 16
#define GPIOPIN 4
#define INIT_DUTY_CYCLE 0

// CURRENT AND VOLTAGE
// MEASUREMENT PORTS
#define CURRENT_PORT 34
#define VOLTAGE_PORT 35

#define CURRENT_REP_0A 1450
#define CURRENT_REP_1A 1512

#define VOLTAGE_REP_0V 1883
#define VOLTAGE_REP_Neg325V 1038
#define VOLTAGE_REP_325V 2694

#define ALPHA 0.001

const char* ssid = "IMEON-IMEON-88911122050112";
const char* password = "BonjourImeon";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Decode HTTP GET value
String valueString = String(5);
int pos1 = 0;
int pos2 = 0;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

hw_timer_t * timer0 = NULL;
hw_timer_t * timer1 = NULL;

int sampling_counter = 0;
int duty_cycle = 0;

float m_current = 0;
float m_voltage = 0;
float m_power = 0;

float instructed_power = 0;
float mean_power = 0;

int sample_flag = 0;

//----------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  //Peripherics configuration
  pwm_setup();
  adc_setup();
  wifi_setup();

  // STARTS WIFI INF. EXCH.
  //    timer0_setup();

  // STARTS MEASURING POWER
  timer1_setup();

}

//----------------------------------------------------------------

void loop() {

  check_wifi();
  if (sample_flag == 1) {
    m_voltage=read_voltage();
//    m_voltage = analogRead(VOLTAGE_PORT) * 3.3 / 4095;
    m_current = read_current();
    m_power = m_current * m_voltage;
    mean_power += ALPHA * m_power + (1 - ALPHA) * mean_power;
    //    Serial.print("Power[W]:");
    //    Serial.print(m_power);
    //    Serial.print(",");
    //    Serial.print("Mean Power[W]:");
    //    Serial.print(mean_power);
    //    Serial.print(",");
    Serial.print("Current[A]:");
    Serial.print(m_current);
    Serial.print(",");
    Serial.print("Voltage[V]:");
    Serial.println(m_voltage);

    // Updates DC every 1s
    //    if ( sampling_counter==1000 ){
    //        sampling_counter=0;
    //        duty_cycle = compute_dc( duty_cycle, instructed_power, mean_power  );
    //        set_duty_cycle( duty_cycle );
    //    }
    //    else{
    //        sampling_counter++;
    //    }
    sample_flag = 0;
  }

  //  DEBUGGING FUNCTIONS    c
  //    current_calibration();
  //    m_current=analogRead(CURRENT_PORT);
  //    Serial.print(",");
  //    Serial.print("Current[A]:");
  //    Serial.println(m_current);

  //    while(Serial.available()==0){}
  //    duty_cycle=Serial.parseInt();
  //    set_duty_cycle(duty_cycle);

}

//--------------------- PWM Configuration -------------------------

// pwm configuration functions

//
void pwm_setup() {
  ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
  ledcAttachPin(GPIOPIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, INIT_DUTY_CYCLE);
}

// Use this function to set the duty cycle
void set_duty_cycle(int dc) {
  dc = dc * 65535 / 100;
  //    Serial.print("Setting DC:\n");
  //    Serial.println(dc);
  ledcWrite(PWM_CHANNEL, dc);
}

//---------------------- ADC Configuration ------------------------

//
void adc_setup() {
  // Accurate readings upto 2.5v
  analogSetPinAttenuation(CURRENT_PORT, ADC_11db);
  analogSetPinAttenuation(VOLTAGE_PORT, ADC_11db);

}

//-------------------- Wifi Connection Setup ----------------------

int wifi_setup() {

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();


  return 0;
}

//------------------------ Power measurement --------------------------

// We use timer 1 to sample voltage and
// current at a constant rate

// Timer 1 configuration
void IRAM_ATTR timer1_handler() {
  sample_adc();
}
void timer1_setup() {
  timer1 = timerBegin(1, 80, true);
  timerAttachInterrupt(timer1, &timer1_handler, true);
  timerAlarmWrite(timer1, 1000, true);
  timerAlarmEnable(timer1);
}

//---------------------------------------------------------------------------

// We need to measure or compute the values of:
//    CURRENT_REP_0A    &   CURRENT_REP_1A

// Use this function to read the current measurement

float read_current() {
  float current = analogRead(CURRENT_PORT);
  return (current - CURRENT_REP_0A) / (CURRENT_REP_1A - CURRENT_REP_0A);
}

//---------------------------------------------------------------------------

// We need to measure or compute the values of:
//    VOLTAGE_REP_0V    &   VOLTAGE_REP_230V

// Use this function to read the voltage measurement

float read_voltage() {
  float voltage = analogRead(VOLTAGE_PORT);
  return 325 * ( voltage - VOLTAGE_REP_0V ) / ( VOLTAGE_REP_325V - VOLTAGE_REP_0V );
}

//---------------------------------------------------------------------------

// Samples power every 1ms
void sample_adc() {
  sample_flag = 1;
}

//--------------------------------------------------------------------------------------

int compute_dc( int duty_cycle, float instructed_power, float mean_power  ) {
  if ( instructed_power == 0 ) {
    return 0;
  }

  if (instructed_power > mean_power )
    return duty_cycle + 25 * ( instructed_power - mean_power ) / instructed_power;
  else
    return duty_cycle + 25 * ( instructed_power - mean_power ) / mean_power;
}

//--------------------------------------------------------------------------------------

// Function for WiFi communication tests

void check_wifi() {

  WiFiClient client = server.available();   // Listen for incoming clients
  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    //      Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //          Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto;}");
            client.println(".slider { width: 300px; }</style>");
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");

            // Web Page
            client.println("</head><body><h1>WIFI power control</h1>");
            client.println("<p>Duty Cycle: <span id=\"servoPos\"></span></p>");
            client.println("<input type=\"range\" min=\"0\" max=\"100\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\"" + valueString + "\"/>");

            client.println("<script>var slider = document.getElementById(\"servoSlider\");");
            client.println("var servoP = document.getElementById(\"servoPos\"); servoP.innerHTML = slider.value;");
            client.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }");
            client.println("$.ajaxSetup({timeout:1000}); function servo(pos) { ");
            client.println("$.get(\"/?value=\" + pos + \"&\"); {Connection: close};}</script>");

            client.println("</body></html>");

            //GET /?value=180& HTTP/1.1
            if (header.indexOf("GET /?value=") >= 0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              valueString = header.substring(pos1 + 1, pos2);
              set_duty_cycle(valueString.toInt());

              //                Serial.println(valueString);
            }
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    //      Serial.println("Client disconnected.");
    //      Serial.println("");
  }

}

//--------------------------------------------------------------------------------------


// Calibration

float current_calibration() {
  float average_i_rep = 0;

  for (int i = 0; i <= 10000; i++) {
    average_i_rep += analogRead(CURRENT_PORT);
  }

  Serial.print("Current Bit Representation:");
  Serial.print(average_i_rep / 10000);

  return average_i_rep;
}

float voltage_calibration() {
  float average_v_rep = 0;

  for (int i = 0; i <= 1000; i++) {
    average_v_rep += analogRead(VOLTAGE_PORT);
  }

  Serial.print("Voltage Bit Representation:");
  Serial.println(average_v_rep / 1000);

  return average_v_rep;
}
