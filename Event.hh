#ifndef __EVENT_HH
#define __EVENT_HH

#include <iostream>
#include <vector>
#include <math.h>
#include "TObject.h"

using namespace std;

class event : public TObject { 
public:
  event() {};
  ~event() {};
  
  Int_t GetMult(){return mult;}
  Int_t GetID(int i){return ID[i];}
  Double_t GetE(int i){return E[i];}
  
  void SetMult(Int_t m){mult = m;}
  void SetID(Int_t i){ID.push_back(i);} 
  void SetE(Double_t e){E.push_back(e);} 
  
  void Clear(Option_t* opt = ""){
    mult = 0;    
    ID.clear();	 
    E.clear();	 
  }
  
protected:
  vector<Int_t> ID;
  vector<Double_t> E;
  Int_t mult;

  ClassDef(event,1)

};

#endif
