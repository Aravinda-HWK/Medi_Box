//Name          : H.W.K.Aravinda
//Index No      : 200045U
//EN2853: Embedded Systems and Applications
//Programming Assignment 1


//Importing the libraries to work
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

//Defining the orientation
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

//Definging the pin numbers
#define BUZZER 5
#define LED_1 15
#define LED_2 2
#define LED_3 4
#define PB_CANCEL 34
#define PB_OK 32
#define PB_UP 33
#define PB_DOWN 35
#define DHTPIN 12

//NTP server defining
#define NTP_SERVER     "pool.ntp.org"
int UTC_OFFSET=0;
int UTC_OFFSET_DST=0;

//DHT11 sensor initialization
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHTesp dhtSensor;

//Declare variables
int days=0;
int hours=0;
int minutes=0;
int seconds=0;
int month=0;
int year=0;

bool alarm_enabled=false;
int n_alarms=3;
int alarm_hours[]={0,0,0};
int alarm_minutes[]={0,0,0};
bool alarm_triggered[]={false,false,false};

int C=262;
int D=294;
int E=330;
int F=349;
int A=440;
int B=494;
int C_H=523;
int notes[]={C,D,E,F,A,B,C_H};
int n_notes=8;

int current_mode=0;
int max_modes=5;
String modes[]={"1 - Set Time","2 - Set Alarm 1","3 - Set Alarm 2","4 - Set Alarm 3","5 - Disable Alarm"};


void setup() {
  /*
  put your setup code here, to run once:
  */
  pinMode(BUZZER,OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);

  dhtSensor.setup(DHTPIN,DHTesp::DHT22);

  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)){
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.display();
  delay(2000);

  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to WIFI",0,0,2);
  }

  display.clearDisplay();
  print_line("Connected to WIFI",0,0,2);

  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();

  print_line("Welcome to Medibox!",10, 18, 2);
  display.clearDisplay();
}

void loop() {
  /*
  put your main code here, to run repeatedly:
  */
  update_time_with_check_alarm();  
  delay(750);

  if (digitalRead(PB_OK)==LOW){
    delay(200);
    go_to_menu();
  }
  check_temp();
}

void print_line(String text, int column, int row, int text_size){
  /*
  The all print statements are defining here for
  given testsize,text colour and the position
  */
  display.setTextSize(text_size);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column,row);
  display.println(text);
  display.display();
}

void print_time_now(void){
  /*
  Printing the current time the format as 00:00:00 
  and print the date format as 05/04/2023
  */
  display.clearDisplay();  
  print_line(String(hours),0,0,2);
  print_line(":",20,0,2);
  print_line(String(minutes),30,0,2);
  print_line(":",50,0,2);
  print_line(String(seconds),60,0,2);
  print_line(String(month),0,20,2);
  print_line("/",20,20,2);
  print_line(String(days),30,20,2);
  print_line("/",50,20,2);
  print_line(String(year),60,20,2);
}

void update_time(void){
  /*
  Finding the GMT time using timeinfo
  */
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  char timeHour[3];
  strftime(timeHour,3,"%H",&timeinfo);
  hours=atoi(timeHour);

  char timeMinute[3];
  strftime(timeMinute,3,"%M",&timeinfo);
  minutes=atoi(timeMinute);

  char timeSecond[3];
  strftime(timeSecond,3,"%S",&timeinfo);
  seconds=atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3,"%d",&timeinfo);
  days=atoi(timeDay);

  char timeMonth[3];
  strftime(timeMonth,3,"%m",&timeinfo);
  month=atoi(timeMonth);

  char timeYear[5];
  strftime(timeYear,5,"%Y",&timeinfo);
  year=atoi(timeYear);
};

void ring_alarm(void){
  /*
  The all functions related to ringing the alarm.
  1. print Medicine Time! on the screen
  2. Ring the buzzer
  3. Light up the Red LED
  */
  display.clearDisplay();
  print_line("MEDICINE TIME!",0,0,2);

  //Output High to the LED
  digitalWrite(LED_1, HIGH);

  bool break_happened=false;

  //Ring the buzzer
  while (digitalRead(PB_CANCEL)==HIGH && break_happened==false){
    for (int i=0;i<n_notes;i++){
      if (digitalRead(PB_CANCEL)==LOW){
        delay(200);
        break_happened=true;
        break;
      }
      tone(BUZZER,notes[i]);
      delay(500);
      noTone(BUZZER);
      delay(2);
    }
  }
  

  digitalWrite(LED_1, LOW);
  display.clearDisplay();
}

void update_time_with_check_alarm(void){
  /*
  Check the alarm times and if the time is equal to the
  alarm time call the ring_alarm() function.
  */
  update_time();
  print_time_now();

  if (alarm_enabled==true){
    for (int i=0;i<n_alarms;i++){
      if (alarm_triggered[i]==false && alarm_hours[i]==hours && alarm_minutes[i]==minutes){
        ring_alarm();
        alarm_triggered[i]=true;
      }
    }
  }
}

int wait_for_button_press(void){
  /*
  The functions related to the button pressed.
  */
  while (true){
    if (digitalRead(PB_UP)==LOW){
      delay(200);
      return PB_UP;
    }

    else if (digitalRead(PB_DOWN)==LOW){
      delay(200);
      return PB_DOWN;
    }

    else if (digitalRead(PB_OK)==LOW){
      delay(200);
      return PB_OK;
    }

    else if (digitalRead(PB_CANCEL)==LOW){
      delay(200);
      return PB_CANCEL;
    }

    update_time();
  }
}

void go_to_menu(){
  /*
  All functions related to menu section  
  */
  while (digitalRead(PB_CANCEL)==HIGH){
    display.clearDisplay();
    print_line(modes[current_mode],0,0,2);

    int pressed=wait_for_button_press();
    if (pressed==PB_UP){
      delay(200);
      current_mode+=1;
      current_mode%=max_modes;
    }

    else if (pressed==PB_DOWN){
      delay(200);
      current_mode-=1;
      if (current_mode<0){
        current_mode=max_modes-1;
      }
    }

    else if (pressed==PB_OK){
      delay(200);
      run_mode(current_mode);
    }

    else if (pressed==PB_CANCEL){
      delay(200);
      break;
    }
  }
}

void set_time(){
  /*
  Set time zone by taking the offset from UTC as input.
  The minutes are set as 00,15,30,45 because of the global time zone.
  */
  int temp_hour=0;
  while (true){
    display.clearDisplay();
    print_line("Type UTC offset hours: "+String(temp_hour),0,0,2);

    int pressed=wait_for_button_press();
      if (pressed==PB_UP){
        delay(200);
        temp_hour+=1;
        if (temp_hour>14){
          temp_hour=-12;
        }
      }

      else if (pressed==PB_DOWN){
        delay(200);
        temp_hour-=1;
        if (temp_hour<-12){
          temp_hour=14;
        }
      }

      else if (pressed==PB_OK){
        delay(200);
        UTC_OFFSET+=(temp_hour*3600);
        break;
      }

      else if (pressed==PB_CANCEL){
        delay(200);
        break;
      }

    }
  
  int temp_minutes=0;
  while (true){
    display.clearDisplay();
    print_line("Type UTC offset minutes: "+String(temp_minutes),0,0,2);

    int pressed=wait_for_button_press();
      if (pressed==PB_UP){
        delay(200);
        temp_minutes+=15;
        temp_minutes%=60;
      }

      else if (pressed==PB_DOWN){
        delay(200);
        temp_minutes-=15;
        if (temp_minutes<0){
          temp_minutes=45;
        }
      }

      else if (pressed==PB_OK){
        delay(200);
        UTC_OFFSET+=(temp_minutes*60);
        break;
      }

      else if (pressed==PB_CANCEL){
        delay(200);
        break;
      }

    }
    configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);

  display.clearDisplay();
  print_line("Time is set",0,0,2);
  delay(1000);

}

void set_alarm(int alarm){
  /*
  The function related to setting the alarm using
  given input time as hours and minutes
  */
   alarm_enabled=true;
   int temp_hour=alarm_hours[alarm];
   while (true){
    display.clearDisplay();
    print_line("Enter hour: "+String(temp_hour),0,0,2);

    int pressed=wait_for_button_press();
      if (pressed==PB_UP){
        delay(200);
        temp_hour+=1;
        temp_hour%=24;
      }

      else if (pressed==PB_DOWN){
        delay(200);
        temp_hour-=1;
        if (temp_hour<0){
          temp_hour=23;
        }
      }

      else if (pressed==PB_OK){
        delay(200);
        alarm_hours[alarm]=temp_hour;
        break;
      }

      else if (pressed==PB_CANCEL){
        delay(200);
        break;
      }

    }
  
  int temp_minutes=alarm_minutes[alarm];
  while (true){
    display.clearDisplay();
    print_line("Enter minutes: "+String(temp_minutes),0,0,2);

    int pressed=wait_for_button_press();
      if (pressed==PB_UP){
        delay(200);
        temp_minutes+=1;
        temp_minutes%=60;
      }

      else if (pressed==PB_DOWN){
        delay(200);
        temp_minutes-=1;
        if (temp_minutes<0){
          temp_minutes=59;
        }
      }

      else if (pressed==PB_OK){
        delay(200);
        alarm_minutes[alarm]=temp_minutes;
        break;
      }

      else if (pressed==PB_CANCEL){
        delay(200);
        break;
      }

    }

  display.clearDisplay();
  print_line("Alarm is set",0,0,2);
  delay(1000);
}

void run_mode(int mode){
  /*
  Do the current mode actions
  */
  if (mode==0){
    set_time();
  }

  else if (mode==1||mode==2||mode==3){
    set_alarm(mode-1);
  }

  else if (mode==4){
    alarm_enabled=false;
  }
}

void check_temp(){
  /*
  Checking the temperature and humidity continuously
  if the temperature is not in the range Blue LED will light up
  and if the humidity is not in the given range green LED will light up
  */
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature>32){
    display.clearDisplay();
    print_line("TEMP HIGH",0,40,1);
    digitalWrite(LED_2, HIGH);
    delay(200);
  }

  else if (data.temperature<26){
    display.clearDisplay();
    print_line("TEMP LOW",0,40,1);
    digitalWrite(LED_2, HIGH);
    delay(200);
  }
  else{
    digitalWrite(LED_2, LOW);
  }

  if (data.humidity>80){
    display.clearDisplay();
    print_line("HUMIDITY HIGH",0,50,1);
    digitalWrite(LED_3, HIGH);
    delay(200);
  }

  else if (data.humidity<60){
    display.clearDisplay();
    print_line("HUMIDITY LOW",0,50,1);
    digitalWrite(LED_3, HIGH);
    delay(200);
  }
  else{
    digitalWrite(LED_3, LOW);
  }
}