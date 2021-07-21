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
void initStats();
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
void merge();
void linearSearch();
void binarySearch();
int binarySearchHelper(int left, int right, int num);
void printStats();

void SC(vector<int> newPage, int frameNum, char *tName);
void LRU(vector<int> newPage, int frameNum, char *tName);
void WSClock(vector<int> newPage, int frameNum, char *tName);
void FIFO(vector<int> newPage, int frameNum, char *tName);
void NRU(vector<int> newPage, int frameNum, char *tName);

void incReadNum(char *tName);
void incWriteNum(char *tName);
void incMissNum(char *tName);
void incRepNum(char *tName);
void incDiskWriteNum(char *tName);
void incDiskReadNum(char *tName);
void addWorkingSet(char *tName);

int LRUcounter = 0;
//threshold for WSClock algorithm
unsigned long T = 100; //usec
int WSClock_ind = 0;
int memAccesCounter = 0;	//it increases every memory access happend
							//When it is equal to pageTablePrintInt, it becomes 0 again

struct PageTableEntry{
    int referenced; //referenced
    int present; //present/absent
    int modified;
    int frameIndex;
    int LRUcounter;
	unsigned long accessTime;
};

struct Statistics{
    int read;
    int write;
    int miss;
    int replacement;
    int diskWrite;
    int diskRead;
    vector<vector<int>> workingSet;
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
struct Statistics stats[6]; //bubble, quick, merge, linear, binary, fill

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cvSort = PTHREAD_COND_INITIALIZER;
pthread_cond_t cvMerge = PTHREAD_COND_INITIALIZER;

pthread_t threads[5];
bool sortFlag = false;
bool mergeFlag = false;

int main(int argc, char **argv){
	int index = 0;

	parseCL(argc, argv);
	initPTEntry();
	fillMemories();
	initStats();

	cout<<"\nBefore sorting:"<<endl;
	for (int i = 0; i < numVirtual; i++){
		for (int j = 0; j < frameSize; j++)
			cout<<get(index++, (char*)"")<<"\t";
		cout<<endl;
	}
	cout<<endl;

	thrOperations();

	index = 0;

	cout<<"\nAfter sorting:"<<endl;
	for (int i = 0; i < numVirtual; i++){
		for (int j = 0; j < frameSize; j++)
			cout<<get(index++, (char*)"")<<"\t";
		cout<<endl;		
	}
	cout<<endl;

	printStats();

    if(pthread_mutex_destroy(&mutex)!=0){
        perror("pthread_mutex_destroy: ");
        exit(EXIT_FAILURE);
    }

    free(PTEntry);	
}

//parses command line arguments
void parseCL(int argc, char **argv){
	if(argc != 8)
		usageError();

	frameSize = atoi(argv[1]);
	numPhysical = atoi(argv[2]);
	numVirtual = atoi(argv[3]);
	pageReplacement = argv[4];
	tableType = argv[5];
	pageTablePrintInt = atoi(argv[6]);
	diskFileName = argv[7];
	
	if(frameSize<0 || numPhysical<0 || numVirtual<0 || numVirtual<numPhysical)
		usageError();

	if(!((strcmp(pageReplacement,"SC") == 0) || (strcmp(pageReplacement,"LRU") == 0) || (strcmp(pageReplacement,"WSClock") == 0) || (strcmp(pageReplacement,"FIFO") == 0) || (strcmp(pageReplacement,"NRU") == 0))){
		cerr<<"pageReplacement can be SC, LRU, NRU, FIFO, or WSClock"<<endl;
		exit(EXIT_FAILURE);
	}

	frameSize = pow(2, frameSize);
	numPhysical = pow(2, numPhysical);
	numVirtual = pow(2, numVirtual);	
}

void usageError(){
	cout<<"You entered wrong command line arguments. Command line structure:\n";
	cout<<"./sortArrays <frameSize> <numPhysical> <numVirtual> <pageReplacement> <tableType> <pageTablePrintInt> <diskFileName.dat>\n";
	cout<<"frameSize, numVirtual, and numPhysical must be bigger than 0.\n";
	cout<<"numVirtual must be bigger than numPhysical.\n";
	exit(EXIT_FAILURE);
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
		incWriteNum((char*)"fill"); 
		incDiskWriteNum((char*)"fill");
	}

	outdata.close();
}

//initializes statistics
void initStats(){
	for(int i=0; i<5; ++i){
		stats[i].read = 0;
		stats[i].write = 0;
		stats[i].miss = 0;
		stats[i].replacement = 0;
		stats[i].diskRead = 0;
		stats[i].diskWrite = 0;
	}	
}

//creates 5 threads. first thread sorts first part by using bubble sort
//second thread sorts second part by using quick sort. third thread merge two parts
//fourth and fifth threads search with linear search and binary search respectively
//merge thread waits for sorting operations. linear search and binary search threads wait for merge operation
void thrOperations(){
	int tid[5];

    for(int i=0; i<5; ++i) {
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

    sortFlag = true;

    //send dignal to notify merge thread
	if(pthread_cond_signal(&cvSort) !=0 ) {
	    perror("pthread_cond_signal: ");             
	    exit(EXIT_FAILURE);
	}     

    for(int i=2; i<5; ++i) {
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
	else if(thrNum == 2) //merges two parts
		merge();
	else if(thrNum == 3) //searchs array with linear search
		linearSearch();
	else //searchs array with binary search
		binarySearch();
}

//waits until sorting operation is done. then merges two parts
//after merging, send cvMerge signal to indicate merging is done
void merge(){
	int left, right, mid, i, j, k, N;
	left = 0;
	right = numVirtual*frameSize-1;
	mid = right/2;

	N = numVirtual*frameSize/2;
    int leftArr[N], rightArr[N]; 

    if(pthread_mutex_lock(&mutex) != 0){
        perror("pthread_mutex_lock: ");              
        exit(EXIT_FAILURE);
    }      

    while(!sortFlag){
        if(pthread_cond_wait(&cvSort, &mutex) != 0){
            perror("pthread_cond_wait: ");                
            exit(EXIT_FAILURE);
        } 
    }

    if(pthread_mutex_unlock(&mutex) != 0){
        perror("pthread_mutex_unlock: ");             
        exit(EXIT_FAILURE);
    }     
  
    for(i=0; i<N; ++i) 
        leftArr[i] = get(left+i, (char*)"merge"); 
    for(j=0; j<N; ++j) 
        rightArr[j] = get(mid+1+j, (char*)"merge");

    i = j = 0;
    k = left;

    while(i<N && j<N) { 
        if(leftArr[i] <= rightArr[j])
            set(k, leftArr[i++], (char*)"merge"); 
        else
        	set(k, rightArr[j++], (char*)"merge"); 
        ++k; 
    } 

    while(i < N)
        set(k++, leftArr[i++], (char*)"merge"); 

    while(j < N)
    	set(k++, rightArr[j++], (char*)"merge");  

    mergeFlag = true;   

	if(pthread_cond_broadcast(&cvMerge) !=0 ) {
	    perror("pthread_cond_broadcast: ");             
	    exit(EXIT_FAILURE);
	}    
} 

//searchs an element with linear search
//if it exists, writes its address
void linearSearch(){
	int arr[5] = {173226398, 3, 1570801147, 71, 2121871803}, flag;

    if(pthread_mutex_lock(&mutex) != 0){
        perror("pthread_mutex_lock: ");              
        exit(EXIT_FAILURE);
    }      

    while(!mergeFlag){
        if(pthread_cond_wait(&cvMerge, &mutex) != 0){
            perror("pthread_cond_wait: ");                
            exit(EXIT_FAILURE);
        } 
    }

    if(pthread_mutex_unlock(&mutex) != 0){
        perror("pthread_mutex_unlock: ");             
        exit(EXIT_FAILURE);
    } 

    cout<<"\n*****Linear Search*****"<<endl;

   	for(int i=0; i<5; ++i){
   		cout<<"Searched number: "<<arr[i]<<endl;
   		flag = 0;

   		for(int j=0; j<numVirtual*frameSize; ++j){
   			if(arr[i] == get(j, (char*)"linear")){
   				cout<<arr[i]<<" is at address "<<j<<".\n"<<endl;
   				flag = 1;
   			}	
   		}

		if(!flag)
			cout<<arr[i]<<" is not present in this array.\n"<<endl;   		
   	}
}

//searchs an element with binary search
//if it exists, writes its address
void binarySearch(){
	int arr[5] = {173226398, 3, 1570801147, 71, 2121871803}, res;

    if(pthread_mutex_lock(&mutex) != 0){
        perror("pthread_mutex_lock: ");              
        exit(EXIT_FAILURE);
    }      

    while(!mergeFlag){
        if(pthread_cond_wait(&cvMerge, &mutex) != 0){
            perror("pthread_cond_wait: ");                
            exit(EXIT_FAILURE);
        } 
    }

    if(pthread_mutex_unlock(&mutex) != 0){
        perror("pthread_mutex_unlock: ");             
        exit(EXIT_FAILURE);
    } 

     cout<<"\n*****Binary Search*****"<<endl;

   	for(int i=0; i<5; ++i){
   		cout<<"Searched number: "<<arr[i]<<endl;  

    	res = binarySearchHelper(0, numVirtual*frameSize-1, arr[i]);

		if(res >= 0)
			cout<<arr[i]<<" is at address "<<res<<".\n"<<endl;
		else
			cout<<arr[i]<<" is not present in this array.\n"<<endl; 
	}    	
}

int binarySearchHelper(int left, int right, int num){
	int mid;

    if (left <= right){
        mid = left + (right-left)/2;
 
        //if number is at mid element
        if(num == get(mid, (char*)"binary"))
            return mid;

        //if number is smaller than mid element
        if(num < get(mid, (char*)"binary"))
            return binarySearchHelper(left, mid-1, num);
 
		//if number is bigger than mid element
        return binarySearchHelper(mid+1, right, num);
    }

    return -1;
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
			while(i<last && get(i, (char*)"quick") <= get(pivot, (char*)"quick")) ++i;
			while(get(j, (char*)"quick") > get(pivot, (char*)"quick")) --j;
			
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

//prints statistics
void printStats(){
	cout<<"**********Statistics**********"<<endl;
	for(int i=0; i<6; ++i){
		if(i == 0)
			cout<<"Bubble Sort"<<endl;
		else if(i == 1)
			cout<<"\nQuick Sort"<<endl;
		else if(i == 2)
			cout<<"\nMerge"<<endl;
		else if(i == 3)
			cout<<"\nLinear Search"<<endl;
		else if(i == 4)
			cout<<"\nBinary Search"<<endl;
		else
			cout<<"\nFill"<<endl;

		cout<<"Number of reads:\t\t"<<stats[i].read<<endl;
		cout<<"Number of writes:\t\t"<<stats[i].write<<endl;
		cout<<"Number of page misses:\t\t"<<stats[i].miss<<endl;
		cout<<"Number of page replacements:\t"<<stats[i].replacement<<endl;
		cout<<"Number of disk page writes:\t"<<stats[i].diskWrite<<endl;
		cout<<"Number of disk page reads:\t"<<stats[i].diskRead<<endl;	

		cout<<"Working sets:"<<endl;
		for(int j=0; j<(stats[i].workingSet).size(); ++j){
			cout<<"w"<<j<<":\t";
			for(int k=0; k<(stats[i].workingSet[j]).size(); ++k){
				cout<<stats[i].workingSet[j][k]<<" ";
			}
			cout<<endl;
		}
	}	
}

//prints page table entries
void printPageTable(){
    cout<<"\n*****Page Table*****"<<endl;

    for(int i=0; i<numVirtual; ++i)
    	cout<<"Frame Index:\t"<<i<<"\tR bit:\t"<<PTEntry[i].referenced<<"\tM bit:\t"<<PTEntry[i].modified<<"\tPresent/absent bit:\t"<<PTEntry[i].present<<"\tFrame Index in Physical Memory:\t"<<PTEntry[i].frameIndex<<"\tLRU Counter:\t"<<PTEntry[i].LRUcounter<<"\tLast Access Time:\t"<<PTEntry[i].accessTime<<endl;
    cout<<endl;
}

//changes disk elements while sorting
void changeDisk(int index, int newValue, char *tName){
	ofstream o1, o2;
	ifstream i1, i2;
	string line = "";
	int lineIndex = 0, row;

	incDiskWriteNum(tName); //increase disk write statistics of relevant thread

	//change modified bit
	row = index/frameSize;
	PTEntry[row].modified = 0;

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

	//if memAccesCounter is equal pageTablePrintInt, prints page table entries
	//then makes memAccesCounter 0
	if(memAccesCounter++ == pageTablePrintInt){
		memAccesCounter = 0;
		printPageTable();
	}	

	//increase read statistics of relevant thread
	incReadNum(tName);
	
	row = index/frameSize;
	column = index%frameSize;

	if(pthread_mutex_lock(&mutex) != 0){
		perror("pthread_mutex_lock: ");
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

		incMissNum(tName); //increase miss statistics of relevant thread
		incDiskReadNum(tName); //increase disk read statistics of relevant thread
		incRepNum(tName); //increase page repl statistics of relevant thread
		addWorkingSet(tName);

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

	if(memAccesCounter++ == pageTablePrintInt){
		memAccesCounter = 0;
		printPageTable();
	}	

	incWriteNum(tName); //increase write statistics of relevant thread

	row = index/frameSize;
	column = index%frameSize;

	if(pthread_mutex_lock(&mutex) != 0){
		perror("pthread_mutex_lock: ");
		exit(EXIT_FAILURE);
	}	

	present = PTEntry[row].present;
	PTEntry[row].modified = 1;

	if(pthread_mutex_unlock(&mutex) != 0){
		perror("pthread_mutex_unlock: ");
		exit(EXIT_FAILURE);
	}		

	if(present == 0)
		get(index, (char*)"bubble");

	if(pthread_mutex_lock(&mutex) != 0){
		perror("pthread_mutex_lock: ");
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

	//incWriteNum(tName); //increase write statistics of relevant thread

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

	//incWriteNum(tName); //increase write statistics of relevant thread

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

	//incWriteNum(tName);	//increase write statistics of relevant thread

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

//increase read statistics of relevant thread
void incReadNum(char *tName){
	if(strcmp(tName, "bubble") == 0)	
		++stats[0].read;
	else if(strcmp(tName, "quick") == 0)	
		++stats[1].read;
	else if(strcmp(tName, "merge") == 0)	
		++stats[2].read;			
	else if(strcmp(tName, "linear") == 0)	
		++stats[3].read;
	else if(strcmp(tName, "binary") == 0)	
		++stats[4].read;	
	else if(strcmp(tName, "fill") == 0)		
		++stats[5].read;			
}

//increase write statistics of relevant thread
void incWriteNum(char *tName){
	if(strcmp(tName, "bubble") == 0)	
		++stats[0].write;
	else if(strcmp(tName, "quick") == 0)	
		++stats[1].write;
	else if(strcmp(tName, "merge") == 0)	
		++stats[2].write;			
	else if(strcmp(tName, "linear") == 0)	
		++stats[3].write;
	else if(strcmp(tName, "binary") == 0)	
		++stats[4].write;	
	else if(strcmp(tName, "fill") == 0)		
		++stats[5].write;			
}

//increase miss statistics of relevant thread
void incMissNum(char *tName){
	if(strcmp(tName, "bubble") == 0)	
		++stats[0].miss;
	else if(strcmp(tName, "quick") == 0)	
		++stats[1].miss;
	else if(strcmp(tName, "merge") == 0)	
		++stats[2].miss;			
	else if(strcmp(tName, "linear") == 0)	
		++stats[3].miss;
	else if(strcmp(tName, "binary") == 0)	
		++stats[4].miss;	
	else if(strcmp(tName, "fill") == 0)		
		++stats[5].miss;			
}

//increase page repl statistics of relevant thread
void incRepNum(char *tName){
	if(strcmp(tName, "bubble") == 0)	
		++stats[0].replacement;
	else if(strcmp(tName, "quick") == 0)	
		++stats[1].replacement;
	else if(strcmp(tName, "merge") == 0)	
		++stats[2].replacement;			
	else if(strcmp(tName, "linear") == 0)	
		++stats[3].replacement;
	else if(strcmp(tName, "binary") == 0)	
		++stats[4].replacement;	
	else if(strcmp(tName, "fill") == 0)		
		++stats[5].replacement;			
}

//increase disk read statistics of relevant thread
void incDiskReadNum(char *tName){
	if(strcmp(tName, "bubble") == 0)	
		++stats[0].diskRead;
	else if(strcmp(tName, "quick") == 0)	
		++stats[1].diskRead;
	else if(strcmp(tName, "merge") == 0)	
		++stats[2].diskRead;			
	else if(strcmp(tName, "linear") == 0)	
		++stats[3].diskRead;
	else if(strcmp(tName, "binary") == 0)	
		++stats[4].diskRead;	
	else if(strcmp(tName, "fill") == 0)	
		++stats[5].diskRead;			
}

//increase disk write statistics of relevant thread
void incDiskWriteNum(char *tName){
	if(strcmp(tName, "bubble") == 0)	
		++stats[0].diskWrite;
	else if(strcmp(tName, "quick") == 0)	
		++stats[1].diskWrite;
	else if(strcmp(tName, "merge") == 0)	
		++stats[2].diskWrite;			
	else if(strcmp(tName, "linear") == 0)	
		++stats[3].diskWrite;
	else if(strcmp(tName, "binary") == 0)	
		++stats[4].diskWrite;	
	else if(strcmp(tName, "fill") == 0)		
		++stats[5].diskWrite;			
}

//add new working set to working set vector of relevant thread
void addWorkingSet(char *tName){
	if(strcmp(tName, "bubble") == 0)	
		(stats[0].workingSet).push_back(pageFrameArr);
	else if(strcmp(tName, "quick") == 0)	
		(stats[1].workingSet).push_back(pageFrameArr);
	else if(strcmp(tName, "merge") == 0)	
		(stats[2].workingSet).push_back(pageFrameArr);		
	else if(strcmp(tName, "linear") == 0)	
		(stats[3].workingSet).push_back(pageFrameArr);
	else if(strcmp(tName, "binary") == 0)	
		(stats[4].workingSet).push_back(pageFrameArr);	
	else if(strcmp(tName, "fill") == 0)		
		(stats[5].workingSet).push_back(pageFrameArr);			
}

//gets current time for wsclock algorithm
unsigned long getCurrentTime(){
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return tv.tv_sec/1000000 + tv.tv_usec ;
}
