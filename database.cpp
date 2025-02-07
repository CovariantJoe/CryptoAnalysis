/** ========================================================================================
    
    Filename:  database.cpp
    
    Description:  This program handles the SQLite database Crypto.db. It calls the CoinGecko API 
                  with requests for each currency in the table Currencies and adds new prices 
                  to the database, if they changed. If changes occurred, the Python 
                  statistical analysis program is called by program.cpp, which could trigger a 
                  SMTP alert. 
                  
    Version:  1.0 Changes:
    Created:  01/22/2025
    Revision:  01/26/2025
    Compiler:  g++
    Author:  Covariant Joe
    
    ========================================================================================
*/

#include "headers.h"
#include <stdio.h>
#include <sqlite3.h>
#include <iostream>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <ctime>

int update(sqlite3 *db,std::vector<std::string> &names, std::vector<double> &time, std::vector<std::string> &date, std::vector<double> &price);
int API(sqlite3 *db);
size_t curlCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
std::string UNIX(std::string unix_time);

int database()
{
    sqlite3* db;
    int updates;
    if (sqlite3_open("Crypto.db",&db) != SQLITE_OK)
    {
        Alert(std::vector<std::string> (1,"Error opening database, probably the database is missing: " + std::string(sqlite3_errmsg(db))),"error");
        return -1;
    }
    updates = API(db);
    sqlite3_close(db);
    return updates;
}

// Calls Gecko API then update() whose output is returned
// Returns: 0 without changes, -1 with errors, and > 0 if that number of database changes ocurred.
int API(sqlite3 *db)
{
    const std::string url = "https://api.coingecko.com/api/v3";
    const std::string Query = "SELECT GeckoID FROM Currencies;";
    const std::string currency = "usd";
    std::vector<std::string> Endpoints = {"/simple/price?ids=","/ping"};
    std::string Request;
    std::string Response;
    std::vector<std::string> Data;
    std::vector<std::string> names, names_returned;
    std::vector<std::string> date;
    std::vector<double> time;
    std::vector<double> price;
    CURL* curl;
    CURLcode flag;;
    std::string subresponse;
    int j = 0,k = 0, i,flag2,begin,end;

    // find what currencies to request
    sqlite3_stmt* stmt;
    flag2 = sqlite3_prepare_v2(db,Query.c_str(),-1,&stmt,nullptr);
    if (flag2 == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            names.push_back( reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)) );
            Endpoints[0] = Endpoints[0] + names[j] + "%2C";
            j++;
        }
        if (j == 0)
        {
            Alert(std::vector<std::string> (1,"You haven't defined any cryptos to query"),"local");
            sqlite3_finalize(stmt);
            return -1;
        }      
        Endpoints[0].erase(Endpoints[0].end() - 3, Endpoints[0].end()); // remove the extra comma
        Endpoints[0] = Endpoints[0] + "&vs_currencies=" + currency + "&include_last_updated_at=true";
    }
    else
    {
        Alert(std::vector<std::string> (1,"Error reading database: " + std::string(sqlite3_errmsg(db))),"error");
        sqlite3_finalize(stmt);
        return -1;
    }
    // Call each endpoint
    for (int i = 0; i < Endpoints.size(); i++)
    {
        curl = curl_easy_init();
        if (curl) 
        {
            Request = url + Endpoints[i];
            //std::cout<< Request << std::endl; 
            curl_easy_setopt(curl, CURLOPT_URL, Request.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &Response);

            flag = curl_easy_perform(curl);

            if (flag != CURLE_OK) 
            {
                Alert(std::vector<std::string> (1,"Failed to use curl. Your internet is probably down"),"error");
                sqlite3_finalize(stmt);
                return -1;
            } 
            else
            {
                //std::cout << "raw response: " << Response << std::endl;
                Data.push_back(Response);
            }
            curl_easy_cleanup(curl);
        }
        else 
        {
            Alert(std::vector<std::string> (1,"Failed to initialize curl"),"error");
            sqlite3_finalize(stmt);
            return -1;
        }
    }
    const std::string test = "{\"gecko_says\":\"(V3) To the Moon!\"}";

    if(Data[1].find("exceeded the Rate Limit") != std::string::npos)
    {
        Alert(std::vector<std::string> (1,"Rate limited, server response: " + Data[0]),"error");
        sqlite3_finalize(stmt);
        return -1;
    }
    else if (Data[1].find( test ) == std::string::npos)
    {
        Alert(std::vector<std::string> (1,"API may be down, response: " + Data[1]),"error");
        sqlite3_finalize(stmt);
        return -1;
    }
    else
    {
        // assign date and price
        for(i = 0; i < j; i++)
        {
            if(Data[0].find("\"" + names[i]+"\"") == std::string::npos)
            {
                k++;
                continue;
            }
            names_returned.push_back(names[i]);
            subresponse = Data[0].substr(Data[0].find(names[i]));
            begin = subresponse.find(currency);

            // Maybe put this inside previous if

            if(begin == std::string::npos)
            {
                Alert(std::vector<std::string> (1,"Error retrieving the crypto " + names[i] + " from API"),"error");
                sqlite3_finalize(stmt);
                return -1;
            }
            else
            {
                begin = subresponse.find_first_of("0123456789.",begin);
                end = subresponse.find_first_not_of("0123456789.",begin);
                price.push_back(std::stod(subresponse.substr(begin,end-begin)));
                begin = subresponse.find_first_of("0123456789.",end);
                date.push_back( UNIX(subresponse.substr(begin, subresponse.find_first_not_of("0123456789.",begin)-begin)) );
                time.push_back( std::stod(subresponse.substr(begin, subresponse.find_first_not_of("0123456789.",begin)-begin)) );
                ;
            }

        }
    }
    sqlite3_finalize(stmt);
    return update(db,names_returned,time,date,price);
}

// New entry in the database with new date/price for each currency from API response.
// Only updates the currencies that have changed. 
// returns 0 without changes, -1 with errors, and updated > 0 if database changes ocurred.
int update(sqlite3 *db,std::vector<std::string> &names, std::vector<double> &time, std::vector<std::string> &date, std::vector<double> &price)
{
    sqlite3_stmt* stmt;
    sqlite3_stmt* chck;
    sqlite3_stmt* nchck;
    const char* Statement = "INSERT INTO Prices (\"CurrencyID\",\"time\",\"date\",\"price\") VALUES (?,?,?,?);";
    const char* nameCheck = "SELECT ID FROM Currencies WHERE GeckoID = ?;";
    const char* Check = "SELECT * FROM Prices WHERE CurrencyID = ? ORDER BY PriceID DESC LIMIT 1;";
    std::vector<int> IDs;
    int flag, flag2;
    int updated = 0;

    // Extract internal crypto ID corresponding to each name returned by API
    for(int i = 0; i < names.size(); i++)
    {
        flag = sqlite3_prepare_v2(db,nameCheck,-1,&nchck,nullptr);
        if (flag == SQLITE_OK)
        {
            sqlite3_bind_text(nchck,1,names[i].c_str(),names[i].size(),nullptr);
            if (sqlite3_step(nchck) != SQLITE_ROW)
            {
                Alert(std::vector<std::string> (1,"Error reading crypto IDs internally: " + std::string(sqlite3_errmsg(db))),"error");
                sqlite3_finalize(nchck);
                return -1;
            }
            IDs.push_back(sqlite3_column_int(nchck,0));
            if (sqlite3_step(nchck) == SQLITE_ROW)
            {
                Alert(std::vector<std::string> (1,"Duplicated ID entries for the same crypto currency: " + names[i] + ", the database in your computer is not reliable. Aborting."),"error");
                sqlite3_finalize(nchck);
                return -1;
            }
            sqlite3_finalize(nchck);
            }
        else
        {
            Alert(std::vector<std::string> (1,"Error reading crypto IDs internally: " + std::string(sqlite3_errmsg(db)) ),"error");
            sqlite3_finalize(nchck);
            return -1;
        }
    }

    
    for (int i = 0; i < IDs.size() ; i++) // for each currency of interest
    {
        flag = sqlite3_prepare_v2(db,Check,-1,&chck,nullptr);
        if(flag != SQLITE_OK)
        {
            Alert(std::vector<std::string> (1,"Error preparing statements: " + std::string(sqlite3_errmsg(db)) ),"error");
            sqlite3_finalize(chck);
            return -1;
        }
        else if (sqlite3_prepare_v2(db,Statement,-1,&stmt,nullptr) != SQLITE_OK)
        {
            Alert(std::vector<std::string> (1,"Error preparing statements: " + std::string(sqlite3_errmsg(db)) ),"error");
            sqlite3_finalize(stmt);
            return -1;
        }

        sqlite3_bind_int(chck,1,IDs[i]);
        flag2 = sqlite3_step(chck);

        if(flag2 == SQLITE_ROW || flag2 == 101)
        {
            // Check if price changed since last API call
            if(std::abs(price[i] - sqlite3_column_double(chck, 4)) < 0.0000002)
            {
                sqlite3_step(chck);
                sqlite3_finalize(stmt);
                sqlite3_finalize(chck);
                continue;
            }
            else // Append to db
            {
                sqlite3_bind_int(stmt,1,IDs[i]);
                sqlite3_bind_double(stmt,2,time[i]);
                sqlite3_bind_text(stmt,3,date[i].c_str(),19,nullptr); // call UNIX to date
                sqlite3_bind_double(stmt,4,price[i]);
                if(sqlite3_step(stmt) == SQLITE_DONE)
                {
                    updated++;
                }
                else
                {
                    Alert(std::vector<std::string> (1,"Error writing changes to database: " + std::string(sqlite3_errmsg(db)) ),"error");
                    sqlite3_finalize(stmt);
                    sqlite3_finalize(chck);
                    return -1;
                }
                
            }
        }
        else
        {
            Alert(std::vector<std::string> (1,"Error performing query: " + std::string(sqlite3_errmsg(db)) ),"error");
            sqlite3_finalize(stmt);
            sqlite3_finalize(chck);
            return -1;
        }
        sqlite3_finalize(stmt);
        sqlite3_finalize(chck);
    }
    return updated;
}

// Function required to save output into variables when calling the API
size_t curlCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Function to convert UNIX output given by the API to format: YYYY-MM-DD HH:MM:SS before saving to database
std::string UNIX(std::string unix_time)
{
    const std::time_t unix_time_t = std::stoll(unix_time);
    std::tm* local_time = std::localtime(&unix_time_t);

    char temp[100];
    if (std::strftime(temp, sizeof(temp), "%Y-%m-%d %H:%M:%S", local_time)) {
        std::string result(temp);
        return result;
    } else {
        std::cerr << "Failed to convert time from UNIX to local time." << std::endl;
        return "0000-00-00 00:00:00";
    }
    
}
