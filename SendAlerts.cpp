/** ========================================================================================
    
    Filename:  SendAlerts.cpp
    
    Description:  This program handles the statistical alerts to either write a local one or 
                  send one through SMTP. This program is executed when Analysis.py determines
                  that a threshold has been met or for a variety of other types of log.

    Version:  1.0 Changes:
    Created:  01/30/2025
    Revision:  02/2/2025
    Compiler:  g++
    Author:  Covariant Joe
    
    ========================================================================================
*/
#include "headers.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstring>
#include <sqlite3.h>

void SMTP(std::string &message);

int Alert(std::vector<std::string> alerts, const char* mode)
{
    auto now = std::chrono::system_clock::now();
    auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    std::string base = "The Database was updated but is still waiting for data";
    std::ofstream File("log.txt",std::ios::app);

    if (!File) {
        std::cout << "[" << UNIX(std::to_string(unix_time)) << "] " << "Error opening log.txt. Couldn't write an alert" << std::endl;
        return -1;
    }

    for (auto i: alerts)
    {    
        if( strcmp(mode, "error") == 0 )
        {   
            std::cout << "An error was encountered, see log.txt" << std::endl;
            File << "[" << UNIX(std::to_string(unix_time)) << "] " << i << '\n'; 
           // std::cout << "[" << UNIX(std::to_string(unix_time)) << "] " << i << std::endl;
        }
        else if (strcmp(mode, "kill") == 0)
        {
            File << "------------------------------------------------------------------------------------------------------------------------------------------------------------"<< '\n';
        }
        else if (strcmp(mode, "local") == 0 || i.find( base ) != std::string::npos )
        {
            File << "[" << UNIX(std::to_string(unix_time)) << "] " << i << '\n'; 
        }
        else if (strcmp(i.c_str(),".") == 0 )
        {
            continue;
        }
        else if (strcmp(mode, "mail") == 0)
        {
            SMTP(i);
        }
    }
    File.close();
    return 0;
}
// This is yet to be implemented
void SMTP(std::string &message)
{
    return;
}