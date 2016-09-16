// analog pins
const int PHONE_PIN = 5; // phone input 

// digital pins
const int PUMP_PIN = 13; // pump transistor 

// display digit pins
const int DIGIT1_PIN = 2; // 
const int DIGIT2_PIN = 3; // 
const int DIGIT3_PIN = 4; // 
const int DIGIT4_PIN = 5; // 

// shift register pins
const int CLOCK_PIN = 10; // Pin connected to SH_CP of 74HC595
const int LATCH_PIN = 11; // Pin connected to ST_CP of 74HC595
const int DATA_PIN = 12; // Pin connected to DS of 74HC595

// leds to dispaly internal state
const int LED1_PIN = 6; // 
const int LED2_PIN = 7; // 
const int LED3_PIN = 8; // 
const int LED4_PIN = 9; // 


// dummy analog phone interface
// be sure to update with appropriate frequency (1 Hz is good enough)
class CPhoneReader
{
  const int nPhonePin_ = PHONE_PIN; // phone pin
  const int nVoltageTh_ = 500; // threshold for signal from phone
  const long long int nTimeTh_ = 100; // [ms] time delay impossible between two counts of one number transmission 
  long long int nLastTime_ = 0; // [ms] time when last signal transmission finished
  bool bState_ = false; // high or low
  int nResult_ = -1; // number received from phone 
  float fVoltage_ = 0; // current averaged voltage
  //int nDebugCounter_ = 0;
  //const int nDebugFreq_ = 100;
  
public:
  void init()
  {
  }
  // use in main loop with small delay
  // when transmission from phone is finished, will return result
  // all other time will return -1
  int readPinValue() 
  {
    long long int nCurrTime = millis();
    if(nCurrTime < nLastTime_) // time overload happend
      return clean();

    int nVal = analogRead(nPhonePin_);
    fVoltage_ = float(nVal) * 0.3 + fVoltage_ * 0.7; // smooth update of voltage

    //if(++nDebugCounter_ % nDebugFreq_ == 0)
    //{
    //  nDebugCounter_ = 0;
    //  Serial.println(fVoltage_);
    //}
    bool bNewState = fVoltage_ > nVoltageTh_;
    
    if(!bNewState and bState_) // 1 became 0
    {
      //if(nResult_ == -1)
      //  nResult_ = 1;
      //else
        ++nResult_;
        
      if(nResult_ == 10) // zero is coded by ten counts + first long one
        nResult_ = 0;  
      
      nLastTime_ = nCurrTime; // remember time
      //Serial.print("signal:");
      //Serial.println(nLastTime_);
      //Serial.println(fVoltage_);
      //Serial.println(bState_);
      //Serial.println(nResult_);
    }
    else if(!bNewState and !bState_ and nResult_ >= 0) // possible end of counting
    {
      if(nCurrTime - nLastTime_ > nTimeTh_) // indeed
      {
        int nRet = nResult_;
        clean();
        if(nRet < 0 || nRet > 9) // that bastards were cheating with phone
          nRet = -1;
        return nRet;
      }
    }

    bState_ = bNewState;
    return -1;
  }

  int clean()
  {
    nLastTime_ = 0;
    bState_ = false; 
    nResult_ = -1;
    fVoltage_ = 0;
    return -1;
  }
  
};

// store game state and question answers
class CGameState
{
  byte nState_ = 0; // number of current question
  const int nQuestions_ = 16; // total number of questions
  const int pAnswers_[16] = {1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917};
  const int pPins_[4] = {LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN};

public:
  void init()
  {
    for(int nP = 0; nP < 4; ++nP)
      pinMode(pPins_[nP], OUTPUT);  
  }
  bool checkAnswer(int nAnswer)
  {
    return (nAnswer == pAnswers_[nState_]); 
  }
  // choose new random state
  void newState()
  {
    nState_ = (byte)(millis() % nQuestions_);
  }
  // light up leds to display game state
  void displayState()
  {
    for(int nB = 0; nB < 4; ++nB)
      digitalWrite(pPins_[nB], bitRead(nState_, nB)); 
  }
  
};


// send byte to shift register
void shiftRegisterOut(int myDataPin, int myClockPin, byte myDataOut) {
  // This shifts 8 bits out MSB first, 
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOut�
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);

    //if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {  
      pinState= 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  //stop shifting
  digitalWrite(myClockPin, 0);
}

// 4 digit display with shift regiter
class CDisplay
{
  const int latchPin_ = LATCH_PIN; //Pin connected to ST_CP of 74HC595
  const int clockPin_ = CLOCK_PIN; //Pin connected to SH_CP of 74HC595
  const int dataPin_ = DATA_PIN; // Pin connected to DS of 74HC595
  const int digitPins_[4] = {DIGIT1_PIN, DIGIT2_PIN, DIGIT3_PIN, DIGIT4_PIN};
  byte dataArray_[11]; // codes for 7 segment indicator
  int digitsToDisplay_[4]; // what digits to display now

public:
  void init()
  {
    pinMode(latchPin_, OUTPUT);
    pinMode(clockPin_, OUTPUT);
    pinMode(dataPin_, OUTPUT);
    for(int nP = 0; nP < 4; ++nP)
    {
      pinMode(digitPins_[nP], OUTPUT);
      digitsToDisplay_[nP] = 10;
    }
  }
  CDisplay()
  {
    // abcdefg
    /*
    dataArray_[0] = 0x7E; 
    dataArray_[1] = 0x30; 
    dataArray_[2] = 0x6D; 
    dataArray_[3] = 0x79; 
    dataArray_[4] = 0x33; 
    dataArray_[5] = 0x5B; 
    dataArray_[6] = 0x5F; 
    dataArray_[7] = 0x70; 
    dataArray_[8] = 0x7F; 
    dataArray_[9] = 0x7B;
    //*/
    
    dataArray_[0] = 0x3F; 
    dataArray_[1] = 0x06; 
    dataArray_[2] = 0x5B; 
    dataArray_[3] = 0x4F; 
    dataArray_[4] = 0x66; 
    dataArray_[5] = 0x6D; 
    dataArray_[6] = 0x7D; 
    dataArray_[7] = 0x07; 
    dataArray_[8] = 0x7F; 
    dataArray_[9] = 0x6F;

    dataArray_[10] = 0x00; // all off    
  }


  void debug()
  {
    //changeDigitAtPos(7, 0);
    //changeDigitAtPos(6, 1);
    //changeDigitAtPos(5, 2);
    //changeDigitAtPos(4, 3);
    
    //if(millis() % 1000 == 0)
    //{
    //}

    /*
    if(millis() - nLastTime > 1000)
    {
      for(int nP =0; nP < 4; ++nP)
      {
        changeDigitAtPos(nDigit, nP); 
      }
      ++nDigit;
      nDigit %= 11;

      nLastTime = millis();
      //bGot = true;
    }
    */
    
    //for(int nP = 0; nP < 4; ++nP)
    //{
    //  for (int nD = 0; nD < 11; ++nD) 
    //  {
    //    changeDigitAtPos(nD, nP);
    //    displayWork();
    //    delay(300);
    //  }
    //}
  }
  
  void debugPins()
  {
    for(int nP = 0; nP < 4; ++nP)
    {
      selectDigitPin(nP);
      delay(300);
      clearDigitPins();
    }
  }

  void displayBlink(int nTimes)
  {
    for(int nT = 0; nT < nTimes; ++nT)
    {
      for(int nP = 0; nP < 4; ++nP)
      {
        changeDigitAtPos(8, nP);
        displayWork();
        delay(10);
      }
      //for(int nP = 0; nP < 4; ++nP)
      //{  
      //  displayDigitAtPos(10, nP);
      //  delay(300);
      //}
    }
  }

  void clearDisplay()
  {
    for(int nP = 0; nP < 4; ++nP)
      changeDigitAtPos(10, nP);
     displayWork();
  }

  // display nVal in position nPos
  // nVal == 10 means black screen
  void changeDigitAtPos(int nDigit, int nPos)
  {
    if(nDigit > 10 || nDigit < 0)
      return;

    digitsToDisplay_[nPos] = nDigit;
  }

  // run in main cycle with high frequency
  void displayWork()
  {
    for(int nP = 0; nP < 4; ++nP)
    {    
      sendDigit(10); // clear byte in register
      selectDigitPin(nP);
      sendDigit(digitsToDisplay_[nP]);
      delay(1);
    }//clearDigitPins();    
  }

private:
  // activates pin for selected digit position
  void selectDigitPin(int nPos)
  {
    clearDigitPins();
    digitalWrite(digitPins_[nPos], 1);
  }

  void clearDigitPins()
  {
    for(int nP = 0; nP < 4; ++nP)
      digitalWrite(digitPins_[nP], 0); // set all to 0    
  }
  
  void sendDigit(int nVal)
  {
    if(nVal > 10 || nVal < 0)
      return;

    //ground latchPin and hold low for as long as you are transmitting
    digitalWrite(latchPin_, 0);
    //move 'em out
    shiftRegisterOut(dataPin_, clockPin_, dataArray_[nVal]);
    //return the latch pin high to signal chip that it 
    //no longer needs to listen for information
    digitalWrite(latchPin_, 1);
    
  }
    
};

class CPump
{
  const int pumpPin_ = PUMP_PIN;
public:
  void init()
  {
    pinMode(pumpPin_, OUTPUT);
  }
  void pourDrink()
  {
      digitalWrite(pumpPin_, 1);
      delay(2000); // adjust delay to pour nesessary amount of liquid
      digitalWrite(pumpPin_, 0);      
  }
};

// main program starts here

CPhoneReader phoneReader;
CGameState gameState;
CDisplay digitDisplay;
CPump pump;

int nDigitsObtained = 0; // how many digits we received from user
int nAnswer = 0; // what we received
long long int nTimeOfAnswerEnd = -1;
bool bAnswerCorrect = false;

void setup()
{               
  //Serial.begin(9600);  //Initialize serial for debugging
  //Serial.println("ISKRA 1.0");

  phoneReader.init();
  gameState.init();
  digitDisplay.init();
  pump.init();
  
  //digitDisplay.debug();
  //digitDisplay.changeDigitAtPos(7, 2); 
}

//bool bGot = false;
//int nDigit = -1;
//long long int nLastTime = 1;


void loop() 
{
  digitDisplay.displayWork(); // strobe display
  //return;
  
  // simulate digit from phone
  /*
  if(millis() - nLastTime > 1000)
  {
      //for(int nP =0; nP < 4; ++nP)
      //{
      //  digitDisplay.changeDigitAtPos(nDigit, nP); 
      //}
      ++nDigit;
      nDigit %= 10;

      nLastTime = millis();
      bGot = true;
  }
  //*/
  
  int nDigit = phoneReader.readPinValue(); // try to get phone output

  //if(bGot) //nDigit >= 0)
  if(nDigit >= 0)
  {
    nAnswer = 10 * nAnswer + nDigit;
    digitDisplay.changeDigitAtPos(nDigit, 3-nDigitsObtained);
    ++nDigitsObtained;
    
    //Serial.print("got digit: ");      
    //Serial.println(nDigit);
    //bGot = false;
  }
  //delay(1);

  if(nDigitsObtained == 4)
  {    
    nTimeOfAnswerEnd = millis();
    // We will wait a bit since last letter is passed
    // Otherwize user won't see the last digit
    if(gameState.checkAnswer(nAnswer)) // correct
      bAnswerCorrect = true;
      
    nDigitsObtained = 0;
  }

  if(nTimeOfAnswerEnd > 0 && millis() - nTimeOfAnswerEnd > 1000)
  {
    digitDisplay.clearDisplay();
    //delay(200);
    
    //digitDisplay.displayBlink(3);

    if(bAnswerCorrect)
    {
      // put the reward
      pump.pourDrink();

      gameState.newState();
      gameState.displayState();   
    }
    
    nTimeOfAnswerEnd = -1;
    nAnswer = 0;
    bAnswerCorrect = false;
  }
  
}
