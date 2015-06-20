/*
  piRefly
  Kuramoto Modell via GPIO Broadcasts on the Arduino UNO and Raspberry Pi by Nico Maas
  Ported from RF12B and ATtiny84 ( https://github.com/davruet/1000fireflies )
  Original Code by David Allan Rueter
*/

/*
#define max(a,b) ((a)>(b)?(a):(b))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
*/
//typedef enum { false, true } boolean;

struct filter {
  unsigned long int current;
  unsigned long int sum;
  unsigned long int filtered;
};

int OK_LED = 12;
int GPIO_LED = 13;
int IO[] = { 4, 5, 6, 7 };

#define INITIAL_SYNC_PERIOD 1000000 // Duration in microseconds of the blink cycle
#define LED_DURATION 70000 // Duration in microseconds of the LED blink
#define PERIOD_TOLERANCE (INITIAL_SYNC_PERIOD / 10)
#define PHASE_SHIFT_FACTOR 10 // Strength coefficient of adjustment made when receiving other signals
#define PERIOD_CHANGE_FACTOR 40
//#define SEND_FAIL_LIMIT 20
//#define ALONE_PERIOD 300000
#define MAX_CORRECTION (INITIAL_SYNC_PERIOD / 5)
unsigned long int period = INITIAL_SYNC_PERIOD;
unsigned long ledTriggerTime = 0;
//unsigned long lastReportTime = 0;
int consecutiveSendFailures = 0;

int i = 0;
boolean ledOn = false;
boolean Receive = false;
int receivedIO = 0;
struct filter peers;
//struct filter coherence;

unsigned long getPhase(){
    unsigned long now = micros();
    if (now < ledTriggerTime) return 0;
    unsigned long phase = now - ledTriggerTime;
    return phase;
}

void updateFilter(struct filter *value, unsigned int sampleCount){
  value->sum += value->current - value->filtered;
  value->filtered = value->sum / sampleCount;
  value->current = 0;
}

void broadcast_on(){
  for( i = 0; i < 4; i++ )
  {
    if (i!=receivedIO)
    {
      pinMode(IO[i], OUTPUT);
      digitalWrite(IO[i], LOW);    
    }
  }
}

void broadcast_off(){
  for( i = 0; i < 4; i++ )
  {
    if (i!=receivedIO)
    {
      pinMode(IO[i], INPUT_PULLUP);  
    }
  }
}

void broadcast(){
  broadcast_on();
  delay(5);
  broadcast_off();
}

void setup() {
  for( i = 0; i < 4; i++ )
  {
    pinMode(IO[i], INPUT_PULLUP);
  }
  
  pinMode(GPIO_LED, OUTPUT);
  pinMode(OK_LED, OUTPUT);
  
  ledTriggerTime = micros();
}


void loop() {
    for( i = 0; i < 4; i++ )
    {
      if (digitalRead(IO[i])==LOW)
      {
        Receive=true;
        receivedIO=i;  
      }
    }
    /*if (micros() - lastReportTime > 5000000) period = ALONE_PERIOD;
     else period = SYNC_PERIOD;
    */
    if (Receive) 
    {
      //printf ("LOW!\n"); 
      broadcast();
      //p("pulse %i avg %i val %i\n ", irDiff, irFilteredValue, val);
      unsigned long timediff = micros() - ledTriggerTime;
      long adjustment;
      if (timediff < period / 2)
      { // we are too early. 
        adjustment = (long)timediff;
        //p("early %li +- %li\n", timediff, adjustment);
      } 
      else 
      { // we are too late
        adjustment = ((long)timediff - (long)period);
        //p("late %li +- %li\n", timediff, adjustment);
      }
      
      peers.current++;
      //coherence.current += abs(adjustment);
  
      adjustment /= PHASE_SHIFT_FACTOR; 
      adjustment /= max(1, (int)peers.filtered); // scale the adjustment by a shift factor weighted by peer count
      adjustment = constrain(adjustment, -MAX_CORRECTION, MAX_CORRECTION);
      
      ledTriggerTime += adjustment;  
  
      period += adjustment / PERIOD_CHANGE_FACTOR;
      period = constrain(period, INITIAL_SYNC_PERIOD - PERIOD_TOLERANCE, INITIAL_SYNC_PERIOD + PERIOD_TOLERANCE);
  
      Receive = false;
      receivedIO = -1;
    }
    
    unsigned long phase = getPhase();
    
    if (!ledOn && phase > period)
    {
      // phase is done.
      // if (consecutiveSendFailures < SEND_FAIL_LIMIT) wdt_reset(); // reset the watchdog only when we turn on the led and reset the timer.
      ledOn = true;
      digitalWrite(GPIO_LED, HIGH);
      digitalWrite(OK_LED, HIGH);
      ledTriggerTime = micros();
      broadcast();
      phase = getPhase();     
      //reset the peer count
      updateFilter(&peers, 10);
      //updateFilter(&coherence, 3);
      //unsigned int normalizedCoherence = (unsigned int)(coherence.filtered / (1 + peers.filtered * 1000)); 
    } 
  
    if (ledOn && phase > LED_DURATION)
    {
      ledOn = false;
      digitalWrite(GPIO_LED, LOW);
      digitalWrite(OK_LED, LOW);
    }  
  }

