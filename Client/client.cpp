#include <stdarg.h>
#include <resolv.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <sstream>
#include <fstream>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <fstream>

using namespace std;

#define MAXBUFFER 1024000
string requests_file = "requests.txt"; 
string default_portNumber = "8080";
string default_server_ip = "localhost"; 


vector<string> split(string str) {
	string buffer;
	stringstream ss(str);
	vector<string> tokens;
	while (ss >> buffer)
		tokens.push_back(buffer);
	return tokens;
}

vector<string> readLines(){
	string line;
	vector<string> lines;
	ifstream myfile(requests_file);

	if (myfile.is_open()) {
		while (myfile.good()) {
			getline(myfile, line);
			if (strcmp(line.c_str(), "\\r\\n") == 0) {   // last line in the file
				break;
			}
			int index = line.find_first_of("\\");
			line = line.substr(0, index);
			lines.push_back(line);
		}
		myfile.close();
	} else {
		cout << "Unable to open file\n";
	}
	for (auto line : lines)
	{
		cout << line << endl;
	}
	cout << "****************" << endl;
	return lines;
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

	bool type = isImage(fileName);
	string line;

	ifstream myfile(fileName);
	string fileContent;
	if (!type) {
		//read regular files
		if (myfile.is_open()) {
			while (myfile.good()) {
				getline(myfile, line);
				fileContent += line;
				fileContent += "\n";
			}
			myfile.close();
		}
		else {
			cout << "Unable to open file";
			return "";
		}
	} else {
		if (myfile.is_open()) { //image file .. read it binary
			ifstream fin(fileName, ios::in | ios::binary);
			ostringstream oss;
			oss << fin.rdbuf();
			string data(oss.str());
			fileContent += oss.str();
		} else {
			cout << "Unable to open image";
			return "";
		}

	}
	// add header to the file content
	fileContent = "{" + fileContent + "}";
	return fileContent;
}

void writeTheFile(string fileName, string content){
	int file_start = content.find("{", 0);
	int file_end = content.length() - 1;
	int file_length = file_end - file_start - 1;
	string file = content.substr(file_start + 1, file_length);
	ofstream outfile((fileName).c_str(), ios::binary);
	outfile.write(file.c_str(), file.length());
	outfile.close();
}

void GET_REQUEST(string port_num, string host_name, string fileName, string request){

	cout << port_num + " " + host_name + " " + fileName + " " + request << endl;

	int sockfd;
	struct sockaddr_in serv_addr;
	char buffer[MAXBUFFER];
	struct hostent *server;
	string response = "";

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket failed");
	}else
		printf("Socket successfully created..\n");

	// host name
	server = gethostbyname(host_name.c_str());
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(port_num.c_str()));
	bcopy(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
	
	
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		perror("Connect");
	
	memcpy(buffer, request.c_str(), request.size() + 1); // 1 because of terminal of string
	send(sockfd, buffer, strlen(buffer), 0);  //request is sent

	int bytes_read = 0;
	do {
		bzero(buffer, sizeof(buffer));
		bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
		if (bytes_read > 0) {
			if (bytes_read < MAXBUFFER) {
				for (int i = 0; i < bytes_read; i++) {
					response += buffer[i];
				}
			}
		}
	} while (bytes_read > 0);
	if(response.find("HTTP/1.0 404 Not Found") != string::npos){
		cout << "HTTP/1.0 404 Not Found" << endl;
		return;
	}
	cout << "Response : " << response << endl;
	writeTheFile(fileName, response);
	// cout << "Request : " << request << endl;
	
	return;
}

void POST_REQUEST(string port_num, string host_name, string fileName, string request){

	int sockfd;
	struct sockaddr_in serv_addr;
	char buffer[MAXBUFFER];
	struct hostent *server;
	string response = "";

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("socket failed");
	}else
		printf("Socket successfully created..\n");

	// host name
	server = gethostbyname(host_name.c_str());
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host\n");
		exit(0);
	}
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(port_num.c_str()));
	bcopy(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
	
	
	if (connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		perror("Connect");
	
	string fileContent = readTheFile(fileName);

	request += fileContent;

	cout << request << endl;
	
	memcpy(buffer, request.c_str(), request.size() + 1); // 1 because of terminal of string
	send(sockfd, buffer, strlen(buffer), 0);  //request is sent

	int n = read(sockfd, buffer, MAXBUFFER);
	if (n < 0)
		perror("ERROR reading from socket");

	printf("Here is the message that reached from the server: %s\n", buffer);
	close(sockfd);
	// cout << "Request : " << request << endl;
	// cout << "Response : " << response << endl;
	return;
}

int main(int argc, char *argv[]) {

	string host_name;
	string portNum;

	if (argc < 3) {
		fprintf(stderr, "usage %s hostname port\n", argv[0]);
		exit(0);
	}

	// taking values from the arguements and changing the default values of the server
	default_server_ip = argv[1];
	default_portNumber = argv[2];

	// read the requests file
	vector<string> lines = readLines();

	for (auto line : lines)
	{
		cout << "a request that read from the file" + line << endl;
		vector<string> tokens = split(line);
		for(string t : tokens){
			cout << t << endl;
		}

		if (tokens.size() == 4) {
			host_name = tokens[2];
			portNum = tokens[3];
		} else {
			host_name = default_server_ip;
			portNum = default_portNumber;
		}

		string fileName = tokens[1].substr(1, tokens[1].length());  // file name in the request (post or get)

		cout << "fileName in the main " + fileName << endl;

		if(tokens[0] == "GET")
		{
			GET_REQUEST(portNum, host_name, fileName, line);
		}
		else if (tokens[0] == "POST")
		{
			POST_REQUEST(portNum, host_name, fileName, line);
		}
		else
		{
			cout << "UNSUPPORTED Request \n\n" << endl;
		}
	}
	return 0;
}