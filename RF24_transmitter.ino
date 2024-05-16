/*
 * 4-MAY-2024
 * ARDUINO radio transmitter
 * RF24
 * OLED 128x64 SH1106 I2C
 * X,Y,SW joystick
 */

#include <SPI.h>
#include <Wire.h>
#include <RF24.h>
#include <nRF24L01.h>
#include <U8g2lib.h>

#define RF24_CE 7
#define RF24_CSN 8

RF24 radio(RF24_CE, RF24_CSN);


U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

struct dataStruct {
  uint8_t Jx0;
  uint8_t Jy0;
  uint8_t Jx1;
  uint8_t Jy1;
  bool sw0;
  bool sw1;
  bool virt_sw0;
  bool virt_sw1;
  bool virt_sw2;
  bool virt_sw3;
  bool virt_sw4;
  bool virt_sw5;
}data;

#define MMENU_MAX 5
#define MMENU_MIN 0

#define WIND_MAX 2
#define WIND_MIN 0

uint8_t main_cursor = 0;
uint8_t main_menu_cnt = 0;
uint8_t main_window_cnt = 0;

bool menu_dwn = false; 
bool menu_up = false;

char *menu[MMENU_MAX+1] = {     
      "Inputs",
      "Channels",
      "Virtual Switches",
      "TEST0",
      "TEST1",
      "Back"       
};

char initial[20];

volatile bool ent_lea_menu = false;
volatile bool btn0 = false;

void setup() {
//  Serial.begin(115200);
  
  cli();
  EICRA = 0;
  PORTD |= (1<<2) | (1<<3);
  EICRA |= (1<<ISC11) | (1<<ISC01);
  EIMSK |= (1<<INT1) | (1<<INT0);
  sei();
  
  u8g2.begin();
  
  while(!radio.begin()){
    u8g2.firstPage();
    do{
     u8g2.setFont(u8g2_font_ncenB08_tr);
     u8g2.drawStr(58,10,":(");
     u8g2.drawStr(5,30,"Radio doesn't works!");
     u8g2.drawStr(5,50,"Check NRF24 module.");
    }while(u8g2.nextPage());
  }

  radio.setDataRate(RF24_2MBPS);
  radio.setChannel(57);
  radio.openWritingPipe(0xA1B2C34);//0xA1B2C34
  radio.setPALevel(RF24_PA_MAX);
  radio.setAutoAck(false);
  idle_pos();
}

void loop() {
  _delay_ms(5);
  uint16_t val0 = read_adc(3);
  uint16_t val1 = read_adc(2);
  uint16_t val2 = read_adc(1);
  uint16_t val3 = read_adc(0);

  data.Jx0 = ((val2 > 510) && (val2 < 514)) ? 128 : map(val2, 1023, 0, 0, 255);
  data.Jy0 = ((val3 > 501) && (val3 < 505)) ? 128 : map(val3, 1023, 0, 0, 255);
  data.Jx1 = ((val0 > 521) && (val0 < 525)) ? 128 : map(val0, 1023, 0, 0, 255);
  data.Jy1 = ((val1 > 507) && (val1 < 512)) ? 128 : map(val1, 1023, 0, 0, 255);

  data.sw0 = (PIND & (1<<5));
  data.sw1 = (PIND & (1<<6));

  if(!ent_lea_menu){
    dashboard_page(); 
  }else{

    idle_pos();    
    item_selector(MMENU_MAX, val0, &main_menu_cnt, &main_window_cnt);
    menu1("Main menu", menu, &main_cursor, &main_window_cnt);
  }

  radio.write(&data, sizeof(dataStruct));     
}

ISR(INT0_vect){
  _delay_ms(1);
  btn0 = 1;
  if(!(PIND & (1<<3))){
    ent_lea_menu = !ent_lea_menu;
  }
}

ISR(INT1_vect){
  _delay_ms(1);
  if(!(PIND & (1<<2))){
    ent_lea_menu = !ent_lea_menu;
  }
}

void idle_pos(void){
  data.Jx0 = 128;
  data.Jy0 = 128;
  data.Jx1 = 128;
  data.Jy1 = 128; 
}

void dashboard_page(void){
    u8g2.firstPage();
    do{
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.setDrawColor(1);

      u8g2.drawStr(35, 10, "Dashboard");
      u8g2.drawLine(0, 15, 128, 15);

      char tmp[15];
      
      sprintf(tmp, "X0: %d", data.Jx0);
      u8g2.drawStr(10,31, tmp);
      
      sprintf(tmp, "Y0: %d", data.Jy0);
      u8g2.drawStr(10,45, tmp);
      
      sprintf(tmp, "X1: %d", data.Jx1);
      u8g2.drawStr(70,31, tmp);
      
      sprintf(tmp, "Y1: %d", data.Jy1);
      u8g2.drawStr(70,45, tmp);

      sprintf(tmp, "sw0: %d", data.sw0);
      u8g2.drawStr(10,59, tmp);
      
      sprintf(tmp, "sw1: %d", data.sw1);
      u8g2.drawStr(70,59, tmp);
    }while(u8g2.nextPage());
} 

void item_selector(uint8_t menu_max, uint8_t val, uint8_t *menu_cnt, uint8_t *window_cnt){

    if(val > 700){
      if(*menu_cnt < menu_max){
        menu_up = true;
        menu_cnt++;
        _delay_ms(150);
      }
    }else if(val < 300){
       if(*menu_cnt > 0){
          menu_dwn = true;
          menu_cnt--;
          _delay_ms(150);
       }
    }

     if((window_cnt+2) <= menu_max){
        if((window_cnt+2) < *menu_cnt){
          window_cnt++;
        }
      }
      
     if(*window_cnt > 0){
        if(*window_cnt > *menu_cnt){
          window_cnt--;
        }
     }     
}

struct menu {
  char* title;
  char** item;
  uint8_t menu_cnt;
  uint8_t menu_max;
  uint16_t val;
  uint8_t wind_max;
  uint8_t window_cnt;
  uint8_t curs;
};

void menu1(){

      if(val > 700){
      if(menu_cnt < menu_max){
        menu_up = true;
        menu_cnt++;
        _delay_ms(150);
      }
    }else if(val < 300){
       if(menu_cnt > 0){
          menu_dwn = true;
          menu_cnt--;
          _delay_ms(150);
       }
    }

     if((window_cnt+2) <= menu_max){
        if((window_cnt+2) < menu_cnt){
          window_cnt++;
        }
      }
      
     if(window_cnt > 0){
        if(window_cnt > menu_cnt){
          window_cnt--;
        }
     }     
    u8g2.firstPage();
    do{
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.setDrawColor(1);
      u8g2.drawStr(35, 10, title);
      u8g2.drawLine(0, 15, 128, 15);

      bool it_color[3] = {1,1,1};

      if(menu_up == true){
        if(curs < WIND_MAX)
            curs++;
        menu_up = false;
      }

      if(menu_dwn == true){
        if(curs > WIND_MIN)
            curs--;
        menu_dwn = false;
      }
      
      it_color[curs] = 0;

      u8g2.drawBox(0, 20+(curs*15), 128, 12);  

      u8g2.setDrawColor(it_color[0]);      
      u8g2.drawStr(0, 30, items[window_cnt+0]);
      
      u8g2.setDrawColor(it_color[1]);
      u8g2.drawStr(0, 45, items[window_cnt+1]);
      
      u8g2.setDrawColor(it_color[2]);
      u8g2.drawStr(0, 60, items[window_cnt+2]);

    }while(u8g2.nextPage());  
}

uint16_t read_adc(uint8_t adc){
  ADMUX = 0;
  ADCSRA = 0;
  
  ADMUX |= (0<<REFS1) | (1<<REFS0) | adc;
  ADCSRA |= (1<<ADSC) | (1<<ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

  while(ADCSRA & (1<<ADSC));

  return ADCL | (ADCH<<8);
}
