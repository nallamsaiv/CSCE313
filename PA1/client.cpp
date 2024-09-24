/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name:
	UIN:
	Date:
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

	//Add other arguments here
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

	if(pid == -1){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	
	if(pid == 0){
		char server_msg[MAX_MESSAGE];
		sprintf(server_msg, "%d", message);
		const char* server_args[] = {"./server", "-m", server_msg, NULL};
		execvp(server_args[0], (char* const*)server_args);
		perror("exec of server");
		exit(EXIT_FAILURE); 
	}

    FIFORequestChannel control_chan("control", FIFORequestChannel::CLIENT_SIDE);
	vector<FIFORequestChannel*> channels;
	channels.push_back(&control_chan);
	//Task 4:
	//Request a new channel
	if(create_chan == true){
		MESSAGE_TYPE temp_msg = NEWCHANNEL_MSG;
		control_chan.cwrite(&temp_msg, sizeof(MESSAGE_TYPE));
		char temp[MAX_MESSAGE];
		control_chan.cread(temp, MAX_MESSAGE);
		channels.push_back(new FIFORequestChannel(temp, FIFORequestChannel::CLIENT_SIDE));
	}
	FIFORequestChannel chan = *channels.back();
	//Task 2:
	//Request data points
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
		ofstream file_out_1;
		file_out_1.open("received/x1.csv");
		if(!file_out_1){
			perror("unable to open file");
			exit(EXIT_FAILURE);
		}
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

	__int64_t file_length;
	chan.cread(&file_length, sizeof(__int64_t));

	ofstream file_out_2;
	file_out_2.open("received/" + fname);
	if(!file_out_2){
		perror("unable to open file");
		exit(EXIT_FAILURE);
	}
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
	if(create_chan == true){
		MESSAGE_TYPE m_2 = QUIT_MSG;
		FIFORequestChannel last_channel = *channels.back();
		last_channel.cwrite(&m_2, sizeof(MESSAGE_TYPE));
		delete channels.back();
		channels.pop_back();
	}
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));

	waitpid(pid, nullptr, 0);
}
