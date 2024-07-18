#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>

using namespace std;

#define LINELENGTH 10

#define PAGESIZE 256
vector<int> pageTable(PAGESIZE, -1);
vector<int> pageFrame(PAGESIZE, -1);

#define TLB_LENGTH 16
vector<int> TLBPage(TLB_LENGTH, -1);
vector<int> TLBFrame(TLB_LENGTH, -1);
int TLBNum = 0;
int TLBCounter = 0;

#define FRAMELENGTH 256
vector<char> readBacker(FRAMELENGTH);

#define physicalMemoryBytes 65536
vector<int> physicalMemory(physicalMemoryBytes, 0);
int pageFault = 0;

// Function to read from the BACKING_STORE file into the readBacker array and load into physical memory
int readBackStore(int page, ifstream &backStore)
{
	if (!backStore)
	{
		cerr << "ERROR: Unable to seek in BACKING_STORE.bin" << endl;
		return -1;
	}

	backStore.seekg(page * PAGESIZE, ios::beg);
	backStore.read(readBacker.data(), PAGESIZE);

	int availableFrame = -1;
	for (int i = 0; i < PAGESIZE; ++i)
	{
		if (pageFrame[i] == -1)
		{
			pageFrame[i] = 0;
			availableFrame = i;
			break;
		}
	}

	if (availableFrame == -1)
	{
		cerr << "ERROR: No available frames in memory" << endl;
		return -1;
	}

	int startFrameIndex = PAGESIZE * availableFrame;
	for (int j = 0; j < PAGESIZE; ++j)
	{
		physicalMemory[startFrameIndex + j] = readBacker[j];
	}

	pageTable[page] = availableFrame;
	pageFault++;

	return availableFrame;
}

// Function to translate logical address to physical address
int changeAddress(int logAddress, ifstream &backStore)
{
	int page = logAddress / PAGESIZE;
	int offset = logAddress % PAGESIZE;

	int frameNum = -1;
	for (int i = 0; i < TLB_LENGTH; ++i)
	{
		if (TLBPage[i] == page)
		{
			frameNum = TLBFrame[i];
			TLBNum++;
			break;
		}
	}

	if (frameNum == -1)
	{
		if (pageTable[page] == -1)
		{
			frameNum = readBackStore(page, backStore);
		}
		else
		{
			frameNum = pageTable[page];
		}

		TLBPage[TLBCounter % TLB_LENGTH] = page;
		TLBFrame[TLBCounter % TLB_LENGTH] = frameNum;
		TLBCounter++;
	}

	return frameNum * PAGESIZE + offset;
}

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " addresses.txt" << endl;
		return 1;
	}

	ifstream backStore("BACKING_STORE.bin", ios::binary);
	if (!backStore)
	{
		cerr << "ERROR: Unable to open BACKING_STORE.bin" << endl;
		return 1;
	}

	ifstream addressFile(argv[1]);
	if (!addressFile)
	{
		cerr << "ERROR: Unable to open " << argv[1] << endl;
		return 1;
	}

	int translations = 0, logAddress = 0, address = 0;
	string line;

	while (getline(addressFile, line))
	{
		logAddress = stoi(line);
		address = changeAddress(logAddress, backStore);

		cout << "Logical Address: " << logAddress
			 << ", Physical Memory: " << address
			 << ", Value: " << physicalMemory[address] << endl;

		translations++;
	}

	// Print out results
	cout << "\n*** Final Info ***" << endl;
	cout << "Number of translations: " << translations << endl;
	cout << "Number of Page Faults: " << pageFault << endl;
	cout << "Page Fault Rate: " << static_cast<float>(pageFault) / translations << endl;
	cout << "Number of TLB Hits: " << TLBNum << endl;
	cout << "TLB Rate: " << static_cast<float>(TLBNum) / translations << endl;

	return 0;
}
