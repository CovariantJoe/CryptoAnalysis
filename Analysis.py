'''
    ===========================================================================================================
    
    Filename:  Analysis.py
    
    Description:  This script performs the statistical analyses for each currency defined by the user with the 
                  data in Crypto.db. The script first checks whether or not there is enough data in the time
                  window to perform analyses, this is done comparing with the parameters "minimumData" and 
                  "timeWindow" defined by the user. If any threshold defined by the user is surpassed, the alert 
                  program is called, which can produce either a local or SMTP alert.

                  The following metrics are used to evaluate the data: instant Return (sudden change),
                  "timeWindow" Return (change during "timeWindow"), 10 vs 50 minute moving average, 
                  volatility during timeWindow, x% anomaly (higher/lower price than x% of data), coefficient of
                  variation (volatility/mean, useful for comparing currencies)
                  
    Version:  1.0 Changes:
    Created:  01/27/2025
    Revision:  01/28/2025
    Author:  Covariant Joe
    
    ===========================================================================================================
    
'''
import pandas as pd
import sqlite3
import time

def Returns(data,N = 2):
    '''
    Simple function to calculate the instant or time Window return as explained above.
    Returns: a float64: (New price - N prices ago) / N prices ago
    '''
    prices = data.price.values
    try: 
        old = prices[-N]
    except IndexError:
        return 0
    return ( prices[-1] - old ) / old

def movingAverage(data, nMinute):
    '''
    Simple function to calculate the nMinute moving average as explained above.
    Returns: a float64 representing the average price over nMinutes 
    '''    
    winBegin = tm - nMinute*60
    within = data[data.time > winBegin]
    if within.empty:
        return 0
    else:
        return within.price.mean()

def anomaly(data,threshold):
    '''
    Simple function to check whether the last value is an anomaly of x% as explained above
    Returns: 1 if value is higher than x%, 2 if value is lower than x%, 0 if neither
    '''
    if threshold == 0:
        return 0
    high = data.describe(percentiles=[threshold/100]).loc[str(threshold) + "%"].price
    low = data.describe(percentiles=[1 - threshold/100]).loc[str(100 - threshold) + "%"].price
    val = data.price.values[-1]
    if val > high:
        return 1
    elif val < low:
        return 2
    else:
        return 0

def variation(data):
    '''
    Simple function to calculate the coefficient of variation described above
    Returns: two float64: coefficient of variation, volatility
    '''
    volatility = data.price.std()
    return data.price.mean()/volatility,volatility

if __name__ == "__main__":
    
    con = sqlite3.connect('Crypto.db')
    nameQuery = "SELECT * FROM Currencies;"
    configQuery = "SELECT CurrencyID,minimumData,timeWindow FROM Configs WHERE CurrencyID = "
    priceQuery = "SELECT * FROM Prices;"
    thresholdQuery = "SELECT gain,longGain,movingAvg,anomaly FROM Configs WHERE CurrencyID = "

    names = pd.read_sql(nameQuery,con)
    prices = pd.read_sql(priceQuery,con)

    names.set_index(names.ID,inplace=True)
    valid = []
    thresholds = {}
    tm = time.time() 
##  1738175548.420135: 100 entries
##  1738184426.260612; last values
    tm = 1738184426.260612
# Check there is enough data for each currency
# Then loads threshold configurations and sets default values if they don't exist
    for i in names.index:

        configs = pd.read_sql(configQuery + str(i) + " ORDER BY alertID DESC LIMIT 1;",con)
        if configs.empty:
            configs = pd.DataFrame([{"CurrencyID":i,"timeWindow":5,"minimumData":30}])
            data = prices[prices["CurrencyID"] == i]
            winBegin = tm - 3600*configs["timeWindow"].values[0]
            if data.time[data.time > winBegin].count() < configs.minimumData.values[0]:
                continue
            else:
                alerts = pd.read_sql(thresholdQuery + str(i)+" ORDER BY alertID DESC LIMIT 1;",con)
                if alerts.empty:
                    thresholds[str(i)] = [5,5,True,95]
                else:
                    thresholds[str(i)] = [alerts.gain.values[0],alerts.longGain.values[0],alerts.movingAvg.values[0],alerts.anomaly.values[0]]
                valid.append(data[data.time > winBegin])
    con.close()
    if valid == []:
        print("The Database was updated but is still waiting for data")
# Perform calculations and check thresholds iterating for each currency with enough data
    for data in valid:
        test1 = Returns(data) # To check if returns had a significant immediate change 
        test2 = Returns(data,data.price.count() ) # To check if returns changed significantly in the whole analysis period
        test3 = movingAverage(data,20) 
        test4 = movingAverage(data,70) # Comparing this quantity in 20 min and 70 min to capture developing change
        test5 = anomaly(data,thresholds[str(i)][-1]) # Check whether the last price is an anomaly
        test6,volatility = variation(data) # Compute the coefficient of variation
    
        print(".")
        if abs(test1) > thresholds[str(i)][0] and thresholds[str(i)][0] != 0:
            if test1 > 0:
                print("The instant return value of {names.loc[i,'name']} has surpassed the {thresholds[str(i)][1]}% threshold, this may be a significant instant increase.")
            else:
                print("The instant return value of {names.loc[i,'name']} has surpassed the {thresholds[str(i)][1]}% threshold, this may be a significant instant decrease.")
        if abs(test2) > thresholds[str(i)][1] and thresholds[str(i)][1] != 0:
            if test2 > 0:
                print("The return value of {names.loc[i,'name']} has surpassed the {thresholds[str(i)][1]}% threshold with {data.price.count()} data since {prices[prices.CurrencyID == i].date.values[0]}. This may be a significant increase")
            else:
                print("The return value of {names.loc[i,'name']} has surpassed the {thresholds[str(i)][1]}% threshold with {data.price.count()} data since {prices[prices.CurrencyID == i].date.values[0]}. This may be a significant decrease")
        if test3 > test4 and thresholds[str(i)][3] == True:
            print("The average value of {names.loc[i,'name']} in the last 20 minutes ({test3}) has surpassed the average in 90 minutes ({test4}), so change could be developing fast ")
        if test5 != 0:
            if test5 == 1:
                print(f"The latest price retrieved for {names.loc[i,'name']} ({prices[prices.CurrencyID == i].price.values[-1]}) is higher than {thresholds[str(i)][-1]}% from a total of {data.price.count()} data since {prices[prices.CurrencyID == i].date.values[0]}")
            else:
                print(f"The latest price retrieved for {names.loc[i,'name']} ({prices[prices.CurrencyID == i].price.values[-1]}) is lower than {thresholds[str(i)][-1]}% from a total of {data.price.count()} data since {prices[prices.CurrencyID == i].date.values[0]}")
        if test6 > 1.5:
            print(f"The coefficient of variation for {names.loc[i,'name']} is at {test6}. Consider the volatility at this moment is {volatility}, calculated with {data.price.count()} data since {prices[prices.CurrencyID == i].date.values[0]}")
            
    
