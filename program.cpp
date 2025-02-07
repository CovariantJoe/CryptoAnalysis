/** ========================================================================================
    
    Filename:  program.cpp
    
    Description:  This is the main program that controls the flow of execution. While the GUI
                  is open, the API will be called (program.cpp) after the time defined by the 
                  user has passed. If the database gets updated, calls the python analysis
                  program. Finally, SendAlerts.cpp handles the action to be taken. 

    Version:  1.0 Changes:
    Created:  01/30/2025
    Revision:  02/2/2025
    Compiler:  g++
    Author:  Covariant Joe
    
    ========================================================================================
*/

#include "headers.h"
#include <sqlite3.h>
#include <iostream>
#include <thread>
#include <future>
#include <atomic>
#include <cstdio>
#include <memory>
#include <vector>
#include <array>
#include <cstdlib>
#include <chrono>
#include <cstring>

// Atomic flag to indicate if the GUI is still running
std::atomic<bool> isGuiRunning(true);

// Function to run a Python script and capture its output in real-time
std::vector<std::string> Analyze(const std::string &command) {
    std::array<char, 256> buffer;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);

    if (!pipe) {
        return std::vector<std::string> (1,"Error: Failed to start python process");
    }
    std::vector<std::string> output;
    // Read and print output line by line
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        output.push_back( buffer.data() );
        // Check if the GUI is no longer running and stop the task
        if (!isGuiRunning.load()) {
            return output;
        }
    }
    return output;
}

// Function to run the Python GUI
void runPythonGUI() {
    system("python3 GUI.py");
    isGuiRunning.store(false); 
}

int main() {
    int updates;
    int count = 0;
    int count2 = 0;
    std::thread guiThread(runPythonGUI);
    //std::thread outputThread;

    const std::string Check = "SELECT * FROM Mode ORDER BY key DESC LIMIT 1;";
    std::string mode = "local";
    double timing = 60;
    std::vector<std::string> alerts;

    // Main loop to keep running while the GUI is open
    auto before = std::chrono::steady_clock::now();
    while (isGuiRunning.load()) {
        sqlite3* db;
        if (sqlite3_open("Crypto.db",&db) != SQLITE_OK)
        {
            Alert(std::vector<std::string> (1,sqlite3_errmsg(db)), "error");
        }
        sqlite3_stmt* chck;
        if (sqlite3_prepare_v2(db,Check.c_str(),-1,&chck,nullptr) == SQLITE_OK)
        {
            if (sqlite3_step(chck) == SQLITE_ROW)
            {
                timing = 60*sqlite3_column_double(chck,1);
                mode = reinterpret_cast<const char*>(sqlite3_column_text(chck,2));
            }
        }
        else
        {
            Alert(std::vector<std::string> (1,sqlite3_errmsg(db)), "error");
        }
        sqlite3_finalize(chck);
        sqlite3_close(db);
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed = now - before;
        if (elapsed.count() >= timing || count == 0)
        {   
            count = 1;
            updates = database();
            if(updates > 0)
            {
                std::future<std::vector<std::string>> output = std::async(std::launch::async, Analyze, "python3 Analysis.py");
                std::vector<std::string> result = output.get();
                if ( strcmp(result[0].c_str(),".\n") == 0 && count2 == 0)
                {
                    Alert(std::vector<std::string>(1,"There is now enough data for at least 1 currency to perform analyses from now on"),"local");
                    count2 = 1;
                }
                Alert(result,mode.c_str());
            }
            before = now;
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));  // Sleep to prevent busy-wait
    }

    Alert(std::vector<std::string> (1,""),"kill");
    guiThread.join();  // Wait for the GUI thread to finish


    return 0;
}
