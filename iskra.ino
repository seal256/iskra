
// dummy analog phone interface
// be sure to update with appropriate frequency (1 Hz is good enough)
class phoneReader
{
  const int nPhonePin_ = 5; // phone pin
  const int nVoltageTh_ = 500; // threshold for signal from phone
  const int nTimeTh_ = 100; // [ms] time delay impossible between two counts of one number transmission 
  int nLastTime_ = 0; // [ms] time when last signal transmission finished
  bool bState_ = false; // high or low
  int nResult_ = -1; // number received from phone 
  float fVoltage_ = 0; // current averaged voltage
  //int nDebugCounter_ = 0;
  //const int nDebugFreq_ = 100;
  
public:
  // use in main loop with small delay
  // when transmission from phone is finished, will return result
  // all other time will return -1
  int readPinValue() 
  {
    int nCurrTime = millis();
    if(nCurrTime < nLastTime_) // time overload happend
      return clean();

    int nVal = analogRead(nPhonePin_);
    fVoltage_ = float(nVal) * 0.2 + fVoltage_ * 0.8; // smooth update of voltage

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


phoneReader Phone;

void setup()
{               
  Serial.begin(9600);  //Initialize serial for debugging
  Serial.println("ISKRA 1.0");

}


void loop() 
{

  int nPhoneNum = Phone.readPinValue(); // try to get phone output
  if(nPhoneNum >= 0)
  {
    Serial.print("result: ");      
    Serial.println(nPhoneNum);
  }
  delay(1);

}
