/*
  piRefly
  Kuramoto Modell via GPIO Broadcasts on the Arduino UNO and Raspberry Pi by Nico Maas
  Ported from RF12B and ATtiny84 ( https://github.com/davruet/1000fireflies )
  Original Code by David Allan Rueter
*/

// Include wiringPi
#include <stdio.h>
#include <wiringPi.h>
// Include for OK_LED Pin remapping
#include <fcntl.h>
// Include for Ethernet LED Pins
#include <usb.h>

#define max(a,b) ((a)>(b)?(a):(b))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

typedef enum { false, true } boolean;

struct filter {
  unsigned long int current;
  unsigned long int sum;
  unsigned long int filtered;
};

int fd;

int OK_LED = 16;
int PWR_LED = 0;
boolean negateOK_LED = false;
boolean modelLAN = true;
int GPIO_LED = 4;
int IO[] = { 18, 23, 24, 25 };

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

// RPi Model Analyzer 
int model, rev, mem, maker, overVolted ;

// RPi Ethernet LED Pins
#define USB_CTRL_SET_TIMEOUT    5000
#define NET_VENDOR_ID		0x0424
#define	NET_PRODUCT_ID		0xec00

void write_config(usb_dev_handle *devh, unsigned int value)
{
	int ret;
	ret = usb_control_msg(devh, 0x40, 0xA0, 0, 0x24, (char*)&value, 4, 5000);
}

struct usb_device *get_usb_device(struct usb_bus *bus, struct usb_device *usbdev)
{
	usb_init();
	usb_find_busses();
	usb_find_devices();
	for (bus = usb_busses; bus; bus = bus->next)
	{
		for (usbdev = bus->devices; usbdev; usbdev = usbdev->next)
		{
			if ((usbdev->descriptor.idVendor == NET_VENDOR_ID) &&
			(usbdev->descriptor.idProduct == NET_PRODUCT_ID))
			{
				return(usbdev);
			}
		}
	}
}

void ethernet_led(int cmd_choose)
{
  if (modelLAN)
  { // Only works on RPi with LAN Chip - Ignore Model A, A+ and Compute Modul
  	struct usb_bus *bus;
  	struct usb_device *usbdev;
  	struct usb_device *lan9512;
  	usb_dev_handle *devh;
  
  	lan9512 = get_usb_device(bus,usbdev);
  	devh = usb_open(lan9512);
    
    // Aus, An, Auto als CMD Argument 0 1 2
    char* cmds[4] = { "w00000007", "w00000070", "w01110000" };
    char* str = cmds[cmd_choose];  
    int cmd = strtol(str+1,NULL,16); 
  	write_config(devh,cmd);
    usb_close(devh);
  }
}

void ethernet_led_off()
{
  ethernet_led(0);
}

void ethernet_led_on()
{
  ethernet_led(1);
}

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
      pinMode(IO[i], INPUT);
      pullUpDnControl(IO[i], PUD_UP);   
    }
  }
}

void broadcast(){
  broadcast_on();
  delay(5);
  broadcast_off();
}

int main ()
{
  wiringPiSetupGpio() ;
  piBoardId (&model, &rev, &mem, &maker, &overVolted);

  if (model > 2)
  { // RPi Model A+ or B+ or other 
    OK_LED = 47;
    PWR_LED = 35;
    negateOK_LED = true;
  }
  else 
  { // RPi Model A or B 
    OK_LED = 16;
    PWR_LED = 0;
    negateOK_LED = false;  
  }
  
  if ((model==1) || (model==4) || (model==5)){
    modelLAN = false;
  }
  else
  {
    modelLAN = true;
  }

  // Change the trigger on the OK/Act LED to "none"  
  if ((fd = open("/sys/class/leds/led0/trigger", O_RDWR)) < 0)
  {
    //fprintf (stderr, "Unable to change LED trigger: %s\n", strerror (errno)) ;
    return 1 ;
  }
  write (fd, "none\n", 5) ;
  close (fd) ;
    
  for( i = 0; i < 4; i++ )
  {
    pinMode(IO[i], INPUT);
    pullUpDnControl(IO[i], PUD_UP);
  }
  
  pinMode(GPIO_LED, OUTPUT);
  pinMode(OK_LED, OUTPUT);
  if (negateOK_LED) { pinMode(PWR_LED, OUTPUT); } 
  
  ledTriggerTime = micros();

  while (true)
  {  
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
      digitalWrite(OK_LED, negateOK_LED);
      if (negateOK_LED) { digitalWrite(PWR_LED, negateOK_LED); }
      ethernet_led_on();
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
      digitalWrite(OK_LED, !negateOK_LED);
      if (negateOK_LED) { digitalWrite(PWR_LED, !negateOK_LED); }
      ethernet_led_off();
    }  
  }
}





