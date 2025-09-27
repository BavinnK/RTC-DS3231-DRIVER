#include <util/delay.h>
#include <Arduino.h>
#define slave_addLCD    0b01001110
#define backLight       0b00001000 //third bit for display on/off
#define clk_speed 16000000
#define baud 9600
#define my_ubrr (clk_speed/16/baud-1)
#define slave_add    0b11010000
#define command_flag 0b00000000 //if RS is zero it means command 
#define read_flag    0b00000001 //else if its 1 it means we want to display data
////////////////////////////////////////////////////////////////////////////////I2C INIT
//we initilize the I2C protocol
void i2c_init(void){
  //we set the TWSR to zero the default bc the datasheet says so lol so basically we set the prescaler to zero
  TWSR=0x00;
  //after that we gonna set up the  TWBR register to 72 why? bc of this formula
  //SCL freq = 16MHz / (16 + 2 * TWBR * 1) => TWBR = 72
  TWBR=72;
  //after wee need to enable the TWI BY setting the third bit of the TWCR the third bit of that register
  TWCR=(1<<TWEN);
}
////////////////////////////////////////////////////////////////////////////////I2C start
//in here we gonna send the start command to a specific I2C device
void i2c_start(void){
  //in this line of code we want todo 3 things first we set the TWINT bit to 1  so we can make it to zero why is that ??? bc when we set that bit to 1
  //basiccally it clears the bit to zero by this the hardware says alr i will start the communication to that specific device
  //TWEA bit controls the generation of the acknowledge pulse
  //TWEN: TWI Enable Bit  //for more info look at the data sheet
  TWCR=(1<<TWSTA)|(1<<TWINT)|(1<<TWEN);
  //and here for this loop in the comments we said when its 0 "TWINT" bit it means the hardware is not finished and its sending a start to the device how do we know
  //it send it ?? by a loop
  while(!(TWCR&(1<<TWINT)));
}
////////////////////////////////////////////////////////////////////////////////I2C write
//this function sends the data via I2C protocol
void i2c_write(uint8_t data){
  TWDR=data;
  TWCR=(1<<TWINT)|(1<<TWEN);
  while(!(TWCR&(1<<TWINT)));
  
}
void i2c_stop(){
  TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
   //while(!(TWCR&(1<<TWINT)));
}
////////////////////////////////////////////////////////////////////////////////RTC write
//note every hex or binary u send u have to calculate it by BCD format or binary coded decimal when we say 0x45 for example this is the binary for it
//0b0100 0101

void RTC_register_write(uint8_t data,uint8_t reg_add){
  uint16_t slave_add1=(slave_add|command_flag);
  i2c_start();
  i2c_write(slave_add1);
  i2c_write(reg_add);
  i2c_write(data);
  i2c_stop();
}
////////////////////////////////////////////////////////////////////////////////BCD TO DEC
uint8_t BCD_to_dec(uint8_t bcd){
  //when we get the data back from the DS3231 we get BCD but we have to conver that to decimal so we can see it on the serial monitor
  //for example we get this 0b0100 0010 this in decimal means 42
  uint16_t clk_dec=((bcd>>4)*10);//first we shift it 4 times to right so we get this 0b0100 0010-->0b0000 0100 and now we multiply it by ten means 40 now

  clk_dec=clk_dec+(bcd&0x0F);//and for this part the lower nibble we use bitwise and of 0x0F means 15 in dec and 0b0000 1111 in binary so we can get the bcd to dec very easily
  return clk_dec;
}
////////////////////////////////////////////////////////////////////////////////RTC read data
//we use a pointer to register to read the data from the DS3231 first we have to send the slave add with command or write flag then specify the register add that we want to read
//then send another slave add but this time with read flag and thats it now the slave sends the data from thet register that we specified u have to look at the data sheet for more info 

uint8_t RTC_register_read(uint8_t reg){
  uint16_t slave_add1=(slave_add|read_flag);
  uint16_t slave_add2=(slave_add|command_flag);
  i2c_start();
  i2c_write(slave_add2);
  i2c_write(reg);
  i2c_start();

  ///////////////now we send another slave add buttt with read flag
  i2c_write(slave_add1);
  //now manually we have to read data from  the bus why bc now the slave puts data on the bus or  TWO WIRE DATA REGISTER
  TWCR=(1<<TWINT)|(1<<TWEN);
  while(!(TWCR&(1<<TWINT)));


  uint8_t data=TWDR;
  i2c_stop();
  return data;

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//LCD CODE
void lcd_sendNibble(uint8_t nibble,uint8_t flag){
  uint16_t lcd_byte=(nibble|flag|backLight);
  i2c_write(lcd_byte|1<<2);
  _delay_ms(5);
  i2c_write(lcd_byte);
}
void lcd_send_data(uint8_t data,uint8_t flag){
  i2c_start();
  i2c_write(slave_addLCD);
  lcd_sendNibble((data&0xF0),  flag);//sending 4 high bit or 4 most significant bit
  lcd_sendNibble((data<<4)&0xF0,  flag);//sending low 4 bit or 4 least significant bit
  i2c_stop();
}
///////////////////////////////////////////////////////////////////////////////////////////LAYER 3
void lcd_cmd(uint8_t command){
  lcd_send_data(command, command_flag);

}
void lcd_char(char character){
  lcd_send_data(character, read_flag);
}
void lcd_init(void){
  //a special wake up sequence
  uint8_t wakeUP=0x30;
  i2c_start();
  _delay_ms(40);
  i2c_write(slave_addLCD);//sending slave add
  _delay_ms(1);
  lcd_sendNibble(wakeUP,command_flag); 
  _delay_ms(1);
  lcd_sendNibble(wakeUP,command_flag); 
  _delay_ms(1);
  lcd_sendNibble(wakeUP,command_flag); 
  _delay_ms(1);
  lcd_sendNibble(wakeUP,command_flag); 
  _delay_ms(1);
  lcd_sendNibble(wakeUP,command_flag); 
  _delay_ms(5);

  //now that we woke up the chip from the lcd now we gonna do our configration look at the HD44780U (LCD-II), (Dot Matrix Liquid Crystal Display Controller/Driver) datasheet for  more info
  lcd_cmd(0b00000010);//return home add
  _delay_ms(5);
  lcd_cmd(0b00101000);//funstion set: DL=4 bit | N=2lines | F=5x8 dots
  _delay_ms(5);
  lcd_cmd(0b00001100);//display control: display on \ cursor move\ no blink
  _delay_ms(5);
  lcd_cmd(0b00000001);//clearing the disp
  _delay_ms(5);
  lcd_cmd(0b00000110);//entry mode set
  _delay_ms(5);
  i2c_stop();
}
//now we need a function to display strings on the lcd
void lcd_print_string(const char* str) {
    while (*str) {
        lcd_char(*str++);
        
    }
}//done
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
char buff[80];
void setup() {
  i2c_init();
  lcd_init();
  //Serial.begin(9600);
 // USART_init(my_ubrr);
  //again these datas are not in binary they are in BCD format 
  RTC_register_write(0b01000101,0x00);//second :45
  RTC_register_write(0b00100101,0x01);//minute : 25
  RTC_register_write(0b00010010,0x02);//hour : 12

}

void loop() {
  uint8_t sec=BCD_to_dec(RTC_register_read(0x00));
  uint8_t min=BCD_to_dec(RTC_register_read(0x01));
  uint8_t hr=BCD_to_dec(RTC_register_read(0x02));
  sprintf(buff,"Time:%d:%d:%02d",hr,min,sec);
  //Serial.println(buff);
    lcd_cmd(0b10000000);

  lcd_print_string(buff);
  delay(1000);
  


}