/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name: Venkat Nallam
	UIN: 532003829
	Date: 09/29/2024
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int message = MAX_MESSAGE;
	bool create_chan = false;
	string filename = "";
	vector<FIFORequestChannel*> channels;

	//Add other arguments here
	//added other arguments m and c
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				message = atoi(optarg);
				break;
			case 'c':
				create_chan = true;
				break;
		}
	}

	//Task 1:
	//Run the server process as a child of the client process
	pid_t pid = fork();

	//checks if the fork successfully ran
	if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	
	if(pid == 0){
		char server_msg[MAX_MESSAGE];
		sprintf(server_msg, "%d", message);
		//runs the ./server process as a child
		const char* server_args[] = {"./server", "-m", server_msg, NULL};
		execvp(server_args[0], (char* const*)server_args);
		//does not execute if the execvp above is succeful
		perror("exec of server");
		exit(EXIT_FAILURE); 
	}

	//starts the main control channel via fifo
    FIFORequestChannel control_chan("control", FIFORequestChannel::CLIENT_SIDE);

	//addes this to the back of the channels
	channels.push_back(&control_chan);
	//Task 4:
	//Request a new channel
	if(create_chan == true) {
		//creates new channel if needed
		MESSAGE_TYPE channel_message = NEWCHANNEL_MSG;
    	control_chan.cwrite(&channel_message, sizeof(MESSAGE_TYPE));
		char* chan_name = new char[MAX_MESSAGE];
		control_chan.cread(chan_name, MAX_MESSAGE);
		FIFORequestChannel* new_channel = new FIFORequestChannel(chan_name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(new_channel);
		delete[] chan_name;
	}

	//uses the channel that has been created
	FIFORequestChannel chan = *channels.back();
	//Task 2:
	//Request data points
	//checks of if the client is requesting a specific datapoint
	if(p != -1 && t != -1.0 && e != -1){
		char* buf = new char[MAX_MESSAGE];
		datamsg x(p, t, e);
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));
		double reply;
		chan.cread(&reply, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		delete[] buf;
	}else if(p != -1){
		//checks if the client is requesting for the datapoints
		ofstream file_out_1;
		file_out_1.open("received/x1.csv");
		if(!file_out_1){
			perror("unable to open file");
			exit(EXIT_FAILURE);
		}
		//for loop that goes over the first 1000 datapoints and rights them to the output file
		for(double t_curr = 0.0; t_curr < 1000 * 0.004; t_curr += 0.004){
			char buf_1[MAX_MESSAGE];
			datamsg x_1(p, t_curr, 1);
			memcpy(buf_1, &x_1, sizeof(datamsg));
			chan.cwrite(buf_1, sizeof(datamsg));
			double reply_1;
			chan.cread(&reply_1, sizeof(double));

			char buf_2[MAX_MESSAGE];
			datamsg x_2(p, t_curr, 2);
			memcpy(buf_2, &x_2, sizeof(datamsg));
			chan.cwrite(buf_2, sizeof(datamsg));
			double reply_2;
			chan.cread(&reply_2, sizeof(double));

			file_out_1 << t_curr << "," << reply_1 << "," << reply_2 << endl;
		}
		file_out_1.close();
	}

	
	//Task 3:
	//Request files
	filemsg fm(0, 0);
	string fname = filename;
	
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf_3 = new char[len];
	memcpy(buf_3, &fm, sizeof(filemsg));
	strcpy(buf_3 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf_3, len);

	//gets the length of the file requested
	__int64_t file_length;
	chan.cread(&file_length, sizeof(__int64_t));

	//creates the output file
	ofstream file_out_2;
	file_out_2.open("received/" + fname);
	if(!file_out_2){
		perror("unable to open file");
		exit(EXIT_FAILURE);
	}

	//iterates through the file and tracks how much of the file has been transeffered using the offest variable
	//also tracks how much of the file is coming in using size_incoming
	__int64_t offset = 0;
	char* buf_4 = new char[message];
	int size_incoming = 0;
	while(offset < file_length){
		if((__int64_t)message <= (file_length-offset)){
			size_incoming = message;
		}else{
			size_incoming = file_length - offset;
		}
		filemsg temp_msg(offset, size_incoming);
		memcpy(buf_3, &temp_msg, sizeof(filemsg));
		chan.cwrite(buf_3, len);
		chan.cread(buf_4, size_incoming);
		file_out_2.write(buf_4, size_incoming);
		offset += size_incoming;
	}
	//Task 5:
	// Closing all the channels
	//closes the channels by writing to the server and then freeing the pointer
	if(create_chan == true){
		MESSAGE_TYPE m_2 = QUIT_MSG;
		FIFORequestChannel* channel_delete = channels.back();
		channel_delete->cwrite(&m_2, sizeof(MESSAGE_TYPE));
		delete channel_delete;
		channels.pop_back();
	}
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
}