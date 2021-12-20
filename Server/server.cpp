#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <sys/time.h>

using namespace std;

inline void error(const char *msg) {
	perror(msg);  // msg : description of the error
	exit(1);
}

inline vector<string> split(string str) {
	string buffer;
	stringstream ss(str);
	vector<string> tokens; 
	while (ss >> buffer)
		tokens.push_back(buffer);
	return tokens;
}

inline void writeTheFile(string fileName, string content){
	int start = content.find("{", 0);
	int end = content.length() - 1;
	int length = end - start - 1;
	string file = content.substr(start + 1, length);
	ofstream outfile((fileName).c_str(), ios::binary);
	outfile.write(file.c_str(), file.length());
	outfile.close();
}

inline bool isImage(string fileName){
    if (fileName.find(".jpg") != string::npos) {
		return true;
	}
	if (fileName.find(".png") != string::npos) {
		return true;
	}
	if (fileName.find(".gif") != string::npos) {
		return true;
	}
    return false;
}

string readTheFile(string fileName) {

	cout << "file Name = " + fileName << endl;

	bool type = isImage(fileName);
	string line;
	ifstream myfile(fileName);
	string fileContent;
	if (!type) {
		//read regular files
		if (myfile.is_open()) {
			while (myfile.good()) {
				getline(myfile, line);
				fileContent += (line);
				fileContent += "\n";
			}
			myfile.close();
		}
		else {
			cout << "Unable to open file";
			return "HTTP/1.0 404 Not Found\r\n";
		}
	} else {
		if (myfile.is_open()) { //image file .. read it binary
			ifstream fin(fileName, ios::in | ios::binary);
			ostringstream oss;
			oss << fin.rdbuf();
			//string s(oss.str());
			fileContent += oss.str();
		} else {
			cout << "Unable to open image";
			return "HTTP/1.0 404 Not Found\r\n";
		}

	}
	// add header to the file content
	string s = "HTTP/1.0 200 OK \r\n{" ;
	s += fileContent;
	s += "}";
	cout << "response = \n" + s << endl;
	return s;
}

int main(int argc, char *argv[]) {
	int server_fd, new_socket;
	int portno;
	socklen_t len;
	char buffer[1024000]; 
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		exit(1);
	}

	// Creating socket file descriptor
	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket failed");
		exit(EXIT_FAILURE);
	}else
		printf("Socket successfully created..\n");
    
	bzero(&serv_addr, sizeof(serv_addr));

	// assign IP, PORT
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);


    // Binding newly created socket to given IP and verification
	if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");
    
	while (true) {
		listen(server_fd, 5);
		len = sizeof(cli_addr);
		new_socket = accept(server_fd, (struct sockaddr *) &cli_addr, &len);

		if (new_socket < 0)
			error("ERROR on accept");

		pid_t PID = fork();
		if (PID == 0) { // we are at child process
			bzero(buffer, 1024000);
			n = read(new_socket, buffer, 1023999);

			if (n < 0)
				error("ERROR reading from socket");

			printf("Here is the message that reached from the client: %s\n", buffer);

			string buf = buffer;

			vector<string> dataOfTheMessage = split(buf);
			if (dataOfTheMessage[0] == "GET") {
				cout << " in GET request" << endl;
				string fileName = dataOfTheMessage[1].substr(1, dataOfTheMessage[1].size() - 1);
				cout << fileName << endl;
				string response = readTheFile(fileName);  // headers + filecontent
				n = write(new_socket, response.c_str(), response.length() + 1);
				cout << "************get request ended**********" << endl;
			} 
			else if(dataOfTheMessage[0] == "POST")
			{
				cout << " in POST request in the server" << endl;
				//cout << buf << endl;
				string fileName = dataOfTheMessage[1].substr(1, dataOfTheMessage[1].size() - 1);
				writeTheFile(fileName, buf);
				cout << "************post request ended**********" << endl;
			}
			else 
			{
				n = write(new_socket, "Error not well formed", 50);
			}
			if (n < 0)
				error("ERROR writing to socket");
			exit(0);
		} else {
			close(new_socket);
		}

	}
	return 0;
}