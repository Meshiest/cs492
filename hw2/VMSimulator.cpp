#include <algorithm>
#include <climits>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

// simple class to throw custom execptions
class MyException {

public:
  MyException(string str);

  void PrinterMsg();

private:
  string _msg;

};

// sets the error msg
MyException::MyException(string str)  {
  this->_msg = str;
}

// prints err msg
void MyException::PrinterMsg() {
  cout << this->_msg << endl;
}


// class for page
class Page {

public:
  Page(int pnum);
  int getPageNum();
  bool getValid();
  unsigned long getTime();
  void updateTime(unsigned long& counter);
  void setValid(bool valid);

private:
  // the page num
  int _page_num;
  // whether page is valid or not
  bool _valid;
  // last accessed time
  unsigned long _access_time;

};

// cpage constructor
Page::Page(int pnum) {
  this->_page_num = pnum;
  this->_valid = 0;
  this->_access_time = 0;
}

// getter for page num
int Page::getPageNum() {
  return this->_page_num;
}

// getter for page validity
bool Page::getValid() {
  return this->_valid;
}

// getter for time
unsigned long Page::getTime() {
  return this->_access_time;
}

// setter for time
void Page::updateTime(unsigned long& counter) {
  this->_access_time = counter++;
}

// setter for validity
void Page::setValid(bool valid) {
  this->_valid = valid;
}

// class for process
class Process {

public:
  Process(int pid, int size, int& page_num);
  int loadPages(int num, int algo, vector<int>& mem);
  Page* getPage(int page);
  int getSize();
  int getID();
  int getPagesRemaining();
  void setPagesRemaining(int pages);

  vector<Page*>::iterator getIterator();
  vector<Page*>::iterator getEndIterator();
  void resetIterator();
  void incrementIterator();
  void replacePage(int page_one, int page_two);
  bool isInMemory(int page);

private:
  // list of all pages
  std::vector<Page*> _page_table;
  // list of all loaded pages
  std::vector<Page*> _loaded_pages;
  // process id
  int _pid;
  // process size
  int _size;
  // num of pages remaining
  int _pages_remaining;
  // iterator to begining of list
  vector<Page*>::iterator _page_iterator;
};

// process constructor
// page_num is reference so it changes the VMSimulator pagNum value
Process::Process(int pid, int size, int& page_num) {
  this->_pid = pid;
  this->_size = size;

  // insert new pages
  for(int pnum = 0; pnum < size; pnum++) {
    this->_page_table.push_back(new Page(++page_num));
  }

  this->_page_iterator = this->_page_table.begin();
}

// create list of pages currently loaded
// reference to VMSimulatr main memory
int Process::loadPages(int num, int algo, vector<int>& mem) {
  this->_pages_remaining = num;

  int num_faults = 0;

  // for each page in page_table set to valid
  for(int cur = 0; cur < this->_size && this->_pages_remaining; cur++) {
    Page* page = this->_page_table.at(cur);
    page->setValid(true);

    // if fifo or lru push back to main memort
    if(algo == 0 || algo == 1) {
      mem.push_back(page->getPageNum());
    } else if(algo == 2) { // clock push to loaded pages
      this->_loaded_pages.push_back(page);
    }

    num_faults++;
    this->_pages_remaining--;
  }

  if(algo == 2) {
    this->_page_iterator = this->_loaded_pages.begin();
  }

  return num_faults;
}

// getter for page from all pages given index of page
Page* Process::getPage(int page_num) {
  return this->_page_table.at(page_num);
}

// getter for proc size
int Process::getSize() {
  return this->_size;
}

// getter for page id
int Process::getID() {
  return this->_pid;
}

// getter for num of remaining pages
int Process::getPagesRemaining() {
  return this->_pages_remaining;
}

// setter for num of remaining pages
void Process::setPagesRemaining(int num_pages) {
  this->_pages_remaining = num_pages;
}

// getter for page list start iterator (clock only)
vector<Page*>::iterator Process::getIterator() {
  return this->_page_iterator;
}

// getter for page list end iterator (clock only)
vector<Page*>::iterator Process::getEndIterator() {
  return this->_loaded_pages.end();
}

// reset page list iterator (clock only)
void Process::resetIterator() {
  this->_page_iterator = this->_loaded_pages.begin();
}

// increment page list iterator (clock only)
void Process::incrementIterator() {
  this->_page_iterator++;
}

// removes page_one inserts page_two (clock only)
void Process::replacePage(int page_one, int page_two) {
  Page* page_copy;

  if(page_one > -1) {

    for(unsigned int cur = 0; cur < this->_loaded_pages.size(); cur++) {
      // if cur page is equal to page_one id remove it
      if(this->_loaded_pages.at(cur)->getPageNum() == page_one) {
        this->_loaded_pages.erase(this->_loaded_pages.begin() + cur);
        this->_pages_remaining++;
        break;
      }

    }

  }

  // grab page two
  for(unsigned int cur = 0; cur < this->_page_table.size(); cur++) {

    page_copy = this->_page_table.at(cur);

    if(page_copy->getPageNum() == page_two) {
      break;
    }

  }

  // set it to valid and insert into loaded pages
  page_copy->setValid(true);
  this->_loaded_pages.insert(this->_page_iterator, page_copy);
  this->_pages_remaining--;
}

// check if page is in list of pages
bool Process::isInMemory(int page_num) {
  Page* page;

  for(unsigned int cur = 0; cur < this->_page_table.size(); cur++) {

    if(this->_page_table.at(cur)->getPageNum() == page_num) {
      page = this->_page_table.at(cur);
      break;
    }

  }

  return find(this->_loaded_pages.begin(), this->_loaded_pages.end(), page) != this->_loaded_pages.end();
}

// class for overall program
class VMSimulator {

public:
  VMSimulator(int argc, char** argv);

private:
  // given construct
  const int _MAX_MEM = 512;

  // passed in params saved to class
  int _page_size;
  int _algo;
  bool _prepaging;

  int _page_num = 0;

  // the vector of main memory
  vector<int> _main_memory;
  // counter
  unsigned long _counter;

  vector<Process*> _processes;
  int _process_count = 0;

  int _page_fault = 0;

  int getIndex(int search);

  void LoadPList(string plist_path);
  void LoadPTrace(string ptrace_path);

  void FIFO(Process* proc, int page_loc);
  void LRU(Process* proc, Page* page);
  void Clock(Process* proc, int page_loc);
};

int VMSimulator::getIndex(int search) {
  int found_id = -1;

  for(unsigned int cur = 0; cur < this->_main_memory.size(); cur++) {
    if(this->_main_memory.at(cur) == search) {
      found_id = cur;
      break;
    }
  }

  return found_id;
}

// fifo function wip
void VMSimulator::FIFO(Process* proc, int page_loc) {
  int min_id = proc->getPage(0)->getPageNum();
  unsigned int cur, size = this->_main_memory.size();
  int max_id = proc->getPage(proc->getSize()-1)->getPageNum();
  int id;

  // insert into first index
  for(cur = 0; cur < size; cur++) {
    id = this->_main_memory.at(cur);
    if(id <= max_id && id >= min_id) break;
  }

  // unload page, say its not found in memory
  this->_main_memory.erase(this->_main_memory.begin() + cur);
  proc->getPage(id - min_id)->setValid(false);

  // put new page into memory
  this->_main_memory.push_back(page_loc);
  proc->getPage(page_loc - min_id)->setValid(true);

  if(this->_prepaging) {

    if(!proc->getPagesRemaining()) {

      for(cur = 0; cur < size; cur++) {
        id = this->_main_memory.at(cur);

        if(id >= min_id && id <= max_id) break;
      }

      this->_main_memory.erase(this->_main_memory.begin() + cur);
      proc->getPage(id - min_id)->setValid(0);
      proc->setPagesRemaining(1);
    }

    for(cur = 0; cur < (unsigned int) proc->getSize(); cur++) {
      if(proc->getPage(cur)->getPageNum() > page_loc && !proc->getPage(cur)->getValid()) break;
    }

    this->_main_memory.push_back(proc->getPage(cur)->getPageNum());
    proc->getPage(cur)->setValid(1);
    proc->setPagesRemaining(proc->getPagesRemaining() - 1);
  }
}

void VMSimulator::LRU(Process* proc, Page* page) {

  unsigned long min_up = ULONG_MAX;
  int min_up_pid;
  Page* temp;

  for(int cur = 0; cur < proc->getSize(); cur++) {

    temp = proc->getPage(cur);

    if(temp->getValid() && temp->getTime() < min_up) {
      min_up = temp->getTime();
      min_up_pid = cur;
    }
  }

  proc->getPage(min_up_pid)->setValid(0);
  this->_main_memory.erase(this->_main_memory.begin() + getIndex(proc->getPage(min_up_pid)->getPageNum()));

  this->_main_memory.push_back(page->getPageNum());
  page->setValid(1);
  page->updateTime(this->_counter);

  if(this->_prepaging) {

    if(proc->getPagesRemaining() == 0) {

      min_up = ULONG_MAX;
      for(int cur = 0; cur < proc->getSize(); cur++) {

        temp = proc->getPage(cur);
        if (temp->getValid() && temp->getTime() < min_up) {
          min_up = temp->getTime();
          min_up_pid = cur;
        }

      }

      proc->getPage(min_up_pid)->setValid(0);
      proc->getPage(min_up_pid)->updateTime(this->_counter);
      this->_main_memory.erase(this->_main_memory.begin() + getIndex(proc->getPage(min_up_pid)->getPageNum()));
      proc->setPagesRemaining(1);
    }

    for(int cur = 0; cur < proc->getSize(); cur++) {

      if(proc->getPage(cur)->getPageNum() > page->getPageNum() && !proc->getPage(cur)->getValid()) {
        this->_main_memory.push_back(proc->getPage(cur)->getPageNum());
        proc->getPage(cur)->setValid(1);
        proc->getPage(cur)->updateTime(this->_counter);
        proc->setPagesRemaining(proc->getPagesRemaining() - 1);
        break;
      }

    }

  }

}

void VMSimulator::Clock(Process* proc, int page_loc) {
  Page* tmp;
  Page* new_page;
  int next_page_num;

  for(;;proc->incrementIterator()) {

    if(proc->getIterator() == proc->getEndIterator()) proc->resetIterator();

    tmp = *(proc->getIterator());

    if(!tmp->getValid()) {
      proc->replacePage(tmp->getPageNum(), page_loc);
      break;
    }
    else tmp->setValid(0);

  }

  if(this->_prepaging) {

    if(!proc->getPagesRemaining()) {

      for(int cur = 0; cur < proc->getSize(); cur++) {
        new_page = proc->getPage(cur);

        if(new_page->getPageNum() > page_loc && !proc->isInMemory(new_page->getPageNum())) {
        next_page_num = new_page->getPageNum();
        break;
        }

      }

    }

    for(;;proc->incrementIterator()) {

      if (proc->getIterator() == proc->getEndIterator()) {
        proc->resetIterator();
      }

      tmp = *(proc->getIterator());

      if(tmp->getValid() == 0) {

        if(!proc->getPagesRemaining()) {
          proc->replacePage(tmp->getPageNum(), next_page_num);
        }
        else {
          proc->replacePage(-1, next_page_num);
        }

        break;
      }

      else {
        tmp->setValid(0);
      }

    }

  }
}

// loads plist file
void VMSimulator::LoadPList(string plist_path) {

  ifstream plist_file(plist_path);
  if(!plist_file) {
    plist_file.clear();
    plist_file.open("plist.txt");

    if(!plist_file) {
      throw MyException("Error: Please give path to a plist file or have default file plist.txt in directory.");
    }
  }

  int first, second;
  while(plist_file >> first >> second) {
    this->_processes.push_back(new Process(first, second, this->_page_num));
    this->_process_count++;
  }

 /*for(auto p: this->_processes) {
    cout << proc->getID() << " " << proc->getSize() << endl;
  }*/

  plist_file.close();

  for (int cur = 0; cur  < this->_process_count; cur++) {
    this->_page_fault += this->_processes.at(cur)->loadPages((int) ceil((double) (this->_MAX_MEM / this->_page_size) / this->_process_count), this->_algo, this->_main_memory);
    //cout << this->_main_memory[cur] << endl;;
  }

}

// loads ptrace file
void VMSimulator::LoadPTrace(string ptrace_path) {

  ifstream ptrace_file(ptrace_path);
  if(!ptrace_file) {
    ptrace_file.clear();
    ptrace_file.open("ptrace.txt");

    if(!ptrace_file) {
      throw MyException("Error: Please give path to a ptrace file or have default file ptrace.txt in directory.");
    }
  }

  vector<int>::iterator it;
  double first, second, page;
  Page* found_page;
  while(ptrace_file >> first >> second) {
    page = (int) floor(second / this->_page_size);
    found_page = this->_processes.at(first)->getPage(page);
    //cout << page << " " << foundPage << endl;

    // perform page replacment algorithm
    if(this->_algo == 2) {

      if(!this->_processes.at(first)->isInMemory(found_page->getPageNum())) {
        this->_page_fault++;
        this->Clock(this->_processes.at(first), found_page->getPageNum());
      } else {
        found_page->setValid(true);
      }

    } else if(this->_algo == 0 || this->_algo == 1) {
      it = find(this->_main_memory.begin(), this->_main_memory.end(), found_page->getPageNum());

      if(it == this->_main_memory.end()) {
        this->_page_fault++;

        if(this->_algo == 0) {
          this->FIFO(this->_processes.at(first), found_page->getPageNum());
        }
        else if(this->_algo == 1) {
          this->LRU(this->_processes.at(first), found_page);
        }

      } else {
        found_page->updateTime(this->_counter);
      }

    }
  }

  cout << this->_page_fault << " swaps" << endl;

}

// constructor takes passed arguments handles properly
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

  this->LoadPList((string) argv[1]);

  this->LoadPTrace((string) argv[2]);

  this->_main_memory.clear();
}

int main(int argc, char** argv) {

  if(argc != 6) {
    cerr << "Error Invalid usage: <" << *argv <<  "> <plist> <ptrace> <page size> <page algo: FIFO | LRU | CLock> <pre_page: on(+) | off(-)>" << endl;
    return -1;
  }

  try {
    VMSimulator(argc, argv);
  } catch(MyException& err) {
    err.PrinterMsg();
  }

  return 0;
}
