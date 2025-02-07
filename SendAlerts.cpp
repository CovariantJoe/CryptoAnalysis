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
#include <curl/curl.h>

void SMTP(std::string &message);
size_t read(void *ptr, size_t size, size_t nmemb, void *userdata);

struct UploadStatus {std::string message; size_t bytes_read;};

int Alert(std::vector<std::string> alerts, const char* mode)
{
    auto now = std::chrono::system_clock::now();
    auto unix_time = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    const std::string base = "The Database was updated but is still waiting for data";
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
        else if (strcmp(mode, "local") == 0)
        {
            if(i.find( base ) != std::string::npos)
            {
            File << "[" << UNIX(std::to_string(unix_time)) << "] " << i;
            }
            else
            {
            File << "[" << UNIX(std::to_string(unix_time)) << "] " << i << '\n'; 
            }
        }
        else if (strcmp(i.c_str(),".\n") == 0 )
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
// Function to send SMTP alert to and from the account provided by the user
void SMTP(std::string &message)
{
    sqlite3* db;
    sqlite3_stmt* stmt;
    const std::string Query = "SELECT mail,password FROM Mode ORDER BY key DESC LIMIT 1;";
    std::string mail;
    std::string password;
    const std::vector<std::string> validDomains {"gmail.com","outlook.com","hotmail.com","alumnos.udg.mx"}; // Scalable way to add custom SMTP servers later
    std::string domain;
    
    if ( sqlite3_open("Crypto.db",&db) != SQLITE_OK)
    {
        Alert(std::vector<std::string> (1,"Error reading database, probably the database is missing " + std::string(sqlite3_errmsg(db))),"error");
        return;
    }

    if ( sqlite3_prepare_v2(db,Query.c_str(),-1,&stmt,nullptr) == SQLITE_OK)
    {
      if(sqlite3_step(stmt) == SQLITE_ROW) 
      {
        mail = reinterpret_cast<const char*>( sqlite3_column_text(stmt,0) ) ;
        password = reinterpret_cast<const char*>( sqlite3_column_text(stmt,1) );
        if (strcmp(mail.c_str(),"") == 0 || strcmp(password.c_str(),"") == 0)
        {
            Alert(std::vector<std::string> (1,"An e-mail alert was triggered but you didn't introduce both e-mail and password "),"error");
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }
      }
      else
      {
        Alert(std::vector<std::string> (1,"Error reading database, probably the database is missing."),"error");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
      }
    }
    else
    {
        
        Alert(std::vector<std::string> (1,"Error preparing statement, probably the database is missing: " + std::string(sqlite3_errmsg(db))),"error");
        sqlite3_close(db);
        return;
    }
    sqlite3_finalize(stmt);
    CURL *curl;
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl)
    {
        domain = mail.substr(mail.find("@")+1);

        for(auto i : validDomains)
        {
            if (strcmp(i.c_str(),domain.c_str()) == 0)
            {
                // Change domains that don't correspond to the actual name of the SMTP server.
                if (strcmp(validDomains[2].c_str(),domain.c_str()) == 0)
                {
                    domain = "outlook.com";
                    break;
                }
                if (strcmp(validDomains[3].c_str(),domain.c_str()) == 0)
                {
                    domain = "gmail.com";
                    break;
                }
            }
        }
        // Curl expects its own type of list
        std::string url = "smtp://smtp." + domain + ":587"; 
        std::string from =  "<"+mail+">";
        std::string to = "<"+ mail + ">";
        struct curl_slist *recipients = NULL;
        recipients = curl_slist_append(recipients, to.c_str());
        std::cout<< url.c_str() <<std::endl;
        std::cout<< domain <<std::endl;
        curl_easy_setopt(curl, CURLOPT_URL,url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME, mail.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM,from.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    
        UploadStatus upload = { message, 0 };
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        CURLcode res;
        res = curl_easy_perform(curl);
        if(res == CURLE_OK)
        {
            Alert(std::vector<std::string> (1,"SMTP alert sent"),"local");
            return;
        }
        else if (res == CURLE_LOGIN_DENIED)
        {
            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
            Alert(std::vector<std::string> (1,"E-mail alert was triggered but got the following error (gmail / outlook don't allow simple email-password authentication, you need to create an application password): " + std::string(curl_easy_strerror(res))),"error");
            return;
        }
        else
        {
            curl_slist_free_all(recipients);
            curl_easy_cleanup(curl);
            Alert(std::vector<std::string> (1,"E-mail alert was triggered but failed to get a response from e-mail server: " + std::string(curl_easy_strerror(res))),"error");
            return; 
        }
    }
    else
    {
        curl_easy_cleanup(curl);  
        Alert(std::vector<std::string> (1,"E-mail alert was triggered but curl failed to initialize"),"error");
        return;
    }
    curl_easy_cleanup(curl);

    return;
}

// Callback for curl
size_t read(void *ptr, size_t size, size_t nmemb, void *userdata) {
    UploadStatus *upload_ctx = static_cast<UploadStatus*>(userdata);
    size_t max_copy = size * nmemb;
    size_t remaining = upload_ctx->message.size() - upload_ctx->bytes_read;
    size_t copy_this_much = (remaining < max_copy) ? remaining : max_copy;
    
    if(copy_this_much > 0) {
        memcpy(ptr, upload_ctx->message.data() + upload_ctx->bytes_read, copy_this_much);
        upload_ctx->bytes_read += copy_this_much;
    }
    return copy_this_much;
}