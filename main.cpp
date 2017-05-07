#include <iostream>
#include <fstream>
#include <vector>
using namespace std;

class Node {
public:
    int time;
    int jobId;
    Node *next;

    Node(int id, int time) {
        jobId = id;
        this->time = time;
        this->next = NULL;
    }

};

class Ordering {
public:
    //IO variables
    ifstream scan1;
    ifstream scan2;
    ofstream out1;
    ofstream out2;

    //data structures
    vector<vector<int>> scheduleTable;
    Node *OPEN;
    vector<Node *> hashTable;
    vector<int> processJob;
    vector<int> processTime;
    vector<int> parentCount;
    vector<int> jobTime;
    vector<int> jobDone;
    vector<int> jobMarked;

    //data variables for the program
    int ProcNeed;
    int ProcUsed;
    int Time;

    void destroy(){
        Node* current = OPEN;
        while( current != 0 ) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        OPEN = 0;

        for(int i=1;i<hashTable.capacity();i++){
            Node* current=hashTable[i];
            delete current;
        }
    }

    Ordering(int argc,char *argv[]) {
        //IO variables intialized
        scan1.open(argv[1]);
        scan2.open(argv[2]);
        out1.open(argv[3]);
        out2.open(argv[4]);

        int numberNodes;
        scan1 >> numberNodes;

        //hashTable
        hashTable = vector<Node *>(numberNodes + 1);
        for (int i = 1; i < hashTable.size(); i++) {
            Node *dummy = new Node(0, 0);
            hashTable[i] = dummy;
        }

        //all other arrays
        processJob = vector<int>(numberNodes + 1);
        processTime = vector<int>(numberNodes + 1);
        jobDone = vector<int>(numberNodes + 1);
        jobMarked = vector<int>(numberNodes + 1);
        parentCount = vector<int>(numberNodes + 1);
        jobTime = vector<int>(numberNodes + 1);

        //Open list
        OPEN = new Node(0, 0);


        int totalJobTime = 0;
        int job, time = 0;
        int data;
        scan2 >> data;
        bool isJob = true;
        while (scan2) {
            scan2 >> job;
            scan2 >> time;
            jobTime[job] = time;
            totalJobTime += time;
        }
        //scheduleTable
        scheduleTable = vector<vector<int>>(numberNodes + 1);
        for (int i = 0; i < scheduleTable.capacity(); i++) {
            scheduleTable[i] = vector<int>(totalJobTime + 1);
        }

        int parent;
        int child;
        Node *track;
        while (scan1) {
            scan1 >> parent;
            scan1 >> child;
            parentCount[child]++;

            track = hashTable[parent];
            while (track->next != NULL) {
                track = track->next;
            }

            Node *newNode = new Node(child, jobTime[child]);
            track->next = newNode;
        }
        //free(track);

        ProcUsed = 0;
        Time = 0;
        cout << "Input the number of processor need: " << "\n";
        cin >> ProcNeed;
        ProcNeed = 5;

        if (ProcNeed > numberNodes) {
            ProcNeed = numberNodes;
        }

    };


    void scheduling() {
        while(alljobNotDone()) {
            findOrphans();
            placedOrphansOnProcessors();
            debugPrinting("Printing before incrementing time unit");
            if (cycleExist())exit(1);
            processed();
            processJobPrint();
            processJobPrint();
            debugPrinting("Printing after incrementing time unit");
        }
        printScheduleTable(out1);
        close();//closing all IO streams
        destroy();
    }

    bool alljobNotDone() {
        for (int i = 1; i < jobDone.capacity(); i++) {
            if (jobDone[i] != 1) return true;
        }
        return false;
    }


    void findOrphans() {
        Node *track;
        for (int currentID = 1; currentID < parentCount.capacity(); currentID++) {
            if (parentCount[currentID] == 0 && jobMarked[currentID] == 0) {//found the orphan node
                jobMarked[currentID] = 1;//marked to ready to process
                Node *orphanNode = new Node(currentID, jobTime[currentID]);


                //put on open list
                track = OPEN;
                while (track->next != NULL) {
                    track = track->next;
                }//go to the end of open list
                track->next = orphanNode;//put the orphan Node on the end of the open list

            }//no parents and has not processed yet
        }
    }

    void placedOrphansOnProcessors() {
        int availibleProc = 0;
        Node *newJob;
        int jobId;
        //while open list is not empty and we use less than what we need
        while (OPEN->next != NULL && ProcUsed <= ProcNeed) {

            //find an available processor
            availibleProc = findAvailableProcessor();

            if (availibleProc >= 0) {
                newJob = OPEN->next;
                OPEN = OPEN->next;//remove job from open

                processJob[availibleProc] = newJob->jobId;
                processTime[availibleProc] = newJob->time;
                scheduleTable[availibleProc][Time] = processTime[availibleProc];
            } else {

                ProcUsed++;//open up more using processors
                availibleProc = ProcUsed - 1;//id of the new processor

                newJob = OPEN->next;
                OPEN = OPEN->next;//remove job from open

                processJob[availibleProc] = newJob->jobId;
                processTime[availibleProc] = newJob->time;
                scheduleTable[availibleProc][Time] = processTime[availibleProc];
            }
        }
    }

    int findAvailableProcessor() {
        for (int id = 0; id < ProcUsed - 1; id++) {
            if (processJob[id] <= 0) {
                return id;//unoccupied used processor
            }
        }
        return -1;
    }

    bool cycleExist() {
        if (OPEN->next == NULL) {//no more orphans
            for (int i = 1; i < jobDone.capacity(); i++) {
                if (jobDone[i] != 1) {//if a job is unfinished aka not Done
                    for (int j = 0; j < ProcUsed; j++) {
                        if (processJob[j] != 0) {
                            return false;
                        }
                    }
                    out2 << "Error: cycle exist"<<endl;
                    return true;
                }
            }
        }
        return false;
    }

    void processed() {
        Time++;
        Node* track;
        for (int i = 0; i < processTime.capacity(); i++) {
            if (processJob[i] > 0) {
                processTime[i]--;//decrement all time by 1
                //if finished
                if (processTime[i] == 0) {

                    //decrease the parent count of children from DG
                    track = hashTable[processJob[i]];
                    while (track->next != NULL) {
                        track = track->next;
                        parentCount[track->jobId]--;
                    }

                    jobDone[processJob[i]] = 1;//update job done
                    processJob[i] = 0;//delete that job from the processor

                }
                scheduleTable[i][Time] = processTime[i];
            }//do this for all finished jobs
        }

    }

    void debugPrinting(string message) {
        out2 << message << endl;
        printScheduleTable(out2);
        out2<< "ProcNeed :" << ProcNeed << endl;
        out2 << "ProcUsed :" << ProcUsed << endl;
        processJobPrint();
        allprocessTimePrint();
        allParentCountPrint();
        allJobTimePrint();
        jobDonePrint();
        jobMarkedPrint();
    }

    void close(){
        //closing all IO
        scan1.close();
        scan2.close();
        out1.close();
        out2.close();
    }

    //Debug Printing Functions

    void OPENprint() {
        Node *track;
        track = OPEN;
        out2 << "OPEN->";
        while (track->next != NULL) {
            track = track->next;
            out2 << track->jobId << "->";
        }
        out2 << "null\n";
    }

    void printScheduleTable(ofstream &out) {
        out << "ScheduleTable :" << endl;
        out << "Time: " << Time << endl;
        for (int i = 0; i <= ProcUsed; i++) {
            if (i < 10) out << "P" << i << "  ";
            else out << "P" << i << " ";
            for (int j = 0; j <= Time; j++) {
                out << "[" << scheduleTable[i][j] << "] ";
            }
            out << endl;
        }
    }

    void processJobPrint() {
        out2 << "Process Job Array: " << endl;
        out2 << "[Processor: Job ID]" << endl;
        for (int i = 0; i < processJob.capacity(); i++) {
            out2 << "[ " << i << " : " << processJob[i] << "] " << endl;
        }
        out2 << endl;
    }

    void allprocessTimePrint() {
        out2 << "Process time Array" << endl;
        out2 << "[Job ID: process time]" << endl;
        for (int i = 0; i < processTime.capacity(); i++) out2 << "[ " << i << " : " << processTime[i] << "] " << endl;
        out2 << endl;
    }

    void allParentCountPrint() {
        out2 << "Parent Count Array:" << endl;
        out2 << "[Job ID: Parent count]" << endl;
        for (int i = 1; i < parentCount.capacity(); i++) out2 << "[ " << i << " : " << parentCount[i] << "] " << endl;
        out2 << endl;
    }

    void allJobTimePrint() {
        out2 << "Job Time Array:" << endl;
        out2 << "[Job ID: time]" << endl;
        for (int i = 1; i < jobTime.capacity(); i++) {
            out2 << "[ " << i << " : " << jobTime[i] << "] " << endl;
        }
        out2 << endl;
    }

    void jobDonePrint() {
        out2 << " Jobs Done Array: " << endl;
        out2 << "[Job ID: Finish status]" << endl;
        for (int i = 1; i < jobDone.capacity(); i++) {
            out2 << "[ " << i << " : " << jobDone[i] << "] " << endl;

        }
        out2 << endl;
    }

    void jobMarkedPrint() {
        out2 << "Job Marked Array: " << endl;
        out2 << "[Job ID: Mark status]" << endl;
        for (int i = 1; i < jobMarked.capacity(); i++) {
            out2 << "[ " << i << " : " << jobMarked[i] << "] " << endl;
        }
        out2 << endl;
    }
};


int main(int argc, char *argv[]) {
    Ordering data(argc,argv);
    data.scheduling();
    return 0;
}
