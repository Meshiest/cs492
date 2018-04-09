#include <iostream>
#include <string>
#include <sstream>

using namespace std;

class MyException {

private:
  string  msg;

public:
  MyException(string str) {
    msg = str;
  }

  void PrinterMsg() {
    cout << msg.c_str() << endl;
  }
};

class VMSimulator {

public:
  VMSimulator(int argc, char** argv);

private:
  int _page_size;
  int _algo;
  bool _prepaging;

  void LoadPList();
  void LoadPTrace();

  void FIFO();
  void LRU();
  void Clock();
};

VMSimulator::VMSimulator(int argc, char** argv) {
  istringstream iss;
  iss.str(((string) argv[3]));

  if(!(iss >> this->_page_size)) {
    throw MyException("Error: Page size was not a valid integer.");
  }

  if(this->_page_size <= 0) {
    throw MyException("Error: Page size cannot be less than or equal to 0.");
  }

  string algo = (string) argv[4];
  if(!algo.compare("fifo") || !algo.compare("FIFO")) {
    this->_algo = 0;
  } else if(!algo.compare("lru") || !algo.compare("LRU")) {
    this->_algo = 1;
  } else if(!algo.compare("clock") || !algo.compare("Clock")) {
    this->_algo = 2;
  } else {
    throw MyException("Error: Invalid algorithim type.");
  }

  if(argv[5][0] == '+') {
    this->_prepaging = true;
  } else if(argv[5][0] == '-') {
    this->_prepaging = false;
  } else {
    throw MyException("Error: Invalid prepaging operator.");
  }
}

int main(int argc, char** argv) {

  if(argc != 6) {
    cerr << "Error Invalid usage: <./VMSimulator> <plist> <ptrace> <page size> <page algo: FIFO | LRU | CLock> <pre_page: on(+) | off(-)>" << endl;
    return -1;
  }

  try {
    VMSimulator(argc, argv);
  } catch(MyException& err) {
    err.PrinterMsg();
  }

  return 0;
}
