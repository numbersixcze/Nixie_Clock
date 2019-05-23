#include <ShiftRegister74HC595.h>
#include <Ticker.h>


// Pin Definition
const uint8_t AnalogPin     = A0;   //analogový vstup z multiplexu
const uint8_t VN_EN_Pin     = 14;   //VN enable  
const uint8_t VN_CTRL_Pin   = 12;   //VN level control
const uint8_t ExtServoPin   = 16;   //Extern Servo Control Pin
const uint8_t SCK_EIO_Pin   = 13;   //SCK to 595 to extended Outputs 
const uint8_t RCK_EIO_Pin   = 15;   //RCK to 595 to extended Outputs
const uint8_t SER_EIO_Pin   = 5;    //serial_in to 595 to extended Outputs
const uint8_t SER_NX_Pin    = 4;    //serial_in to 595 to nixie tubes
const uint8_t SCK_NX_Pin    = 0;    //SCK to 595 to nixie tubes
const uint8_t RCK_NX_Pin    = 2;    //RCK to 595 to nixie tubes

//Loop Times Setup
const uint16_t FastLoopMs = 10;     //Fast loop called thru timer interrupt every x ms
const uint16_t SlowLoopMul = 50;    //After this number is achieved slow loop is called SlowLoopMs = FastLooMs*SlowLoopMul

//Signals defined for weitching analog multilex
enum Signals {VN_U = 0, VN_I, OC_OV, NoiseAD, PWR_Status, Btn1, Btn2, UCC_U};   //Names
double RSignals[8] = {0,};                                                      //Number  storage [V] - value that reads ADC

//Instance creation
Ticker Timer;                                                           //Create instance of Timer
ShiftRegister74HC595 EIO595(1, SER_EIO_Pin, SCK_EIO_Pin, RCK_EIO_Pin);  //Create instance of shift registr for extended output
ShiftRegister74HC595 NX595(2, SER_NX_Pin, SCK_NX_Pin, RCK_NX_Pin);      //Create instance of shift registr for displaying numbers on nixies

//HEADERS
double ReadSignal(Signals MulSig);      //Reads chosen signal, you better use GetSignal()
double GetSignal(Signals Sig);          //signal getter
void SetNX(uint8_t NumLeftPair, char Dots, uint8_t NumRightPair);   //Sign Dots will blink, directly sets nixies
void BuzzerOn();
void BuzzerOff();
void StatusLedOn();
void StatusLedOff();
void ReleOn();
void ReleOff();
void ClockDots(char Dost);  //Sets dots, both, uppper, bottom or none
void SlowLoop();
void VnOn();
void VnOff();
void SerialUpdate();    //not used, only for debug. Sending some data over serial (4 pin header or usb of Wemos)

// fce 
bool StateN = 0;
void SetNX(uint8_t NumLeftPair, char Dots, uint8_t NumRightPair)
{
    uint8_t NumNXSet[2]; //[2]
    NumNXSet[0] = ((NumLeftPair % 10) << 4) | (NumLeftPair / 10);
    NumNXSet[1] = ((NumRightPair % 10) << 4) | (NumRightPair / 10);
    
    //PCB bug correction - exchanged 0 and 1
    if((NumLeftPair % 10) == 1)
        NumNXSet[0] &= ~16;
    if((NumLeftPair % 10) == 0)
        NumNXSet[0] |= 16;
    if((NumLeftPair / 10) == 1)
        NumNXSet[0] &= ~1;
    if((NumLeftPair / 10) == 0)
        NumNXSet[0] |= 1;

    if((NumRightPair % 10) == 1)
        NumNXSet[1] &= ~16;
    if((NumRightPair % 10) == 0)
        NumNXSet[1] |= 16;
    if((NumRightPair / 10) == 1)
        NumNXSet[1] &= ~1;
    if((NumRightPair / 10) == 0)
        NumNXSet[1] |= 1;

    NX595.setAll(NumNXSet); //Sends bytes
    
    //Blinking dots
    StateN = !StateN;
    if(StateN)
        {
        ClockDots(Dots);
        StatusLedOn();
        }
    else
        {
        StatusLedOff();
        ClockDots(' ');        //Sets dots
        }
}

void SerialUpdate()
{
    Serial.write(27);       // ESC command
    Serial.print("[2J");    // clear screen command
    Serial.write(27);
    Serial.print("[H");     // cursor to home command

    Serial.print("Buzzer:\t");
    Serial.print(EIO595.get(0));
    Serial.print("\tRele:\t");
    Serial.print(EIO595.get(1));
    Serial.print("\tStatus:\t");
    Serial.print(EIO595.get(2));
    Serial.print("\tLED top:");
    Serial.print(EIO595.get(3));
    Serial.println("");

    Serial.print("LED bottom:\t");
    Serial.print(EIO595.get(4));
    Serial.print("\tMul_A0:\t");
    Serial.print(EIO595.get(5));
    Serial.print("\tMul_A1:\t");
    Serial.print(EIO595.get(6));
    Serial.print("\tMul_A2:\t");
    Serial.print(EIO595.get(7));
    Serial.println("");

    Serial.print("VN_U:\t");
    Serial.print(GetSignal(VN_U));
    Serial.print("V\tVN_I:\t");
    Serial.print(GetSignal(VN_I));
    Serial.print("V\tOV_OC:\t");
    Serial.print(GetSignal(OC_OV));
    Serial.print("V\tNoise_AD:\t");
    Serial.print(GetSignal(NoiseAD));
    Serial.println("V");

    Serial.print("PWR_Status:\t");
    Serial.print(GetSignal(PWR_Status));
    Serial.print("V\tBtn_1:\t");
    Serial.print(GetSignal(Btn1));
    Serial.print("V\tBtn_2:\t");
    Serial.print(GetSignal(Btn2));
    Serial.print("V\tUCC_U:\t");
    Serial.print(GetSignal(UCC_U));
    Serial.println("V");

}
void VnOn()
{
    digitalWrite(VN_EN_Pin, 1);
}

void VnOff()
{
    digitalWrite(VN_EN_Pin, 0);
}

void ClockDots(char Dots)
{
    switch (Dots)
    {
    case '\'':
        EIO595.setNoUpdate(3,1);
        EIO595.setNoUpdate(4,0);
        EIO595.updateRegisters();
        break;

    case '.':
        EIO595.setNoUpdate(3,0);
        EIO595.setNoUpdate(4,1);
        EIO595.updateRegisters();
        break;

    case ':':
        EIO595.setNoUpdate(3,1);
        EIO595.setNoUpdate(4,1);
        EIO595.updateRegisters();
        break;
    
    default:
        EIO595.setNoUpdate(3,0);
        EIO595.setNoUpdate(4,0);
        EIO595.updateRegisters();
        break;
    }
}
double ReadSignal(Signals MulSig)
{
    //nastaveni cteneho vstupu
    EIO595.setNoUpdate(5, (uint8_t)MulSig & 1);
    EIO595.setNoUpdate(6, ((uint8_t)MulSig >> 1) & 1);
    EIO595.setNoUpdate(7, ((uint8_t)MulSig >> 2) & 1);
    EIO595.updateRegisters();
    return ((3.2/1023.0*(double)analogRead(AnalogPin))-0.0); //Convert to voltage
}

double GetSignal(Signals Sig)
{
    return RSignals[(uint8_t)Sig];
}

void BuzzerOn()
{
    EIO595.set(0,1);
}
void BuzzerOff()
{
   EIO595.set(0,0);

}

void ReleOn()
{
    EIO595.set(1,1);
}

void ReleOff()

{
    EIO595.set(1,0);
}

void StatusLedOn()
{
    EIO595.set(2,1);
}

void StatusLedOff()
{
    EIO595.set(2,0);
}

uint8_t ReadRotate = 0;
uint16_t LoopCounter = 0;
void FastLoop()
{
    //Read inputs everytime
    RSignals[ReadRotate] = ReadSignal((Signals)ReadRotate); //cte postupne vsechny vstupy
    
    ++ReadRotate;
    ReadRotate  %= 8;
    ++LoopCounter;
    if (!(LoopCounter % SlowLoopMul))
        SlowLoop();
}

void SlowLoop()
{
    //Slow loop
    
    //SetNX(Hodiny, ':', Minuty);
}

int i = 0;
void setup()
{
//Setting all I/O configs
pinMode(AnalogPin, INPUT);
pinMode(VN_EN_Pin, OUTPUT);
pinMode(VN_CTRL_Pin, OUTPUT);
pinMode(ExtServoPin, OUTPUT);
pinMode(SCK_EIO_Pin, OUTPUT);
pinMode(RCK_EIO_Pin, OUTPUT);
pinMode(SER_EIO_Pin, OUTPUT);
pinMode(SCK_NX_Pin, OUTPUT);
pinMode(RCK_NX_Pin, OUTPUT);
pinMode(SER_NX_Pin, OUTPUT);

//Begin serial comm
Serial.begin(9600);
//Sets loop
Timer.attach_ms((uint32_t)FastLoopMs, FastLoop);
//Zero everything out
EIO595.setAllLow();
NX595.setAllLow();
}

void loop()
{
if ((GetSignal(Btn2)) > 2.0 || (GetSignal(Btn1)) > 2.0)
    VnOn();
else
    VnOff();
SetNX(i*11, ':', i*11);
delay(500);
++i;
if (i > 9)
    i = 0;
}