#include "PSI/include/Defines.h"
#include "PSI/include/utils.h"
#include "PSI/include/PsiSender.h"
#include "PSI/include/PsiReceiver.h"

#include <cryptoTools/Network/IOService.h>
#include <cryptoTools/Network/Endpoint.h>
#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/CLP.h>
#include <cryptoTools/Common/Log.h>

#include <vector>
#include <fstream>
#include <cmath>

using namespace std;
using namespace PSI;

const block commonSeed = oc::toBlock(123456);

u64 senderSize;
u64 receiverSize;
u64 width;
u64 height;
u64 logHeight;
u64 hashLengthInBytes;
u64 bucket, bucket1, bucket2;
string ip;
string input_path = "";

void read_elements(vector<block>& data_set, uint64_t* nelements, string filename) {
  uint32_t i, j;
  ifstream infile(filename.c_str());
  if(!infile.good()) {
    cerr << "Input file " << filename << " does not exist, program exiting!" << endl;
    exit(0);
  }
  string line;
  if(*nelements == 0) {
    while (std::getline(infile, line)) {
      ++*nelements;
    }
  }

  infile.clear();
  infile.seekg(ios::beg);

  RandomOracle H(16); // 16 is block size in BYTES, BLAKE2;
  unsigned char  hOutput[16] = {0};

  for(i = 0; i < *nelements; i++) {
    std::getline(infile, line);
    int line_len = line.length();
    H.Reset();
    H.Update((uint8_t*) line.c_str(), line_len);
    H.Final(hOutput);

    data_set.push_back(oc::toBlock(hOutput));
  }
}

void runSender() {
  IOService ios;
  Endpoint ep(ios, ip, EpMode::Server, "test-psi");
  Channel ch = ep.addChannel();


  PRNG prng(oc::toBlock(123));
  vector<block> senderSet;

  if (input_path == "") {
    senderSet.resize(senderSize);
    for (auto i = 0; i < senderSize; ++i) {
      senderSet[i] = prng.get<block>();
    }
  }
  else {
    /**********add file path api****************/
    read_elements(senderSet, &senderSize, input_path);
    ch.send(&senderSize, 1);
    ch.recv(&receiverSize, 1);
    height = receiverSize;
    logHeight = std::floor(std::log2(height));
    
    std::cout << " sender/receiver size = " << senderSize << "/" << receiverSize << std::endl;
  }

  PsiSender psiSender;
  psiSender.run(prng, ch, commonSeed, senderSize, receiverSize, height, logHeight, width, senderSet, hashLengthInBytes, 32, bucket1, bucket2);

  ch.close();
  ep.stop();
  ios.stop();
}

void runReceiver() {
  IOService ios;
  Endpoint ep(ios, ip, EpMode::Client, "test-psi");
  Channel ch = ep.addChannel();

  PRNG prng(oc::toBlock(123));
  vector<block> receiverSet;

  if (input_path == "") {
    assert(receiverSize > 0);
    receiverSet.resize(receiverSize);

    for (auto i = 0; i < 100; ++i) {
      receiverSet[i] = prng.get<block>();
    }

    PRNG prng2(oc::toBlock(456));
    for (auto i = 100; i < receiverSize; ++i) {
      receiverSet[i] = prng2.get<block>();
    }
  }
  else {
    /**********add file path api****************/
    read_elements(receiverSet, &receiverSize, input_path);
    ch.send(&receiverSize, 1);
    ch.recv(&senderSize, 1);
    height = receiverSize;
    logHeight = std::floor(std::log2(height));
    
    std::cout << "receiver file size = " << receiverSize << std::endl;
  }

  PsiReceiver psiReceiver;
  psiReceiver.run(prng, ch, commonSeed, senderSize, receiverSize, height, logHeight, width, receiverSet, hashLengthInBytes, 32, bucket1, bucket2);
  
  // save plain result;
  if (input_path != "") {
    ifstream infile(input_path.c_str()), indexfile("./index_result.csv");
    ofstream ofile;
    ofile.open("./intersection_result.csv");
  
    if(!infile.good() || !indexfile.good() || !ofile.good()) {
      string file_name = "";
      if (!infile.good()) file_name = input_path;
      if (!indexfile.good()) file_name = "./index_result.csv";
      if (!ofile.good()) file_name = "./intersection.csv";
      cerr << "file " << file_name << " does not exist, program exiting!" << endl;
      ch.close();
      ep.stop();
      ios.stop();
      exit(0);
    }
    std::vector<uint64_t> index;
    std::vector <std::string> intersection;
    std::string line;
    while (std::getline(indexfile, line)) {  // get the intersection index, which is  output of PSI
      index.push_back(std::atoi(line.c_str()));
    }
    if (index.size() > 0) { // intersection not empty
      uint32_t index_count  = 0;
      uint32_t line_count = 0;
      while (std::getline(infile, line)) {   
        if (line_count == index[index_count]) {
          intersection.push_back(line);
          index_count++;
          if (index_count > index.size()) break;
        }
        line_count++;
      }
      for (uint32_t i = 0; i < intersection.size(); ++i) {
        ofile << intersection[i] << std::endl;
      }
     }
  }
  

  ch.close();
  ep.stop();
  ios.stop();
}




int main(int argc, char** argv) {

  oc::CLP cmd;
  cmd.parse(argc, argv);

  cmd.setDefault("ss", 20);
  senderSize = 1 << cmd.get<u64>("ss");

  cmd.setDefault("rs", 20);
  receiverSize = 1 << cmd.get<u64>("rs");

  cmd.setDefault("w", 632);
  width = cmd.get<u64>("w");

  cmd.setDefault("h", 20);
  logHeight = cmd.get<u64>("h");
  height = 1 << cmd.get<u64>("h");

  cmd.setDefault("hash", 10);
  hashLengthInBytes = cmd.get<u64>("hash");

  cmd.setDefault("ip", "localhost");
  ip = cmd.get<string>("ip");

  cmd.setDefault("file", "");
  input_path = cmd.get<string>("file");
  if (input_path != "") {
    senderSize = 0;
    receiverSize = 0;
  }


  bucket1 = bucket2 = 1 << 8;

  bool noneSet = !cmd.isSet("r");
  if (noneSet) {
    std::cout
            << "=================================\n"
            << "||  Private Set Intersection   ||\n"
            << "=================================\n"
            << "\n"
            << "This program reports the performance of the private set intersection protocol.\n"
            << "\n"
            << "Experimenet flag:\n"
            << " -r 0    to run a sender.\n"
            << " -r 1    to run a receiver.\n"
            << "\n"
            << "Parameters:\n"
            << " -ss     log(#elements) on sender side.\n"
            << " -rs     log(#elements) on receiver side.\n"
            << " -w      width of the matrix.\n"
            << " -h      log(height) of the matrix.\n"
            << " -hash   hash output length in bytes.\n"
            << " -ip     ip address (and port).\n"
            ;
  } else {
    if (cmd.get<u64>("r") == 0) {
      runSender();
    } else if (cmd.get<u64>("r") == 1) {
      runReceiver();
    }
  }

  return 0;
}

