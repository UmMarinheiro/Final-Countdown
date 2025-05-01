#include <IRremote.hpp>
#include <Adafruit_NeoPixel.h>

// Pin definitions
#define IR_RECEIVE_PIN 3

#define STRIP0 13   
#define STRIP1 11
#define STRIP2 7
#define STRIP3 5

#define BUZZER_PIN 9

// Number of Leds in each segment
#define SEGMENT_LED_NUM 1

// Controller Buttons Readings
//Real
#define PLAYb 64

#define UPb 70
#define LEFTb 68
#define DOWNb 21
#define RIGHTb 67

#define ZEROb 82
#define ONEb 22
#define TWOb 25
#define THREEb 13
#define FOURb 12
#define FIVEb 24
#define SIXb 94
#define SEVENb 8
#define EIGHTb 28
#define NINEb 90

#define HASHb 74
#define ASTERISKb 66

#define NO_IR_INPUT 0

//Simulated
/*#define PLAYb 5

#define UPb 1
#define LEFTb 4
#define DOWNb 9
#define RIGHTb 6

#define ZEROb 12
#define ONEb 16
#define TWOb 17
#define THREEb 18
#define FOURb 20
#define FIVEb 21
#define SIXb 22
#define SEVENb 24
#define EIGHTb 25
#define NINEb 26

#define HASHb 8
#define ASTERISKb 10

#define NO_IR_INPUT 0*/

// Color values
#define GREEN 0, 255, 0
#define RED 255, 0, 0
#define GREEN 0, 255, 0
#define BLUE 0, 0, 255
#define YELLOW 255, 255, 0

// Time control
#define IR_INPUT_DELAY 200
#define BLINK_TIME 500
#define INCREMENT_TIME 15
#define BEEP_TIME 150

// Buzzer
#define BEEP_FREQUENCY 100

// -- Variables --
Adafruit_NeoPixel strip[4] =
{
 	Adafruit_NeoPixel(7*SEGMENT_LED_NUM, STRIP0, NEO_GRB + NEO_KHZ800),
	Adafruit_NeoPixel(7*SEGMENT_LED_NUM, STRIP1, NEO_GRB + NEO_KHZ800),
	Adafruit_NeoPixel(7*SEGMENT_LED_NUM, STRIP2, NEO_GRB + NEO_KHZ800),
	Adafruit_NeoPixel(7*SEGMENT_LED_NUM, STRIP3, NEO_GRB + NEO_KHZ800)
};

const byte segment_patterns[][7] = {
  {1, 1, 1, 1, 1, 1, 0}, // 0
  {0, 0, 0, 1, 1, 0, 0}, // 1
  {1, 0, 1, 1, 0, 1, 1}, // 2
  {0, 0, 1, 1, 1, 1, 1}, // 3
  {0, 1, 0, 1, 1, 0, 1}, // 4
  {0, 1, 1, 0, 1, 1, 1}, // 5
  {1, 1, 1, 0, 1, 1, 1}, // 6
  {0, 0, 1, 1, 1, 0, 0}, // 7
  {1, 1, 1, 1, 1, 1, 1}, // 8
  {0, 1, 1, 1, 1, 1, 1}, // 9
  
  {0, 0, 0, 0, 0, 0, 1}, // -
  {0, 0, 0, 0, 0, 1, 0}, // _
  
  {0, 0, 0, 0, 0, 0, 0} // nothing
};
unsigned long update_time = 0;
unsigned long ir_update_time = 0;
unsigned long paused_offset = 0;
unsigned long buzzer_time = 0;

char countdown_time_input[] = {0,5,0,0};
char countdown_time[] = {0,0,0,0};

bool isPaused = false;
bool isBlinking = false;
bool isBeeping = false;

char selected_strip = 0;

bool overtime = false;

// -- Execution Flow Control Declatations -- 
void start_menu();
void update_menu();

void start_timer();
void resume_timer();
void update_timer();

void start_321();
void update_321();

void (*update)();

// -- Hardware-Software Integration -- 
int read_infrared()
{
  if (!IrReceiver.decode()) return NO_IR_INPUT;    
  
  if(millis() < ir_update_time) 
  {
    IrReceiver.resume();
    return NO_IR_INPUT;
  }
  ir_update_time = millis() + IR_INPUT_DELAY;
  
  IrReceiver.printIRResultShort(&Serial); 
  IrReceiver.resume();
  Serial.println(IrReceiver.decodedIRData.command);
  return (int) IrReceiver.decodedIRData.command;
}

void display_number(int number, int position,
        char r, char g, char b) 
{
  strip[position].clear();

  if(number == 10) Serial.print("-");
  else if(number == 11) Serial.print("_");
  else if(number == 12) Serial.print(" ");
  else Serial.print(number);
    
  for (int segment = 0; segment < 7; segment++) 
  {
    if(segment_patterns[number][segment] == 0) continue; 

    for (int led = 0; led < SEGMENT_LED_NUM; led++) 
    {
      strip[position].setPixelColor(
        SEGMENT_LED_NUM * segment + led, 
        strip[position].Color(r, b, g));
    }
  }
  strip[position].show();
}

// -- Common Utility Functions -- 
char increase_countdown_absolute(char *countdown, int position)
{
  if(position < 0) return 0;
  if(position > 3) return 1;
  
  if(countdown[position] < ((position == 2)? 5 : 9)) 
  {
    countdown[position]++;
  	return 1;
  }
  countdown[position] = 0;
  return increase_countdown_absolute(countdown, position - 1);
}

char decrease_countdown_absolute(char *countdown, int position)
{
  if(position < 0) return 0;
  if(position > 3) return 1;
  
  if(countdown[position] > 0)
  {
    countdown[position]--;
    return 1;
  }
  countdown[position] = (position == 2)? 5 : 9;
  return decrease_countdown_absolute(countdown, position - 1);
}

void increase_countdown(char *countdown, int position)
{
  if(!overtime) increase_countdown_absolute(countdown, position);
  else 
  {
    if(!decrease_countdown_absolute(countdown, position))
    {
      overtime = false;
      increase_countdown_absolute(countdown, position);
      increase_countdown_absolute(countdown, position);
    }
  }
}
void decrease_countdown(char *countdown, int position)
{
  if(!overtime)
  {
    if (!decrease_countdown_absolute(countdown, position))
    {
      overtime = true;
      increase_countdown_absolute(countdown, position);
      increase_countdown_absolute(countdown, position);
    }
  }
  else increase_countdown_absolute(countdown, position);
}

int get_seconds(char *countdown)
{
  return (countdown[3] * 1) +
         (countdown[2] * 10) +
    	 (countdown[1] * 60) +
    	 (countdown[0] * 600);
}

// -- Timer Utility Functions --
void add_seconds(int sec)
{
  for(int i = 0; i < sec; i++) increase_countdown(countdown_time,3);
}

void remove_seconds(int sec)
{
  for(int i = 0; i < sec; i++) decrease_countdown(countdown_time,3);
}

void pause_timer()
{
  paused_offset = update_time - millis();
  isPaused = true;
  isBeeping = false;
  print_countdown();
}

void print_countdown()
{
  
  float proportion = (get_seconds(countdown_time) / 
                     (float) get_seconds(countdown_time_input));
  
  float red = 255 * (1 - proportion);
  float green = 255 * proportion;
  
  if (overtime) 
  {
  	red = 255;
    green = 0;
  }
  else if (proportion > 1)
  {
   	red = 0;
    green = 255;
  }
  red *= !isPaused;
  green *= !isPaused;
  
  // 12 is the nothing symbol
  // 10 is the - symbol
  char first_char =
    countdown_time[0] == 0 ?
      overtime ? 10 : 12 :
    countdown_time[0];
  
  display_number(first_char,0,red,green,isPaused*255);
  for(int i = 1; i < 4; i++)
    display_number(countdown_time[i],i,red,green,isPaused*255);  
  Serial.println();
}

void reset_countdown()
{
  overtime = false;
  isPaused = true;
  for(int i = 0; i < 4; i++)
    countdown_time[i] = countdown_time_input[i];
  print_countdown();
}

void update_countdown()
{
  if(millis() < update_time) return;
  update_time += 1000;
 
  decrease_countdown(countdown_time,3);
  
  print_countdown(); 
  
  isBeeping = !overtime ?
    get_seconds(countdown_time) == 60 ||
    get_seconds(countdown_time) == 3 ||
    get_seconds(countdown_time) == 2 ||
    get_seconds(countdown_time) == 1 ||
    get_seconds(countdown_time) == 0 :
  	get_seconds(countdown_time)%60 == 0;

  if(!isBeeping) return; 
  buzzer_time = millis() + BEEP_TIME;
}

// -- Menu Utility Functions -- 
void print_countdown_menu()
{
  for(int i = 0; i < 4; i++)
  {
    // 11 is the _(underline) char
    display_number(
      (i == selected_strip && isBlinking) ? 11:
      countdown_time_input[i],i,GREEN);
  }
  Serial.println();
}

void update_blink()
{
  if(millis() < update_time) return;
  update_time += BLINK_TIME;
  
  isBlinking = !isBlinking;
  
  print_countdown_menu();
}

// -- Main execution Flow --
void setup() {
  
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  
  for(int i = 0;i<4;i++)
  {
    strip[i].begin();
  	strip[i].show();
  	strip[i].setBrightness(255); 
  }
  start_menu();
}
void loop() 
{
  if(isBeeping && (millis() > buzzer_time)) isBeeping = false; 
  analogWrite(BUZZER_PIN, BEEP_FREQUENCY*isBeeping);
  (*update)();
}


void start_menu()
{
  selected_strip = 0;
  update_time = millis();
  isBlinking = false;
  overtime = false;
  print_countdown_menu();
  isBeeping = false;
  print_countdown_menu();
  update = &update_menu;
}
void update_menu()
{
  switch(read_infrared())
  {
    case NO_IR_INPUT:
    	break;
    
    case PLAYb:
      start_timer();
    	return;
    
    case RIGHTb:
      if(selected_strip == 3) break;
        selected_strip++;
      print_countdown_menu();
      break;
    case LEFTb:
      if(selected_strip == 0) break;
      	selected_strip--;
    	print_countdown_menu();
      break;
    
    case UPb:
    	increase_countdown_absolute(countdown_time_input,selected_strip);
    	print_countdown_menu();
    	break;
    case DOWNb:
    	decrease_countdown_absolute(countdown_time_input,selected_strip);
    	print_countdown_menu();
    	break;
    
    case ZEROb:
      countdown_time_input[selected_strip] = 0; 
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;  
    case ONEb:
    	countdown_time_input[selected_strip] = 1; 
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case TWOb:
    	countdown_time_input[selected_strip] = 2;
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case THREEb:
      	countdown_time_input[selected_strip] = 3; 
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case FOURb:
      	countdown_time_input[selected_strip] = 4; 
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case FIVEb:
      	countdown_time_input[selected_strip] = 5; 
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case SIXb:
      	countdown_time_input[selected_strip] = 6;
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case SEVENb:
    	if(selected_strip == 2)break;
    	countdown_time_input[selected_strip] = 7;
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case EIGHTb:
    	if(selected_strip == 2)break;
    	countdown_time_input[selected_strip] = 8;
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    case NINEb:
    	if(selected_strip == 2)break;
    	countdown_time_input[selected_strip] = 9;
    	if(selected_strip != 3) selected_strip++;
  		print_countdown_menu();
    	break;
    
    case HASHb:
    	break;
    case ASTERISKb:
    	break;
    
   	default:
    	break;
  }
  update_blink();
}


void start_timer()
{
  reset_countdown();
  
  update_time = millis();
  if(isPaused) isBeeping = false; 
  print_countdown();
  update = &update_timer;
}
void resume_timer(char _isPaused)
{
  update_time  = millis() + paused_offset;
  isPaused = _isPaused;
  if(isPaused) isBeeping = false;
  print_countdown();
  update = &update_timer;
}

void update_timer()
{
  switch(read_infrared())
  {
    case NO_IR_INPUT:
    	break;
    
    case PLAYb:
        if(!isPaused) pause_timer();
        else 
        {
          start_321();
          return;
        }
    	break;
    case ZEROb:
      if(isPaused)resume_timer(false);
      break;
    case LEFTb:
        add_seconds(INCREMENT_TIME);
    	print_countdown();
        break;
    case RIGHTb:
        remove_seconds(INCREMENT_TIME);
     	print_countdown();
        break;
    
    case UPb:
    	add_seconds(60);
    	print_countdown();
    	break;
    case DOWNb:
    	remove_seconds(60);
    	print_countdown();
      break;
    case HASHb:
    	reset_countdown();
    	break;
    case ASTERISKb:
    	start_menu();
    	return;
    
   	default:
    	break;
  }
  
  if(!isPaused) update_countdown();
}

void start_321()
{
  update_time = millis();
  selected_strip = 1;
  isBeeping = false;
  update = &update_321;
}

void update_321()
{
  switch(read_infrared())
  {
    case NO_IR_INPUT:
    	break;
    
    case PLAYb:
      resume_timer(true);
    	return;
    case ASTERISKb:
    	start_menu();
    	return;
    
   	default:
    	break;
  }
  
  if(millis() < update_time) return;
  update_time += 1000;
  
  switch(selected_strip)
  {
  	case 1:
    	display_number(12,0,GREEN);
        display_number(3,1,GREEN);
    	display_number(12,2,GREEN);
        display_number(12,3,GREEN);
    	Serial.println();
    	break;
    
    case 2:
    	display_number(12,0,YELLOW);
        display_number(12,1,YELLOW);
    	display_number(2,2,YELLOW);
        display_number(12,3,YELLOW);
    	Serial.println();
    	break;
    	
    case 3:
    	display_number(12,0,RED);
        display_number(12,1,RED);
    	display_number(12,2,RED);
        display_number(1,3,RED);
    	Serial.println();
    	break;
    	
    default:
      isBeeping = true;
      buzzer_time = millis() + BEEP_TIME;
  		resume_timer(false);
    	return;
  }
  
  selected_strip++;
  
  isBeeping = true;
  buzzer_time = millis() + BEEP_TIME;
}
