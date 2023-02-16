/*
 
                          ----  ROOT TUTORIAL FOR BEGINNERS  ----

   Author: Jaime Acosta ( jaime.acosta@csic.es )

   Description: this tutorial shows how to read and analyze data using TTree structures, 
   taking as physics case the gamma spectroscopy of some 60Co radioactive source with
   an array of HPGe semiconductor detectors.

   write_tree() takes TIMESTAMP, DETECTOR ID and E_RAW information from data.txt and
   writes a data tree with event-by-event information into 60Co.root.

   raw_histos() reads that tree and writes uncalibrated energy spectra for each detector
   into a file called histos.root.

   cal_histos() reads the raw histograms in histos.root and calculates calibration 
   parameters for each detector, and then creates calibrated histograms. It also shows 
   how to apply some conditions when filling the histograms for analyzing the data.

   draw() draws a TCanvas with two TPads, with a legend, a text and some histograms in it

 */


//C,C++ Libraries
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <algorithm> // for std::find
#include <iterator> // for std::begin, std::end
// ROOT libraries
#include <TROOT.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TTree.h>
#include <TFile.h>
#include <TF1.h>
#include <TRandom3.h>
#include <TCanvas.h>
#include <TChain.h>
#include <TColor.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TCutG.h>
#include <TApplication.h>

#include <Event.hh>

using namespace std;


Int_t Write_tree(){

  cout << "STARTING WRITE_TREE()........" << endl;
  
  // leer el fichero .txt
  ifstream ifile("data.txt");

  // crear el archivo .root de salida
  TFile *ofile = new TFile("./output/60Co.root","RECREATE");

  // creamos el tree
  TTree *tr = new TTree("tr","60Co calibration run");

  // declarar qué contiene el tree
  Int_t n_evento_temp;
  event *evento_temp = new event();

  // creamos el contenido del tree (las ramas)
  tr->Branch("evento", &evento_temp, 320000);
  tr->Branch("n_evento", &n_evento_temp, "n_evento/I");

  Long64_t ts_new;
  Long64_t ts = 999999999999;
  Int_t id;
  Double_t e;

  n_evento_temp = 1;
  evento_temp->SetMult(0);

  Long64_t i = 0;
  system("wc -l data.txt | awk '{print $1}' > nlines.txt");
  ifstream nlines("nlines.txt");
  Long64_t ntotal;
  nlines >> ntotal;
  nlines.close();
  system("rm nlines.txt");
  
  ifile.ignore(1000,'\n');
  while( ifile.good() ){
    ifile >> ts_new >> id >> e;
    //cout << "timestamp: " << ts << "\t\tID: " << id << "\t\tE_raw: " << e << endl;
    if(ts_new - ts > 1000){
      tr->Fill();
      evento_temp->Clear();
      n_evento_temp = n_evento_temp + 1;
    }

    if (i % 100000 == 0){
      std::cout << "Completion: " << i*100/ntotal << " %" << endl;
    }

    evento_temp->SetID(id);
    evento_temp->SetE(e);
    evento_temp->SetMult(evento_temp->GetMult() + 1);

    ts = ts_new;
    i++;

  }

  ifile.close();

  ofile->cd();
  tr->Write();
  ofile->Close();
  
  return 1;
}

// crea los histogramas de energía sin calibrar para cada detector
int Raw_histos(){

  cout << "STARTING RAW_HISTOS()........" << endl;
  
  // leemos el fichero .root con el tree
  TFile *ifile = new TFile("./output/60Co.root","READ");
  if (ifile == NULL) {return -1;}

  // leemos el tree
  TTree *mi_tr = (TTree*)ifile->Get("tr");
  if (mi_tr == NULL) {return -2;}

  // creamos las variables que almacenarán el contenido del tree
  event* mi_evento = new event();
  int mi_n_evento;

  mi_tr->SetBranchAddress("n_evento", &mi_n_evento);
  mi_tr->SetBranchAddress("evento", &mi_evento);

  // creamos el fichero de salida que guardará los histogramas
  TFile *ofile = new TFile("./output/histos.root","UPDATE");
  
  Long64_t n_entries = mi_tr->GetEntries();
  cout << "Número de entradas del tree: " << n_entries << endl;

  // creamos los histogramas raw
  TH1D *h_raw[47];
  for(int h=0; h<47; h++){
    h_raw[h] = new TH1D(Form("h_raw_%i",h),Form("raw energy histogram of detector %i",h),20000,0,20000);
  }
  TH2D *h_raw_summary = new TH2D("h_raw_summary","raw energy vs. ID",47,0,47,20000,0,20000);
  
  // BUCLE DE LOS EVENTOS DEL TREE
  
  //cout << "------------------------------" << endl;
  for (Long64_t i = 0; i < n_entries; i++){

    //if(i*100/n_entries % 10 == 0){
    if(i % 100000 == 0){
      cout << "Program completion: " << i*100/n_entries << "%" << endl;
    }
    // esto es solo para indicar el porcentaje de compleción
    
    mi_tr->GetEntry(i);
    for (int m=0; m < mi_evento->GetMult(); m++){
      h_raw[mi_evento->GetID(m)]->Fill(mi_evento->GetE(m));
      h_raw_summary->Fill(mi_evento->GetID(m), mi_evento->GetE(m));
    }
  }

  // escribimos los histogramas
  ofile->cd();
  for(int h=0; h<47; h++){
    h_raw[h]->Write("",TObject::kOverwrite);
  }
  h_raw_summary->Write("",TObject::kOverwrite);

  ifile->Close();
  ofile->Close();

  cout << "Finished!" << endl;
  
  return 2;
  
}

// lee los histogramas recién creados, escribe los parámetros de calibración y crea los espectros calibrados
Int_t Cal_histos(){

  cout << "STARTING CAL_HISTOS()........" << endl;
  
  // leemos el archivo .root con los histogramas
  TFile *ofile = new TFile("./output/histos.root","UPDATE");

  // establecemos los límites del fit para los dos picos del 60Co
  // para cada uno de los 47 espectros
  // xmin1, xmax1, xmin2, xmax2
  // si el espectro no existe, dejo los límites en 0
  
  Double_t lims[47][4] = {{1820, 1835, 2070, 2085},  // detector 0
			  {2090, 2105, 2375, 2390},  // detector 1
			  {0, 0, 0, 0},              // detector 2
			  {0, 0, 0, 0},              // detector 3
			  {0, 0, 0, 0},              // detector 4
			  {2500, 2515, 2840, 2860},  // detector 5
			  {2295, 2310, 2605, 2620},  // detector 6
			  {0, 0, 0, 0},              // detector 7
			  {2455, 2470, 2790, 2805},  // detector 8
			  {2250, 2265, 2555, 2570},  // detector 9
			  {2460, 2475, 2795, 2810},  // detector 10
			  {0, 0, 0, 0},              // detector 11
			  {2105, 2115, 2390, 2405},  // detector 12
			  {2246, 2260, 2550, 2565},  // detector 13
			  {2250, 2264, 2555, 2570},  // detector 14
			  {0, 0, 0, 0},              // detector 15
			  {2200, 2225, 2490, 2530},  // detector 16
			  {2330, 2360, 2650, 2680},  // detector 17
			  {2840, 2880, 3235, 3270},  // detector 18
			  {0, 0, 0, 0},              // detector 19
			  {2215, 2230, 2515, 2530},  // detector 20
			  {2230, 2245, 2530, 2545},  // detector 21
			  {2250, 2265, 2555, 2570},  // detector 22
			  {0, 0, 0, 0},              // detector 23
			  {2250, 2260, 2555, 2565},  // detector 24
			  {2200, 2210, 2500, 2510},  // detector 25
			  {2150, 2162, 2440, 2455},  // detector 26
			  {2303, 2315, 2615, 2630},  // detector 27
			  {2165, 2180, 2460, 2475},  // detector 28
			  {2200, 2230, 2500, 2530},  // detector 29
			  {2250, 2270, 2555, 2575},  // detector 30
			  {2115, 2130, 2400, 2420},  // detector 31
			  {2125, 2140, 2415, 2430},  // detector 32
			  {2125, 2140, 2410, 2430},  // detector 33
			  {2160, 2175, 2455, 2470},  // detector 34
			  {2115, 2130, 2400, 2420},  // detector 35
			  {2220, 2235, 2520, 2535},  // detector 36
			  {2130, 2144, 2420, 2435},  // detector 37
			  {2150, 2160, 2440, 2455},  // detector 38
			  {2290, 2305, 2605, 2620},  // detector 39
			  {3375, 3390, 3830, 3850},  // detector 40
			  {3700, 3850, 4250, 4350},  // detector 41
			  {3700, 3740, 4200, 4240},  // detector 42
			  {0, 0, 0, 0},              // detector 43
			  {2470, 2485, 2805, 2820},  // detector 44
			  {10050, 10200, 11400, 11550},  // detector 45
			  {2580, 2620, 2930, 2970}};  // detector 46}

  // una forma más eficiente y sofisticada de buscar los picos es con la clase TSpectrum
  
  /* TSpectrum *sp = new TSpectrum (npeaks, resolution);
     Int_t nfound = sp->Search(histogram, resolution, "nobackground", threshold);
     Double_t *xpeaks = sp->GetPositionX();

     // this is to sort the peaks according to their energy, not their intensity
     Int_t ind[2];
     TMath::Sort(2,xpeaks,ind,kFALSE);
     Double_t xpeaks_good[2];
     for(int i=0; i<2; i++) xpeaks_good[i] = xpeaks[ind[i]];

     // now I will do the calibration directly using the peaks given by TSpectrum
     // but it would be even better to do gaussian fits around those values and use
     // the centroids of the gaussians
     TGraphErrors *g = new TGraphErrors(n, x, y)
     TF1 *f = new TF1("f","[0] + [1]*x", 0, 10000); // linear calibration
     g->Fit(f);
  */
  
  // creamos el array que almacenará los parámetros de calibración
  // columnas: m, b (calibración lineal)
  // fila: número de detector

  Double_t cal[47][2]; 

  // creo un histograma para volcar en él los histogramas raw del archivo .root
  TH1D* h_raw_temp;

  // declaro las funciones del fit (una gaussiana para cada pico)
  TF1 *f_fit_a;
  TF1 *f_fit_b;

  // declaro los centroides de los fits y los parámetros de calibración
  Double_t x1, x2;
  Double_t m, b;
  
  ofile->cd();
  for(int h=0; h<47; h++){
    // rechazamos los detectores sin espectro
    if(lims[h][0] == 0){
      cal[h][0] = 0;
      cal[h][1] = 0;
      continue;
    }

    // cogemos el histograma raw
    h_raw_temp = (TH1D*)ofile->Get(Form("h_raw_%i",h));

    // creo las funciones del fit en sus respectivos rangos
    f_fit_a = new TF1("f_fit_a","gaus",lims[h][0],lims[h][1]);
    f_fit_b = new TF1("f_fit_b","gaus",lims[h][2],lims[h][3]);

    // FIT (R = fitear en el rango, N = no dibujar fit, Q = no imprimir en la terminal)
    h_raw_temp->Fit(f_fit_a,"RNQ");
    h_raw_temp->Fit(f_fit_b,"RNQ");

    // cojo los centroides de los picos
    x1 = f_fit_a->GetParameter("Mean");
    x2 = f_fit_b->GetParameter("Mean");
    //cout << "x1: " << x1 << "\tx2: " << x2 << endl;

    // calculo los parámetros de la calibración lineal
    m = (1332.5 - 1173.2) / (x2 - x1);
    b = 1173.2 - m*x1;

    // los guardo
    cal[h][0] = m;
    cal[h][1] = b;
  }

  // ahora hacemos lo mismo que en raw_histos(), leer el tree de 60Co.root
  
  // leemos el fichero .root con el tree
  TFile *ifile = new TFile("./output/60Co.root","READ");
  if (ifile == NULL) {return -1;}

  // leemos el tree
  TTree *mi_tr = (TTree*)ifile->Get("tr");
  if (mi_tr == NULL) {return -2;}

  // creamos las variables que almacenarán el contenido del tree
  event* mi_evento = new event();
  int mi_n_evento;

  mi_tr->SetBranchAddress("n_evento", &mi_n_evento);
  mi_tr->SetBranchAddress("evento", &mi_evento);
  
  Long64_t n_entries = mi_tr->GetEntries();
  cout << "Número de entradas del tree: " << n_entries << endl;

  // creamos los histogramas calibrados
  TH1D *h_cal[47];
  for(int h=0; h<47; h++){
    h_cal[h] = new TH1D(Form("h_cal_%i",h),Form("calibrated energy histogram of detector %i",h),20000,0,20000);
  }
  TH2D *h_cal_summary = new TH2D("h_cal_summary","calibrated energy vs. ID",47,0,47,20000,0,20000);
  TH1D *h_cal_sum = new TH1D("h_cal_sum","calibrated energy histogram of all detectors",20000,0,20000);
  TH1D *h_cal_mult1[47];
  for(int h=0; h<47; h++){
    h_cal_mult1[h] = new TH1D(Form("h_cal_mult1_%i",h),Form("calibrated energy histogram of detector %i, mult 1",h),20000,0,20000);
  }
  
  // BUCLE DE LOS EVENTOS DEL TREE

  Int_t det;
  for (Long64_t i = 0; i < n_entries; i++){

    //if(i*100/n_entries % 10 == 0){
    if(i % 100000 == 0){
      cout << "Program completion: " << i*100/n_entries << "%" << endl;
    }
    mi_tr->GetEntry(i);

    // llenar histogramas con energía calibrada
    for (int m=0; m < mi_evento->GetMult(); m++){
      det = mi_evento->GetID(m);
      h_cal[det]->Fill(mi_evento->GetE(m)*cal[det][0] + cal[det][1]);
      h_cal_summary->Fill(det, mi_evento->GetE(m)*cal[det][0] + cal[det][1]);
      if(mi_evento->GetMult() == 1 && mi_evento->GetE(0)*cal[det][0] + cal[det][1] > 300){
	h_cal_mult1[det]->Fill(mi_evento->GetE(0)*cal[det][0] + cal[det][1]);
      }
    }   
  }

  // suma de todos los espectros
  Double_t content = 0;
  for(int i=0; i<h_cal_sum->GetNbinsX(); i++){
    for(int h=0; h<47; h++){
      content = content + h_cal[h]->GetBinContent(i+1);
    }
    h_cal_sum->SetBinContent(i+1,content);
    content = 0;
  }
  
  ofile->cd();
  for(int h=0; h<47; h++){
    h_cal[h]->Write("",TObject::kOverwrite);
  }
  h_cal_summary->Write("",TObject::kOverwrite);
  h_cal_sum->Write("",TObject::kOverwrite);
  for(int h=0; h<47; h++){
    h_cal_mult1[h]->Write("",TObject::kOverwrite);
  }

  ifile->Close();
  ofile->Close();

  cout << "Finished!" << endl;
  
  
  return 3;
  
}

// ejemplo de función para dibujar espectros
void Draw(){

  cout << "STARTING DRAW()........." << endl;

  TCanvas *c = new TCanvas("c","c");
  TPad *p1 = new TPad("p1","Detector 0",0.02,0.05,0.49,0.9);
  TPad *p2 = new TPad("p2","Detector 1",0.51,0.05,0.98,0.9);
  p1->SetFillColor(0);
  p2->SetFillColor(0);
  p1->Draw();
  p2->Draw();

  TFile *f = new TFile("./output/histos.root");
  TH1D* h0 = (TH1D*)f->Get("h_cal_0");
  TH1D* h1 = (TH1D*)f->Get("h_cal_1");

  h0->SetLineColor(kRed);
  h1->SetLineColor(kAzure+2);
  
  TLegend *l = new TLegend(0.6,0.7,0.87,0.8);                                  
  l->AddEntry(h0,"Crystal 0","f");                                          
  l->AddEntry(h1,"Crystal 1","f");

  c->cd();
  
  p1->cd();
  
  h0->Draw();
  h0->GetXaxis()->SetRangeUser(1000,1500);
  h0->SetXTitle("Energy (keV)");
  h0->SetYTitle("Counts / 1 keV");

  TPaveText *texto = new TPaveText(1250,400,1350,700);
  texto->AddText("texto de prueba");
  texto->Draw("same");

  p2->cd();
  
  h1->Draw();
  h1->GetXaxis()->SetRangeUser(1000,1500);
  h1->SetXTitle("Energy (keV)");
  h1->SetYTitle("Counts / 1 keV");

  l->Draw("same");

  c->Update();
  //  c->Modified();
  c->WaitPrimitive();

}
  
int main(Int_t argc, Char_t** argv){
  TApplication app("app", &argc, argv);
  Write_tree();
  Raw_histos();
  Cal_histos();
  Draw();
  return 0;
}
