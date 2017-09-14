#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// I2C pins for display (fixed!)
// SCL pin 5
// SDA pin 4
LiquidCrystal_I2C lcd(0x3F, 16, 2);

// analog pins
const int PHONE_PIN = 3; // phone input 

// digital pins
const int PUMP_PIN = 9; // pump transistor 


// dummy analog phone interface
// be sure to update with appropriate frequency (1 Hz is good enough)
class CPhoneReader
{
  const int nPhonePin_ = PHONE_PIN; // phone pin
  const int nVoltageTh_ = 600; // threshold for signal from phone
  const long long int nTimeTh_ = 100; // [ms] time delay impossible between two counts of one number transmission 
  long long int nLastTime_ = 0; // [ms] time when last signal transmission finished
  bool bState_ = false; // high or low
  int nResult_ = -1; // number received from phone 
  float fVoltage_ = 0; // current averaged voltage
  //int nDebugCounter_ = 0;
  //const int nDebugFreq_ = 100;
  long long int nLastPrintTime_ = 0;
  
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
    fVoltage_ = float(nVal);// * 0.3 + fVoltage_ * 0.7; // smooth update of voltage

    //if(++nDebugCounter_ % nDebugFreq_ == 0)
    //{
    //  nDebugCounter_ = 0;
    if(nCurrTime - nLastPrintTime_ > 10)
    {
      Serial.print((int)nCurrTime);
      Serial.print('\t');
      Serial.println(fVoltage_);
      nLastPrintTime_ = nCurrTime;
    }
    //}
    bool bNewState = fVoltage_ > nVoltageTh_;
    //if (bNewState) 
    //{
      //lcd.clear();
      //lcd.setCursor(0,0);
      //lcd.print(fVoltage_);      
      //lcd.setCursor(0,1);
      //lcd.print(bNewState);      
    //}
    
    if(!bNewState and bState_) // 1 became 0
    {
      //if(nResult_ == -1)
      //  nResult_ = 1;
      //else
        ++nResult_;
      //lcd.setCursor(0, 1);
      //lcd.print(nResult_);
        
      //if(nResult_ == 10) // zero is coded by ten counts + first long one
      //  nResult_ = 0;  
      
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

  int readPinValueDebug() 
  {
    long long int nCurrTime = millis();
    if(nCurrTime < nLastTime_) // time overload happend
      return clean();

    int nVal = analogRead(nPhonePin_);
    lcd.setCursor(0,0);
    lcd.clear();
    lcd.print(nVal);
    delay(100);
    return -1;
  }

};

// store game state and question answers
class CGameState
{
  int nState_ = 0; // number of current question
  const int nQuestions_ = 16; // total number of questions
  const int pAnswers_[16] = {1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917, 1828, 1917};

public:
  void init()
  {
  }
  bool checkAnswer(int nAnswer)
  {
    return (nAnswer == pAnswers_[nState_]); 
  }
  // choose new random state
  void newState()
  {
    nState_ = millis() % nQuestions_;
  }
  // light up leds to display game state
  void displayState()
  {
    lcd.setCursor(0,1);
    lcd.print(nState_);
  }
  
};


// LCD display
class CDisplay
{
  
public:
  void init()
  {
    // initialize the LCD
    lcd.begin();

    // Turn on the blacklight and print a message.
    lcd.backlight();
    lcd.print("ISKRA 2017");
    delay(500);
    lcd.clear();
    lcd.backlight();
  }

  void debug()
  {
  }
  
  void displayBlink(int nTimes)
  {
  }

  void clearDisplay()
  {
    lcd.clear();
    lcd.backlight();
  }

  // display nDigit in position nPos
  // nDigit == 10 means black screen
  void changeDigitAtPos(int nDigit, int nPos)
  {
      lcd.setCursor(nPos, 0);
      lcd.print(nDigit);
  }

  void printQuestion()
  {    
  }
private:
    
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
  Serial.begin(9600);  //Initialize serial for debugging
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

void debug()
{
  int nDigit = phoneReader.readPinValue(); // try to get phone output
  //delay(1);
  
  //if(bGot) //nDigit >= 0)
  if(nDigit >= 0)
  {
    nAnswer = 10 * nAnswer + nDigit;
    //digitDisplay.changeDigitAtPos(nDigit, 3-nDigitsObtained);
    digitDisplay.changeDigitAtPos(nDigit, 0);
    ++nDigitsObtained;
  }
}

void play()
{
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
  //delay(1);
  
  //if(bGot) //nDigit >= 0)
  if(nDigit >= 0)
  {
    nAnswer = 10 * nAnswer + nDigit;
    //digitDisplay.changeDigitAtPos(nDigit, 3-nDigitsObtained);
    digitDisplay.changeDigitAtPos(nDigit, 0);
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


void loop() 
{  
  debug();
}
