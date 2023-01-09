
// ON-GOING PROJECT.
//
// WIFI FUNCTIONALITY NOT YET IMPLEMENTED
//
// PWM POWER CONTROL AND INTERRUPTION 
// CONTROLS IMPLEMENTED AND TESTED
//


#include <WiFi.h>

#define PWM_CHANNEL 2
#define PWM_FREQUENCY 1 
#define PWM_RESOUTION 16
#define GPIOPIN 2
#define INIT_DUTY_CYCLE 0

// CURRENT AND VOLTAGE
// MEASUREMENT PORTS
#define CURRENT_PORT 34
#define VOLTAGE_PORT 35

#define CURRENT_REP_0A 0
#define CURRENT_REP_1A 1

#define VOLTAGE_REP_0V 0
#define VOLTAGE_REP_230V 1


const char* ssid = "IMEON-IMEON-88911122050112";
const char* password = "BonjourImeon";

hw_timer_t * timer0 = NULL;
hw_timer_t * timer1 = NULL;

int sampling_counter=0;
int duty_cycle=0;
float vrms=0;
float irms=0;

float m_current=0;
float m_voltage=0;

float instructed_power=0;
float mean_power=0;

//----------------------------------------------------------------

void setup(){
    Serial.begin(115200);
    delay(1000);

    pwm_setup();
    adc_setup();

    //Peripherics configuration
     
//    wifi_setup();

    // STARTS WIFI INF. EXCH.
//    timer0_setup();

    // STARTS MEASURING POWER
//    timer1_setup();

}

//----------------------------------------------------------------

void loop(){

//  FINAL VERSION SHOULD SLEEP ON
//  THE MAIN LOOP TO SAVE POWER

//    sleep();


//  DEBUGGING FUNCTIONS

    current_calibration();

//    while(Serial.available()==0){}
//    duty_cycle=Serial.parseInt();
//    set_duty_cycle(duty_cycle);      
      
//    current_r=read_current();
//    Serial.println("Variable_1:");
//    Serial.println(current_r);
    
//    for(int i=1; i<=100; i++){
//      set_duty_cycle(i);
//    delay(500);
//    }

}

//--------------------- PWM Configuration -------------------------

// pwm configuration functions

// 
void pwm_setup(){
    ledcSetup(PWM_CHANNEL, PWM_FREQUENCY, PWM_RESOUTION);
    ledcAttachPin(GPIOPIN, PWM_CHANNEL);
    ledcWrite(PWM_CHANNEL, INIT_DUTY_CYCLE);
}

// Use this function to set the duty cycle
void set_duty_cycle(int dc){
    dc=dc*65535/100;
    Serial.print("Setting DC:");
    Serial.println(dc);    
    ledcWrite(PWM_CHANNEL, dc);
}

//---------------------- ADC Configuration ------------------------

// 
void adc_setup(){
    // Accurate readings upto 2.5v    
    analogSetPinAttenuation(CURRENT_PORT, ADC_11db);
    analogSetPinAttenuation(VOLTAGE_PORT, ADC_11db);

}

//-------------------- Wifi Connection Setup ----------------------

// TO BE COMPLETED. SHOULD CONNECT AUTOMATICALLY TO THE SOLAR
// ONDULATOR AND RECEIVE THE POWER INSTRUCTIONS.

// 
int wifi_setup(){

    // THIS FUNCTION SHOULD SETUP AN
    // AUTOMATIC CONNECTION WITH THE O.S.

    // TO BE COMPLETED
  
//    WiFi.mode(WIFI_STA); //Optional
//    WiFi.begin(ssid, password);
//    Serial.println("\nConnecting");
//
//    while(WiFi.status() != WL_CONNECTED){
//        Serial.print(".");
//        delay(100);
//    }
//
//    Serial.println("\nConnected to the WiFi network");
//    Serial.print("Local ESP32 IP: ");
//    Serial.println(WiFi.localIP());

    return 0;
}

//----------------------------------------------------------------------


// We use a timer interrupt of to exchange 
// information via wifi every 10s 

// Configuration of the timer and its interrupt

void IRAM_ATTR timer0_handler(){ 
    // When the interrupt is activated the
    // power instruction should be requested
    receive_inst();    
}
void timer0_setup(){
    timer0 = timerBegin(0, 800, true);
    timerAttachInterrupt(timer0, &timer0_handler, true);
    timerAlarmWrite(timer0, 1000000, true);
    timerAlarmEnable(timer0);
}

//----------------------------------------------------------------------

// Receive instruction (and send status)

int receive_inst(){
//    Serial.println("Receiving Instruction");  

    // TO BE COMPLETED
    // CONFIGURE THIS FUNCTION FOR THE 
    // INFORMATION EXCHANGE WITH THE O.S.
    
    return 0;
}

//------------------------ Power measurement --------------------------

// We use timer 1 to sample voltage and 
// current at a constant rate

// Timer 1 configuration
void IRAM_ATTR timer1_handler(){ 
    sample_adc();    
}
void timer1_setup(){
    timer1 = timerBegin(1, 80, true);
    timerAttachInterrupt(timer1, &timer1_handler, true);                
    timerAlarmWrite(timer1, 1000, true);                                
    timerAlarmEnable(timer1);                                           
}                                                                       
                                                                        
//---------------------------------------------------------------------------

// We need to measure or compute the values of:
//    CURRENT_REP_0A    &   CURRENT_REP_1A

// Use this function to read the current measurement

float read_current(){
    float current=analogRead(CURRENT_PORT);
    return (current-CURRENT_REP_0A)/(CURRENT_REP_1A-CURRENT_REP_0A);
}

//---------------------------------------------------------------------------

// We need to measure or compute the values of:
//    VOLTAGE_REP_0V    &   VOLTAGE_REP_230V

// Use this function to read the voltage measurement

float read_voltage(){
    float voltage=analogRead(VOLTAGE_PORT);
    return ( voltage - VOLTAGE_REP_0V )/( VOLTAGE_REP_230V-VOLTAGE_REP_0V );
}

//---------------------------------------------------------------------------

// Samples power every 1ms
int sample_adc(){

    m_current = read_current();
    irms += m_current*m_current/1000; 

    m_voltage = read_voltage();
    vrms += m_voltage*m_voltage/1000; 

    mean_power += m_current*m_voltage/1000;

    // Updates DC every 1s
    if ( sampling_counter==1000 ){
        sampling_counter=0;
        duty_cycle = compute_dc( duty_cycle, instructed_power, mean_power  );            
        set_duty_cycle( duty_cycle );

        // SAVE STATUS: Power, Voltage, Current, Duty Cycle
        // INCOMPLETE

        irms=0;
        vrms=0;
        mean_power=0;
    }
    else{
        sampling_counter++;
    }
    
    return 0;
}

//--------------------------------------------------------------------------------------

int compute_dc( int duty_cycle, float instructed_power, float mean_power  ){    
    if( instructed_power == 0 ){
        return 0;
    }

    if (instructed_power > mean_power )
        return duty_cycle + 25 * ( instructed_power - mean_power ) / instructed_power;
    else
        return duty_cycle + 25 * ( instructed_power - mean_power ) / mean_power;
}

//--------------------------------------------------------------------------------------

// Calibration

float current_calibration(){
    float average_i_rep=0;
    
    for(int i=0; i<=1000; i++){
        average_i_rep+=analogRead(CURRENT_PORT)/1000;  
    }

    Serial.print("Current Bit Representation:");
    Serial.println(average_i_rep);    

    return average_i_rep;
}

float voltage_calibration(){
    float average_v_rep=0;
    
    for(int i=0; i<=1000; i++){
        average_v_rep+=analogRead(VOLTAGE_PORT)/1000;  
    }

    Serial.print("Voltage Bit Representation:");
    Serial.println(average_v_rep);    

    return average_v_rep;
}
