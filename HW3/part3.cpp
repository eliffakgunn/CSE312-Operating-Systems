#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <list>
#include <vector>
#include <string.h>
#include <string>
#include <math.h>
#include <sys/time.h>
#include <fstream>
#include <pthread.h>

using namespace std;

void parseCL(int argc, char **argv);
void usageError();
void fillMemories();
unsigned long getCurrentTime();
void set(unsigned int index, int value, char *tName);
int get(unsigned int index, char *tName);
void initPTEntry();
vector<int> getDisk(int row, int column);
void changeDisk(int index, int newValue, char *tName);
void pageReplace(vector<int> newPage, int frameNum, char *tName);
void thrOperations();
void *thrFunc(void* id);
void bubbleSort();
void quickSort(int first, int last);

void SC(vector<int> newPage, int frameNum, char *tName);
void LRU(vector<int> newPage, int frameNum, char *tName);
void WSClock(vector<int> newPage, int frameNum, char *tName);
void FIFO(vector<int> newPage, int frameNum, char *tName);
void NRU(vector<int> newPage, int frameNum, char *tName);

void findBestFrameSize();
void findBestAlgorithm();

int LRUcounter = 0;
//threshold for WSClock algorithm
unsigned long T = 100; //usec
int WSClock_ind = 0;

struct PageTableEntry{
    int referenced; //referenced
    int present; //present/absent
    int modified;
    int frameIndex;
    int LRUcounter;
	unsigned long accessTime;
};

//command line parameters
int frameSize;
int numPhysical;
int numVirtual;
int pageTablePrintInt;
char *pageReplacement;
char *tableType;
char *diskFileName;

vector<vector<int>> physicalMemory;
vector<vector<int>> virtualMemory;
struct PageTableEntry* PTEntry;

vector<int> pageFrameArr;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvSort = PTHREAD_COND_INITIALIZER;
pthread_cond_t cvMerge = PTHREAD_COND_INITIALIZER;

pthread_t threads[5];
bool sortFlag = false;
bool mergeFlag = false;

int pageReplBubble[7];
int pageReplQuick[7];
int counter1=0, counter2=0; //counter1 for bubble, counter2 for quick


int main(int argc, char **	argv){
	diskFileName = (char*)"diskFileName.dat";

	findBestFrameSize();

	findBestAlgorithm();


    if(pthread_mutex_destroy(&mutex)!=0){
        perror("pthread_mutex_destroy: ");
        exit(EXIT_FAILURE);
    }		

}

void findBestFrameSize(){
	int minBubble, minQuick, ind1, ind2;

	pageReplacement = (char*)"LRU";

	cout<<"***Finding best page frame size***"<<endl;

	for(int i=0; i<7; ++i){
		frameSize = pow(2, i);
		/*numPhysical = 16*1024/frameSize;
		numVirtual = 128*1024/frameSize;*/

		numPhysical = 64/frameSize;
		numVirtual = 128/frameSize;

		physicalMemory.clear();
		virtualMemory.clear();
		pageFrameArr.clear();

		counter1 = counter2 = 0;
	
		initPTEntry();
		fillMemories();

		thrOperations();

		cout<<"frameSize:\t"<<frameSize<<"\tpageFaultBubble:\t"<<counter1<<"\tpageFaultQuick:\t"<<counter2<<endl;

		pageReplBubble[i] = counter1;
		pageReplQuick[i] = counter2;
	}

	minBubble = minQuick = 2147483647;

	for(int i=0; i<7; ++i){
		if(minBubble > pageReplBubble[i]){
			minBubble = pageReplBubble[i];
			ind1 = i;
		}
		if(minQuick > pageReplQuick[i]){
			minQuick = pageReplQuick[i];	
			ind2 = i;	
		}
	}

	cout<<"\nBest frame size for bubble sort:\t"<<pow(2, ind1)<<endl;
	cout<<"Best frame size for quick sort:\t\t"<<pow(2, ind2)<<endl;
}

void findBestAlgorithm(){
	int minBubble, minQuick, ind1, ind2, pageFaultCounter;	

	cout<<"\n\n***Finding best page replacement algorithm***"<<endl;

	frameSize = 16;
	numPhysical = 64/frameSize;
	numVirtual = 128/frameSize;	

	/*frameSize = 128;
	numPhysical = 16*1024/frameSize;
	numVirtual = 128*1024/frameSize;*/



	for(int i=0; i<5; ++i){
		if(i == 0)
			pageReplacement = (char*)"SC";
		else if(i == 1)
			pageReplacement = (char*)"LRU";
		else if(i == 2)
			pageReplacement = (char*)"WSClock";
		else if(i == 3)
			pageReplacement = (char*)"FIFO";
		else if(i == 4)
			pageReplacement = (char*)"NRU";					

		physicalMemory.clear();
		virtualMemory.clear();
		pageFrameArr.clear();

		counter1 = counter2 = 0;

		initPTEntry();
		fillMemories();

		thrOperations();


		cout<<"Page Replacement Algorithm:\t"<<pageReplacement<<"\t\tpageFaultBubble:\t"<<counter1<<"\tpageFaultQuick:\t"<<counter2<<endl;

		pageReplBubble[i] = counter1;
		pageReplQuick[i] = counter2;
	}

	minBubble = minQuick = 2147483647;

	for(int i=0; i<3; ++i){
		if(minBubble > pageReplBubble[i]){
			minBubble = pageReplBubble[i];
			ind1 = i;
		}
		if(minQuick > pageReplQuick[i]){
			minQuick = pageReplQuick[i];	
			ind2 = i;	
		}
	}

	if(ind1 == 0)
		pageReplacement = (char*)"SC";
	else if(ind1 == 1)
		pageReplacement = (char*)"LRU";
	else if(ind1 == 2)
		pageReplacement = (char*)"WSClock";
	else if(ind1 == 3)
		pageReplacement = (char*)"FIFO";
	else
		pageReplacement = (char*)"NRU";	

	cout<<"\nBest page replacement algorithm for bubble sort:\t"<<pageReplacement<<endl;

	if(ind2 == 0)
		pageReplacement = (char*)"SC";
	else if(ind2 == 1)
		pageReplacement = (char*)"LRU";
	else if(ind2 == 2)
		pageReplacement = (char*)"WSClock";
	else if(ind2 == 3)
		pageReplacement = (char*)"FIFO";
	else
		pageReplacement = (char*)"NRU";

	cout<<"Best page replacement algorithm for quick sort:\t\t"<<pageReplacement<<endl;
}

//initializes page table entries
void initPTEntry(){
    PTEntry = (PageTableEntry*)malloc(numVirtual*sizeof(PageTableEntry));

	for(int i=0; i<numVirtual; ++i){
	    PTEntry[i].referenced = 0;
	    PTEntry[i].present = 0; 
	    PTEntry[i].modified = 0;
	    PTEntry[i].frameIndex = -1;
	    PTEntry[i].LRUcounter = 0;
		PTEntry[i].accessTime = 0;		
	}
}

//initializes virtual memory and physical memory
//fills physical memory with random numbers and writes same number to the disk to access later
void fillMemories(){
	int random, k=0;
	ofstream outdata;

	outdata.open(diskFileName);
	if(!outdata){ 
		cerr<< "File could not be opened!"<<endl;
		exit(EXIT_FAILURE);
	}

	srand(1000);

	for(int i=0; i<numVirtual; ++i){
		vector<int> temp, temp2;

		for(int j=0; j<frameSize; ++j){
			random = rand();

			temp.push_back(random);
			temp2.push_back(k++);

			outdata<<random<<endl;
		}

		if(i < numPhysical){
			physicalMemory.push_back(temp);
			pageFrameArr.push_back(i);
			PTEntry[i].present = 1;
			PTEntry[i].frameIndex = i;
		}
		virtualMemory.push_back(temp2);
	}

	outdata.close();
}

//creates 5 threads. first thread sorts first part by using bubble sort
//second thread sorts second part by using quick sort. 
void thrOperations(){
	int tid[2];

    for(int i=0; i<2; ++i) {
  		tid[i] = i;
        if(pthread_create(&threads[i], NULL, thrFunc, (void *)&tid[i]) != 0) { 
            perror("pthread_create: ");
            exit(EXIT_FAILURE);
        }
    }

    //waits until sorting threads done
    for(int i=0; i<2; ++i) {
        if(pthread_join(threads[i], NULL) == -1) { 
            perror("pthread_join: ");
            exit(EXIT_FAILURE);
        }
    }
}

void *thrFunc(void* id){
	int thrNum = *(int*)id;

	if(thrNum == 0) //sorts first part
		bubbleSort();
	else if(thrNum == 1) //sorts second part
		quickSort(numVirtual*frameSize/2, numVirtual*frameSize-1);
}

//sorts first part by using bubble sort
void bubbleSort(){
	int temp, size = numVirtual*frameSize/2, frameNum;

    for(int i=0; i <size-1; ++i){     
	    for(int j=0; j<size-i-1; ++j){
	        if(get(j, (char*)"bubble") > get(j+1, (char*)"bubble")){
	        	temp = get(j, (char*)"bubble");
	        	set(j, get(j+1, (char*)"bubble"), (char*)"bubble");
	        	set(j+1, temp, (char*)"bubble");
	        }
	    }
	}
}

//sorts second part by using quick sort
void quickSort(int first, int last){
	int i, j, pivot, temp, frameNum;

	if(first < last){
		i = first;
		j = last;
		pivot = first;

		while(i < j){ 
			while(i<last && get(i, (char*)"quick") <= get(pivot, (char*)"quick"))
				++i;
			while(get(j, (char*)"quick") > get(pivot, (char*)"quick"))
				--j;
			
			if(i < j){
				temp = get(i, (char*)"quick");
				set(i, get(j, (char*)"quick"), (char*)"quick");
				set(j, temp, (char*)"quick");
			}
		}

		temp = get(pivot, (char*)"quick");
		set(pivot, get(j, (char*)"quick"), (char*)"quick");
		set(j, temp, (char*)"quick");

		quickSort(first, j-1);
		quickSort(j+1, last);
	}
}

//changes disk elements while sorting
void changeDisk(int index, int newValue, char *tName){
	ofstream o1, o2;
	ifstream i1, i2;
	string line = "";
	int lineIndex = 0;

    i1.open(diskFileName);	
	o1.open("temp.txt");

	if(!o1 || !i1){ 
		cerr<<"File could not be opened!"<<endl;
		exit(EXIT_FAILURE);
	}

	while(getline(i1, line))
		o1<<line<<endl;

	i1.close();
	o1.close();

	i2.open("temp.txt");
    o2.open(diskFileName);

	if(!o2 || !i2){ 
		cerr<<"File could not be opened!"<<endl;
		exit(EXIT_FAILURE);
	}    	

	int counter = 0;

	while(getline(i2, line)){
		if(counter == index)
			o2<<newValue<<endl;	
		else
			o2<<line<<endl;	
		++counter;
	}

	remove("temp.txt");
}

//gets an element in pyhsical memory. index is address of element.
//if element is exist in ohysical memory it returns it
//otherwise it goes to disk and apply page replacement algorithm 
int get(unsigned int index, char *tName){
	int row, column, address, retVal;
	
	row = index/frameSize;
	column = index%frameSize;

	if(pthread_mutex_lock(&mutex) != 0){
		perror("pthread_mutex_lock0: ");
		exit(EXIT_FAILURE);
	}

	PTEntry[row].referenced = 1;
	PTEntry[row].LRUcounter = LRUcounter++;
	PTEntry[row].accessTime = getCurrentTime();

	if(PTEntry[row].present == 1){ //hit
		row = PTEntry[row].frameIndex;
		retVal = physicalMemory[row][column];

		if(pthread_mutex_unlock(&mutex) != 0){
			perror("pthread_mutex_unlock: ");
			exit(EXIT_FAILURE);			
		}

		return retVal;
	}
	else{ //miss
		vector<int> newPage = getDisk(row, column);

		pageReplace(newPage, row, tName);

		if(pthread_mutex_unlock(&mutex) != 0){
			perror("pthread_mutex_unlock: ");
			exit(EXIT_FAILURE);			
		}

		return newPage[column];
	}
}

//if element is not in physical memory, then gets element from disk 
vector<int> getDisk(int row, int column){
	int lineNum = row*frameSize;
	ifstream read;
	string sLine = "";
	vector<int> pageFrame;

    read.open(diskFileName);

	int lineIndex = 0;
	while (lineIndex<lineNum && getline(read, sLine))
		++lineIndex;

	for(int i=0; i<frameSize; ++i){
		getline(read, sLine);
		pageFrame.push_back(stoi(sLine));
	}   

	return pageFrame;
}

//sets a value at specified index
void set(unsigned int index, int value, char *tName){
	int row, column, address, present;

	row = index/frameSize;
	column = index%frameSize;

	if(pthread_mutex_lock(&mutex) != 0){
		perror("pthread_mutex_lock1: ");
		exit(EXIT_FAILURE);
	}	

	present = PTEntry[row].present;

	if(pthread_mutex_unlock(&mutex) != 0){
		perror("pthread_mutex_unlock: ");
		exit(EXIT_FAILURE);
	}		

	if(present == 0)
		get(index, (char*)"bubble");

	if(pthread_mutex_lock(&mutex) != 0){
		perror("pthread_mutex_lock2: ");
		exit(EXIT_FAILURE);
	}	

	physicalMemory[PTEntry[row].frameIndex][column] = value;
	changeDisk(index, value, tName);

	if(pthread_mutex_unlock(&mutex) != 0){
		perror("pthread_mutex_unlock: ");
		exit(EXIT_FAILURE);
	}	
}

//calls relevant algorithm
void pageReplace(vector<int> newPage, int frameNum, char *tName){
	if(strcmp(tName,"bubble"))
		++counter1;
	else if(strcmp(tName,"quick"))
		++counter2;

	if(strcmp(pageReplacement,"SC") == 0)
		SC(newPage, frameNum, tName);
	else if(strcmp(pageReplacement,"LRU") == 0)
		LRU(newPage, frameNum, tName);
	else if(strcmp(pageReplacement,"WSClock") == 0)
		WSClock(newPage, frameNum, tName);
	else if(strcmp(pageReplacement,"FIFO") == 0)
		FIFO(newPage, frameNum, tName);	
	else if(strcmp(pageReplacement,"NRU") == 0)
		NRU(newPage, frameNum, tName);		
}

//this algorithm likes FIFO. SO it chechs first frame. if it is referenced, then it removes it and puts it to end
//it checks next page so it checks pages until conditions are meet. when conditions are met, it breaks the loop.
void SC(vector<int> newPage, int frameNum, char *tName){
	vector<int> page; 
	int frame;

	while(1){
		frame = pageFrameArr[0];

		//it this page is referenced
		if(PTEntry[frame].referenced){
			page = physicalMemory[0]; //takes first page
			frame = pageFrameArr[0]; //takes first page's frame index

			physicalMemory.erase(physicalMemory.begin()); //removes page 
			physicalMemory.insert(physicalMemory.end(), page); //puts it to the end

			PTEntry[frame].frameIndex = pageFrameArr.size() -1; //puts the page to end so page's index is last index now
			PTEntry[frame].referenced = 0; //changes referenced bit with 0
			PTEntry[frame].present = 1; 

			//changes page's frame index in pageFrameArr
			pageFrameArr.erase(pageFrameArr.begin()); 
			pageFrameArr.insert(pageFrameArr.end(), frame);	

			//uodates pages' frame indexes
			for(int i=0; i<pageFrameArr.size()-1; ++i){
				frame = pageFrameArr[i];
				PTEntry[frame].frameIndex -= 1;
			}			
		}
		//else this page will remove
		else{
			//removes page from page table so it is not present anymore
			PTEntry[frame].frameIndex = -1;
			PTEntry[frame].present = 0; 
			PTEntry[frame].referenced = 0; 

			physicalMemory.erase(physicalMemory.begin()); //removes page 
			physicalMemory.insert(physicalMemory.begin(), newPage);	//puts the new page 

			//update page frame index
			PTEntry[frameNum].frameIndex = 0;
			PTEntry[frameNum].present = 1;			
			PTEntry[frameNum].referenced = 1;			

			//changes page's frame index in pageFrameArr
			pageFrameArr.erase(pageFrameArr.begin());
			pageFrameArr.insert(pageFrameArr.begin(), frameNum);

			break;			
		}
	}
}

//it deletes the pages according to their counter
//whenever getting a page, its counter increases, it means this page is used recently
//so it deletes smallest counter page which is least recently used
void LRU(vector<int> newPage, int frameNum, char *tName){
	int min = 2147483647, index;

	//find smallest counter number to replace
	for(int i=0; i<pageFrameArr.size(); ++i){
		if(PTEntry[pageFrameArr[i]].LRUcounter < min){
			min = PTEntry[pageFrameArr[i]].LRUcounter;
			index = i;			
		}
	}

	//sets frame index and present bit of page to be deleted
	PTEntry[pageFrameArr[index]].frameIndex = -1;
	PTEntry[pageFrameArr[index]].present = 0;	
	PTEntry[pageFrameArr[index]].referenced = 0;	

	//set new page's index and present bit
	PTEntry[frameNum].frameIndex = index;
	PTEntry[frameNum].present = 1;
	PTEntry[frameNum].referenced = 1;

	//update page frame in page table
	pageFrameArr[index] = frameNum;

	//delete old page, inserts new page
	physicalMemory.erase(physicalMemory.begin() + index);
	physicalMemory.insert(physicalMemory.begin() + index, newPage);
}

//it deletes pages according to their last access time and R bits
//first, it checks page's R bit. if it is 1, it changes it with 0 and it changes its accessTime with current time,
//then checks next page. when a page's R bit is 0, it checks (current time - page's last access time)
//if this value is bigger than threshold which is 100 usec, then it replaces this page with new page
void WSClock(vector<int> newPage, int frameNum, char *tName){
	int frame, index;
	unsigned long currTime;

	//it checks until conditions met
	while(1){
		frame = pageFrameArr[WSClock_ind];

		//if R bit is 1, it changes it with 0
		if(PTEntry[frame].referenced == 1){
			PTEntry[frame].referenced = 0;
			PTEntry[frame].accessTime = getCurrentTime();
		}
		else{
			unsigned long currTime = getCurrentTime();	

			//checks page age
			if((currTime - PTEntry[frame].accessTime) > T){
				index = WSClock_ind;
				break;
			}			
		}	
		//nex page
		++WSClock_ind;

		if(WSClock_ind == pageFrameArr.size())
			WSClock_ind = 0;
	}

	//sets frame index and present bit of page to be deleted
	PTEntry[pageFrameArr[index]].frameIndex = -1;
	PTEntry[pageFrameArr[index]].present = 0;	
	PTEntry[pageFrameArr[index]].referenced = 0;	

	//set new page's index and present bit
	PTEntry[frameNum].frameIndex = index;
	PTEntry[frameNum].present = 1;
	PTEntry[frameNum].referenced = 1;

	//update page frame in page table
	pageFrameArr[index] = frameNum;

	//delete old page, inserts new page
	physicalMemory.erase(physicalMemory.begin() + index);
	physicalMemory.insert(physicalMemory.begin() + index, newPage);	
	
}

//first in first out, so when page fault occures, it removes first page all the time.
void FIFO(vector<int> newPage, int frameNum, char *tName){
	int oldFrame = pageFrameArr[0];

	//incWriteNum(tName);

	physicalMemory.erase(physicalMemory.begin()); //removes page 
	physicalMemory.insert(physicalMemory.end(), newPage); //puts it to the end

	//changes page's frame index in pageFrameArr
	pageFrameArr.erase(pageFrameArr.begin());
	pageFrameArr.insert(pageFrameArr.end(), frameNum);

	//update page frame indexes
	for(int i=0; i<numVirtual; ++i){
		for(int j=0; j<pageFrameArr.size(); ++j){
			if(pageFrameArr[j] == i)
				PTEntry[i].frameIndex = j;
		}
	}

	PTEntry[frameNum].frameIndex = physicalMemory.size()-1;
	PTEntry[frameNum].present = 1;
	PTEntry[frameNum].referenced = 1;		

	PTEntry[oldFrame].frameIndex = -1;
	PTEntry[oldFrame].present = 0;
	PTEntry[oldFrame].referenced = 0;	
}

void NRU(vector<int> newPage, int frameNum, char *tName){
	vector<int> class0, class1, class2, class3, class_;

	//find each page class
	for(int i=0; i<pageFrameArr.size(); ++i){
		if(PTEntry[pageFrameArr[i]].referenced == 0 && PTEntry[pageFrameArr[i]].modified == 0)
			class0.push_back(i);
		else if(PTEntry[pageFrameArr[i]].referenced == 0 && PTEntry[pageFrameArr[i]].modified == 1)
			class1.push_back(i);
		else if(PTEntry[pageFrameArr[i]].referenced == 1 && PTEntry[pageFrameArr[i]].modified == 0)
			class2.push_back(i);
		else
			class3.push_back(i);					
	}

	srand(1000);
	int rand;

	//find lowes class
	if(class0.size() != 0)
		class_ = class0;
	else if(class1.size() != 0)
		class_ = class1;
	else if(class2.size() != 0)
		class_ = class2;
	else
		class_ = class3;

	//choose a random page
	rand = random();
	rand = rand%class_.size();

	//sets frame index and present bit of page to be deleted
	PTEntry[pageFrameArr[rand]].frameIndex = -1;
	PTEntry[pageFrameArr[rand]].present = 0;	
	PTEntry[pageFrameArr[rand]].referenced = 0;	

	//set new page's index and present bit
	PTEntry[frameNum].frameIndex = rand;
	PTEntry[frameNum].present = 1;
	PTEntry[frameNum].referenced = 1;

	//update page frame in page table
	pageFrameArr[rand] = frameNum;

	//delete old page, inserts new page
	physicalMemory.erase(physicalMemory.begin() + rand);
	physicalMemory.insert(physicalMemory.begin() + rand, newPage);	
}

//gets current time for wsclock algorithm
unsigned long getCurrentTime(){
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec/1000000 + tv.tv_usec ;
}


