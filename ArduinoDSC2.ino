/*
  Arduino based Digital Setting Circle

 
  Copyright 2020 Michael Fulbright <mike.fulbright at pobox.com>


    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// constants won't change. They're used here to 
// set pin numbers:

//#define DEBUG

const int AZ_enc_A = 6;  // digital pin 6 = PD6
const int AZ_enc_B = 7;  // digital pin 7 = PD7

const int ALT_enc_A = 9;   // digital pin 9 = PB1
const int ALT_enc_B = 10;  // digital pin 10 = PB2

int RES_ALT = 4000;  // resolution of encoders
int RES_AZ = 4000;

byte beenAligned = 0;  

long ALT_pos = RES_ALT/2;  // initial position of encoders is 
long AZ_pos = RES_AZ/2;   // half their resolution

void setup() {

  // initialize the encoder inputs
  pinMode(ALT_enc_A, INPUT);
  pinMode(ALT_enc_B, INPUT);
  pinMode(AZ_enc_A, INPUT);
  pinMode(AZ_enc_B, INPUT);

  Serial.begin(9600); 

  // Pin change interrupt control register - enables interrupt vectors
  // Bit 2 = enable PC vector 2 (PCINT23..16)
  // Bit 1 = enable PC vector 1 (PCINT14..8)
  // Bit 0 = enable PC vector 0 (PCINT7..0)
  PCICR |= (1 << PCIE2);
  PCICR |= (1 << PCIE0);

  // Pin change mask registers decide which pins are enabled as triggers
  PCMSK2 |= (1 << PCINT23);  // arduino pin 7 = PD7 = az encoder A
  PCMSK0 |= (1 << PCINT1);   // arduino pin 9 = PB1 = alt encoder A 

  // enable interrupts
  interrupts();
}

byte readByte() {
  int nloop=0;
   while (!Serial.available())
  {
    delay(50);
    
    if (nloop++ > 20)
      return 0;
  }
  
  return Serial.read(); 
}

void readLine(char *p, int maxlen) {
  int i=0;
  char *s = p;
  
  do
  {
    *s = readByte();

    if (!*s)
       return;
       
    if (*s == '\r')
       break; 
       
    s++;
  } while (i++ < maxlen-1);
  
  *s = '\0';
}

void loop(){


  // see if command sent and reply if appropriate
  //

  byte inchar;
  char cmd[80];


  // throw away rest of command - we don't need it
  //Serial.flush();

  inchar = readByte();

  if (inchar == 'Q')
  {
    printEncoderValue(AZ_pos);
    Serial.print("\t");
    printEncoderValue(ALT_pos);
    Serial.print("\r");
  }
  else if (inchar == 'R' || inchar == 'Z' || inchar == 'I')
  {
    // read to a <CR>
    readLine(cmd, 79);

#ifdef DEBUG    
    Serial.println(cmd);
#endif

    if (inchar == 'R' || inchar == 'I') {
      parseSetResolutionCmd(cmd);
      Serial.print("R");
    }
    else if (inchar == 'Z')
    {
      parseSetResolutionCmd(cmd);
      Serial.print("*"); 
    }

  }
  else if (inchar == 'z')
  {
      parseEkSetResolutionCmd();      
  }
  else if (inchar == 'r' || inchar == 'H') 
  {
    // print out resolution 
    printEncoderValue(RES_AZ);
    Serial.print("\t");
    printEncoderValue(RES_ALT);
    Serial.print("\r");    
  }
  else if (inchar == 'V')
  {
    //version
    Serial.print("V1.0.0\r");
  }
  else if (inchar == 'T')
  {
    printEncoderValue(RES_AZ);
    Serial.print("\t");
    printEncoderValue(RES_ALT);
    Serial.print("\t00000\r");
  }
  else if (inchar == 'q')
  {
    // error count
    Serial.print("00000\r");
  }
  else if (inchar == 'P')
  {
    // encoder power up
    Serial.print("P");
  }
  else if (inchar == 'p')
  {
    // 
    // dave eks error command
    Serial.print("00");
  } 
  else if (inchar == 'h')
  {
    // report resolution in Dave Eks format
    printHexEncoderValue(RES_ALT);
    printHexEncoderValue(RES_AZ);    
  }
  else if (inchar == 'y')
  {
    // report encoders in Dave Eks format
    printHexEncoderValue(ALT_pos);
    printHexEncoderValue(AZ_pos);
  }  
  else if (inchar == 'a')
  {
    if (beenAligned)
      Serial.print("Y");
    else
      Serial.print("N");
  }
  else if (inchar == 'A')
  {
    beenAligned = 1;
  }

#ifdef DEBUG
  Serial.print(digitalRead(AZ_enc_A));
  Serial.print(" ");
  Serial.print(digitalRead(AZ_enc_B));
  Serial.print(" ");
  Serial.print(digitalRead(ALT_enc_A));
  Serial.print(" ");
  Serial.print(digitalRead(ALT_enc_B));
  Serial.println();

  //return;

  Serial.print(AZ_pos);
  Serial.print(" ");
  Serial.print(ALT_pos);
  Serial.println();

  delay(500);
#endif


}


// print encoder value with leading zeros
void printEncoderValue(long val)
{
  unsigned long aval; 

  if (val < 0)
    Serial.print("-");
  else
    Serial.print("+");

  aval = abs(val);

  if (aval < 10)
    Serial.print("0000");
  else if (aval < 100)
    Serial.print("000");
  else if (aval < 1000)
    Serial.print("00");
  else if (aval < 10000) 
    Serial.print("0");

  Serial.print(aval);  
}

void printHexEncoderValue(long val)
{
  byte low, high;

  high = val / 256;

  low = val - high*256;

  Serial.write(low);
  Serial.write(high);

  return;
}

/* string is of format #<tab>#<cr> */
void parseSetResolutionCmd(char *cmd)
{
  char *p;
  int altres, azres;

  for (p=cmd; *p && *p != '\t'; p++);

  /* hit tab */
  if (*p) {
    *p = '\0';

    azres = atoi(cmd);
    altres = atoi(p+1);

#ifdef DEBUG
    Serial.print(azres);
    Serial.print(" ");
    Serial.print(altres);
    Serial.println();    
#endif

    RES_ALT = altres;
    RES_AZ = azres;
  }
}

void parseEkSetResolutionCmd()
{
  byte b1 = readByte();
  byte b2 = readByte();
  RES_ALT = b2*256+b1;

  b1 = readByte();
  b2 = readByte();
  RES_AZ = b2*256+b1;
  
//#ifdef DEBUG
    Serial.print(RES_ALT);
    Serial.print(" ");
    Serial.print(RES_AZ);
    Serial.println();    
//#endif  
}

// we have to write our own interrupt vector handler..
ISR(PCINT2_vect){ 

  if (digitalRead(AZ_enc_A) == digitalRead(AZ_enc_B))
  {
    AZ_pos++;
    if (AZ_pos >= RES_AZ)
      AZ_pos = 0;
  }
  else
  {
    AZ_pos--;

    if (AZ_pos < 0)
      AZ_pos = RES_AZ - 1;
  } 
}


ISR(PCINT0_vect){

  if (digitalRead(ALT_enc_A) == digitalRead(ALT_enc_B))
  {
    ALT_pos++;

    if (ALT_pos >= RES_ALT)
      ALT_pos = 0;
  }
  else
  {
    ALT_pos--;

    if (ALT_pos < 0)
      ALT_pos = RES_ALT - 1;
  }

}
