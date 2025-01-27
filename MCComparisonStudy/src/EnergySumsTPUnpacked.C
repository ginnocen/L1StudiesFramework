#include "TFile.h"
#include "TDirectory.h"
#include "TSystemDirectory.h"
#include "TSystemFile.h"
#include "TChain.h"

#include "TTreeReader.h"
#include "TTreeReaderValue.h"

#include "TMath.h"
#include "TH1F.h"
#include "TGraphAsymmErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TLatex.h"
#include "TProfile2D.h"

#include <string>
#include <vector>
#include <iostream>

using namespace std;

void FormatHistogram(TH1F* hist, int color) {
    hist->SetMarkerColor(color);
    hist->SetLineColor(color);
    hist->SetMarkerSize(0.5);
    hist->SetMarkerStyle(20);
    hist->SetStats(0);
    hist->GetXaxis()->CenterTitle(true);
    hist->GetYaxis()->SetTitle("Normalized Counts");
    hist->GetYaxis()->CenterTitle(true);
}

void FormatHistogramProf2D(TProfile2D* hist, double max) {
    hist->SetStats(0);
    hist->GetXaxis()->SetTitle("Eta");
    hist->GetXaxis()->CenterTitle(true);
    hist->GetYaxis()->SetTitle("Phi");
    hist->GetYaxis()->CenterTitle(true);
    hist->SetMinimum(0);
    hist->SetMaximum(max);
}

void PrintHist(TH1* hist1, TH1* hist2, string title, TCanvas* canvas, TLegend* legend, string filename) {
    hist1->GetXaxis()->SetTitle(title.c_str());
    hist1->Draw("HIST LP");
    hist2->Draw("HIST LP SAME");
    legend->Draw();

    TLatex* newMean = new TLatex();
    string newMeanText;
    newMean->SetTextFont(43);
    newMean->SetTextSize(12);
    newMean->SetTextColor(30);

    TLatex* oldMean = new TLatex();
    string oldMeanText;
    oldMean->SetTextFont(43);
    oldMean->SetTextSize(12);
    oldMean->SetTextColor(46);

    oldMeanText = "2018 Mean: " + to_string(hist2->GetMean());
    oldMean->DrawLatexNDC(0.6, 0.64, oldMeanText.c_str());
    newMeanText = "2022 Mean: " + to_string(hist1->GetMean());
    newMean->DrawLatexNDC(0.6, 0.60, newMeanText.c_str());
    canvas->Print(filename.c_str());
}

void PrintHistProf2D(TProfile2D* hist1, TProfile2D* hist2, TCanvas* canvas, string filename) {
    hist1->Draw("COLZ");
    canvas->Print(filename.c_str());
    hist2->Draw("COLZ");
    canvas->Print(filename.c_str());
}

void GetFiles(char const* input, vector<string>& files) {
    TSystemDirectory dir(input, input);
    TList *list = dir.GetListOfFiles();

    if (list) {
        TSystemFile *file;
        string fname;
        TIter next(list);
        while ((file = (TSystemFile*) next())) {
            fname = file->GetName();

            if (file->IsDirectory() && (fname.find(".") == string::npos)) {
                string newDir = string(input) + fname + "/";
                GetFiles(newDir.c_str(), files);
            }
            else if ((fname.find(".root") != string::npos)) {
                files.push_back(string(input) + fname);
                cout << files.back() << endl;
            }
        }
    }

    return;
}

void FillChain(TChain& chain, vector<string>& files) {
    for (auto file : files) {
        chain.Add(file.c_str());
    }
}

int Compare(char const* oldInput, char const* newInput) {
    /* create map of bit number to energy sum */
    map<int, string> EnergySum = {
        {0, "etSumTotalEt"},
        {1, "etSumTotalEtHF"},
        {2, "etSumTotalEtEm"},
        {3, "etSumMinBiasHFP0"},
        {4, "htSumht"},
        {5, "htSumhtHF"},
        {6, "etSumMinBiasHFM0"},
        {7, "etSumMissingEt"},
        {8, "etSumMinBiasHFP1"},
        {9, "htSumMissingHt"},
        {10, "etSumMinBiasHFM1"},
        {11, "etSumMissingEtHF"},
        {12, "htSumMissingHtHF"},
        {13, "etSumTowCount"},
        {14, "etAsym"},
        {15, "etHFAsym"},
        {16, "htAsym"},
        {17, "htHFAsym"},
        {18, "centrality"}
    };

    /* read in 2018 information */
    vector<string> oldFiles;
    GetFiles(oldInput, oldFiles);

    TChain oldEnergySumChain("l1UpgradeTree/L1UpgradeTree");
    TChain oldCaloTowerChain("l1CaloTowerTree/L1CaloTowerTree");
    FillChain(oldEnergySumChain, oldFiles);
    FillChain(oldCaloTowerChain, oldFiles);
    TTreeReader oldCaloTowerReader(&oldCaloTowerChain);
    TTreeReaderValue<short> oldCaloNTowers(oldCaloTowerReader, "nHCALTP");
    TTreeReaderValue<vector<float>> oldCaloIHad(oldCaloTowerReader, "hcalTPet");
    TTreeReaderValue<vector<short>> oldCaloIEta(oldCaloTowerReader, "hcalTPieta");
    TTreeReaderValue<vector<short>> oldCaloIPhi(oldCaloTowerReader, "hcalTPiphi");

    /* read in 2022 information */
    vector<string> newFiles;
    GetFiles(newInput, newFiles);

    TChain newEnergySumChain("l1UpgradeTree/L1UpgradeTree");
    TChain newCaloTowerChain("l1CaloTowerTree/L1CaloTowerTree");
    FillChain(newEnergySumChain, newFiles);
    FillChain(newCaloTowerChain, newFiles);
    TTreeReader newCaloTowerReader(&newCaloTowerChain);
    TTreeReaderValue<short> newCaloNTowers(newCaloTowerReader, "nHCALTP");
    TTreeReaderValue<vector<float>> newCaloIHad(newCaloTowerReader, "hcalTPet");
    TTreeReaderValue<vector<short>> newCaloIEta(newCaloTowerReader, "hcalTPieta");
    TTreeReaderValue<vector<short>> newCaloIPhi(newCaloTowerReader, "hcalTPiphi");

    /* create histograms for energy sum plots */
    int nbins = 40;
    float min = 0;
    float max = 3000;

    size_t size = EnergySum.size();

    auto oldEnergySumHist = new TH1F("oldEnergySumHist", "", nbins, min, max);
    auto newEnergySumHist = new TH1F("newEnergySumHist", "", nbins, min, max);

    int oldEntries = oldEnergySumChain.GetEntries();
    int newEntries = newEnergySumChain.GetEntries();

    /* customize energy sum histogram draw options */
    auto legend = new TLegend(0.55, 0.75 ,0.85, 0.85);
    legend->SetTextSize(0.03);
    legend->AddEntry(oldEnergySumHist, "2018 MB MC", "p");
    legend->AddEntry(newEnergySumHist, "2022 MB MC", "p");

    FormatHistogram(newEnergySumHist, 30);
    FormatHistogram(oldEnergySumHist, 46);

    /* plot the energy sum distributions */
    auto canvas = new TCanvas("canvas", "", 0 , 0, 500, 500);
    canvas->SetLeftMargin(0.15);
    canvas->SetBottomMargin(0.15);
    canvas->Print("L1EnergySumsUnpacked.pdf[");

    for (size_t i = 0; i < size; ++i) {
        canvas->Clear();

        string oldDrawCommand = "sumEt[" + to_string(i) + "] >> oldEnergySumHist";
        string newDrawCommand = "sumEt[" + to_string(i) + "] >> newEnergySumHist";

        oldEnergySumChain.Draw(oldDrawCommand.c_str(), "", "goff");
        newEnergySumChain.Draw(newDrawCommand.c_str(), "", "goff");

        oldEnergySumHist->Scale(1.0/oldEntries);
        newEnergySumHist->Scale(1.0/newEntries);

        PrintHist(newEnergySumHist, oldEnergySumHist, EnergySum[i], canvas, legend, "L1EnergySumsUnpacked.pdf");
    }

    canvas->Print("L1EnergySumsUnpacked.pdf]");

    /* create histograms for caloTower plots */
    auto oldCaloNTowersHist = new TH1F("oldCaloNTowersHist", "", nbins*2, 0, 5500);
    auto oldCaloIHBHist = new TH1F("oldCaloIHBHist", "", nbins*2, 0, 2500);
    auto oldCaloIHEHist = new TH1F("oldCaloIHEHist", "", nbins*2, 0, 7000);
    auto oldCaloIHFHist = new TH1F("oldCaloIHFHist", "", nbins*2, 0, 13000);

    auto newCaloNTowersHist = new TH1F("newCaloNTowersHist", "", nbins*2, 0, 5500);
    auto newCaloIHBHist = new TH1F("newCaloIHBHist", "", nbins*2, 0, 2500);
    auto newCaloIHEHist = new TH1F("newCaloIHEHist", "", nbins*2, 0, 7000);
    auto newCaloIHFHist = new TH1F("newCaloIHFHist", "", nbins*2, 0, 13000);

    auto oldCaloNTowersHistZoom = new TH1F("oldCaloNTowersHistZoom", "", nbins, 0, 550);
    auto oldCaloIHBHistZoom = new TH1F("oldCaloIHBHistZoom", "", nbins, 0, 300);
    auto oldCaloIHEHistZoom = new TH1F("oldCaloIHEHistZoom", "", nbins, 0, 700);
    auto oldCaloIHFHistZoom = new TH1F("oldCaloIHFHistZoom", "", nbins, 0, 1300);

    auto newCaloNTowersHistZoom = new TH1F("newCaloNTowersHistZoom", "", nbins, 0, 550);
    auto newCaloIHBHistZoom = new TH1F("newCaloIHBHistZoom", "", nbins, 0, 300);
    auto newCaloIHEHistZoom = new TH1F("newCaloIHEHistZoom", "", nbins, 0, 700);
    auto newCaloIHFHistZoom = new TH1F("newCaloIHFHistZoom", "", nbins, 0, 1300);

    auto oldCaloIHadEtaPhiHist = new TProfile2D("oldCaloIHadEtaPhiHist", "2018 Average Had", 84, -42, 42, 73, 0, 73);
    auto newCaloIHadEtaPhiHist = new TProfile2D("newCaloIHadEtaPhiHist", "2022 Average Had", 84, -42, 42, 73, 0, 73);

    /* customize calo tower histogram draw options */
    FormatHistogram(oldCaloNTowersHist, 46);
    FormatHistogram(oldCaloIHBHist, 46);
    FormatHistogram(oldCaloIHEHist, 46);
    FormatHistogram(oldCaloIHFHist, 46);

    FormatHistogram(newCaloNTowersHist, 30);
    FormatHistogram(newCaloIHBHist, 30);
    FormatHistogram(newCaloIHEHist, 30);
    FormatHistogram(newCaloIHFHist, 30);

    FormatHistogram(oldCaloNTowersHistZoom, 46);
    FormatHistogram(oldCaloIHBHistZoom, 46);
    FormatHistogram(oldCaloIHEHistZoom, 46);
    FormatHistogram(oldCaloIHFHistZoom, 46);

    FormatHistogram(newCaloNTowersHistZoom, 30);
    FormatHistogram(newCaloIHBHistZoom, 30);
    FormatHistogram(newCaloIHEHistZoom, 30);
    FormatHistogram(newCaloIHFHistZoom, 30);

    FormatHistogramProf2D(oldCaloIHadEtaPhiHist, 4);
    FormatHistogramProf2D(newCaloIHadEtaPhiHist, 4);


    /* read in information from TTrees */
    for (int i = 1; i < oldEntries; ++i) {
        oldCaloTowerReader.Next();
        if (i % (oldEntries / 20) == 0) cout << i << " / " << oldEntries << endl;

        double hb = 0;
        double he = 0;
        double hf = 0;

        for (int j = 0; j < (*oldCaloNTowers); ++j) {
            if ((*oldCaloIEta)[j] <= 16 && (*oldCaloIEta)[j] >= -16)
                hb += (*oldCaloIHad)[j];
            if ( ((*oldCaloIEta)[j] <= 29 && (*oldCaloIEta)[j] >= 17) || ((*oldCaloIEta)[j] >= -29 && (*oldCaloIEta)[j] <= -17) )
                he += (*oldCaloIHad)[j];
            if ( (*oldCaloIEta)[j] > 29 || (*oldCaloIEta)[j] < -29 )
                hf += (*oldCaloIHad)[j];

            oldCaloIHadEtaPhiHist->Fill((*oldCaloIEta)[j], (*oldCaloIPhi)[j], (*oldCaloIHad)[j]);
        }


        oldCaloNTowersHist->Fill(*oldCaloNTowers);
        oldCaloIHBHist->Fill(hb);
        oldCaloIHEHist->Fill(he);
        oldCaloIHFHist->Fill(hf);

        oldCaloNTowersHistZoom->Fill(*oldCaloNTowers);
        oldCaloIHBHistZoom->Fill(hb);
        oldCaloIHEHistZoom->Fill(he);
        oldCaloIHFHistZoom->Fill(hf);
    }

    for (int i = 1; i < newEntries; ++i) {
        newCaloTowerReader.Next();
        if (i % (newEntries / 20) == 0) cout << i << " / " << newEntries << endl;

        double hb = 0;
        double he = 0;
        double hf = 0;

        for (int j = 0; j < (*newCaloNTowers); ++j) {
            if ((*newCaloIEta)[j] <= 16 && (*newCaloIEta)[j] >= -16)
                hb += (*newCaloIHad)[j];
            if ( ((*newCaloIEta)[j] <= 29 && (*newCaloIEta)[j] >= 17) || ((*newCaloIEta)[j] >= -29 && (*newCaloIEta)[j] <= -17) )
                he += (*newCaloIHad)[j];
            if ( (*newCaloIEta)[j] > 29 || (*newCaloIEta)[j] < -29 )
                hf += (*newCaloIHad)[j];

            newCaloIHadEtaPhiHist->Fill((*newCaloIEta)[j], (*newCaloIPhi)[j], (*newCaloIHad)[j]);
        }

        newCaloNTowersHist->Fill(*newCaloNTowers);
        newCaloIHBHist->Fill(hb);
        newCaloIHEHist->Fill(he);
        newCaloIHFHist->Fill(hf);

        newCaloNTowersHistZoom->Fill(*newCaloNTowers);
        newCaloIHBHistZoom->Fill(hb);
        newCaloIHEHistZoom->Fill(he);
        newCaloIHFHistZoom->Fill(hf);
    }

    /* scale the histograms */
    oldCaloNTowersHist->Scale(1.0/oldEntries);
    oldCaloIHBHist->Scale(1.0/oldEntries);
    oldCaloIHEHist->Scale(1.0/oldEntries);
    oldCaloIHFHist->Scale(1.0/oldEntries);

    oldCaloNTowersHistZoom->Scale(1.0/oldEntries);
    oldCaloIHBHistZoom->Scale(1.0/oldEntries);
    oldCaloIHEHistZoom->Scale(1.0/oldEntries);
    oldCaloIHFHistZoom->Scale(1.0/oldEntries);

    newCaloNTowersHist->Scale(1.0/newEntries);
    newCaloIHBHist->Scale(1.0/newEntries);
    newCaloIHEHist->Scale(1.0/newEntries);
    newCaloIHFHist->Scale(1.0/newEntries);

    newCaloNTowersHistZoom->Scale(1.0/newEntries);
    newCaloIHBHistZoom->Scale(1.0/newEntries);
    newCaloIHEHistZoom->Scale(1.0/newEntries);
    newCaloIHFHistZoom->Scale(1.0/newEntries);

    /* plot the caloTower distributions */
    canvas->Print("L1CaloTPUnpacked.pdf[");
    canvas->SetLogy(1);
    canvas->Clear();

    PrintHist(newCaloNTowersHist, oldCaloNTowersHist, "nTowers", canvas, legend, "L1CaloTPUnpacked.pdf");
    PrintHist(newCaloIHBHist, oldCaloIHBHist, "HB Sum", canvas, legend, "L1CaloTPUnpacked.pdf");
    PrintHist(newCaloIHEHist, oldCaloIHEHist, "HE Sum", canvas, legend, "L1CaloTPUnpacked.pdf");
    PrintHist(newCaloIHFHist, oldCaloIHFHist, "HF Sum", canvas, legend, "L1CaloTPUnpacked.pdf");

    PrintHist(newCaloNTowersHistZoom, oldCaloNTowersHistZoom, "nTowers", canvas, legend, "L1CaloTPUnpacked.pdf");
    PrintHist(newCaloIHBHistZoom, oldCaloIHBHistZoom, "HB Sum", canvas, legend, "L1CaloTPUnpacked.pdf");
    PrintHist(newCaloIHEHistZoom, oldCaloIHEHistZoom, "HE Sum", canvas, legend, "L1CaloTPUnpacked.pdf");
    PrintHist(newCaloIHFHistZoom, oldCaloIHFHistZoom, "HF Sum", canvas, legend, "L1CaloTPUnpacked.pdf");

    canvas->Print("L1CaloTPUnpacked.pdf]");

    canvas->Print("L1CaloTPEtaPhiUnpacked.pdf[");
    canvas->SetLogy(0);
    canvas->Clear();

    PrintHistProf2D(newCaloIHadEtaPhiHist, oldCaloIHadEtaPhiHist, canvas, "L1CaloTPEtaPhiUnpacked.pdf");

    canvas->Print("L1CaloTPEtaPhiUnpacked.pdf]");

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc == 3)
        return Compare(argv[1], argv[2]);

    else {
        cout << "ERROR: Please pass two paths for 2018 MC and 2022 MC." << endl;
        return -1;
    }
}
