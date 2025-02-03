'''
    =====================================================================================================================
    
    Filename:  GUI.py
    
    Description:  This script controls the program's GUI. Its tasks are to show the current currencies
                  being followed, add or delete them, and change every configuration in the program like the thresholds.
                  Clearly, user inputs are checked. Empty inputs leave configs as they are.
                  
    Version:  1.0 Changes:
    Created:  01/28/2025
    Revision:  01/30/2025
    Author:  Covariant Joe
    
    =====================================================================================================================
    
'''

import tkinter as tk
import pandas as pd
import sqlite3

widgets = []

def showData():
    '''
    Function to show which crypto currencies are requested to the API, and to print when was the last price saved.
    '''
    con = sqlite3.connect('Crypto.db')
    nameQuery = "SELECT * FROM Currencies;"
    try:
        names = pd.read_sql(nameQuery,con)
    except pd.errors.DatabaseError:
        data.config(text = "FAILED to gather data. You may be executing this program in a different directory to Crypto.db"); con.close()
        return
    names.set_index(names.ID,inplace=True)
    txt = f"Currencies being followed:\n"
    for i in names.index:
        dateQuery = "SELECT date FROM Prices WHERE CurrencyID = " + str(i) + " ORDER BY date DESC LIMIT 1; "
        try:
            date = " Latest price retrieved at " + pd.read_sql(dateQuery,con).values[0][0] +"\n"
        except (IndexError,pd.errors.DatabaseError):
            date = " A price hasn't been saved \n"
        txt = txt + f"{names.loc[i,'name']}:" + f" API name: {names.loc[i,'GeckoID']}." + date
    
    data.config(text=txt)
    con.close()
    return

def updateFields(entry, var='Add a new currency',index='1',mode='w'):
    '''
    Function with the focus of updating the GUI when a different option is selected, or when inputs are saved.
    Parameters: 
        entry: A list of tk.Entry objects containing the fields to be filled or already filled by the user
        var: String with the current option selected with the action the user wants to perform
        index: placeholder integer not used, but required to have so that this function can be a callback of tk.Stringvar.trace()
        mode: String. Whether the program will just show the fields. Either "w", or "save" to both save to database and reload the fields.
    '''
    val = option_var.get()
    if mode == 'save':
        save(val, entry)
    else:
        error_label.pack_forget()

        
    for widget in widgets:
        widget.pack_forget() 
    #for i in range(len(entry)):
    #    entry.pop()
    
    if val == "Add a new currency":
        label1 = tk.Label(window, text="Insert the 'API ID' from the GeckoAPI (https://www.coingecko.com/en/all-cryptocurrencies)",**styleText)
        label1.pack(pady=5)  # Añadimos un poco de espacio
        entry[0] = tk.Entry(window)
        entry[0].pack(pady=5)
        
        
        # label para el segundo campo de texto (con descripción)
        label2 = tk.Label(window, text="Insert any name by which you'll know this coin:",**styleText)
        label2.pack(pady=5)
        entry[1] = tk.Entry(window)
        entry[1].pack(pady=5)
        widgets.extend([label1, entry[0],label2,entry[1]])
        # label para la opción seleccionada
    
    elif val == "Delete a currency":
        label1 = tk.Label(window, text="Currency name to delete",**styleText)
        label1.pack(pady=5)  # Añadimos un poco de espacio
        entry[0] = tk.Entry(window)
        entry[0].pack(pady=5)

        widgets.extend([label1, entry[0]])
        
    elif val == "Change alert thresholds":
        label0 = tk.Label(window, text = "Empty fields below are kept to the same value, suggested values are defaults.",**styleText)
        label0.pack(pady=5)
        
        label1 = tk.Label(window, text = "The name of the crypto currency (as defined by you) whose config will be updated to the values below.",**styleText)
        label1.pack(pady=5)
        entry[0] = tk.Entry(window)
        entry[0].pack(pady=5)
        
        label2 = tk.Label(window, text = "The minimum number of data since the time defined below required to perform statistics. Suggested 30. Expect about 10 per hour ! ! !.",**styleText)
        label2.pack(pady=5)
        entry[1] = tk.Entry(window)
        entry[1].pack(pady=5)
        
        label3 = tk.Label(window, text = "The number of hours ago that will be considered in the statistical analysis. Suggested 4-6. Results could be skewed if there are very long intervals with no data.",**styleText)
        label3.pack(pady=5)
        entry[2] = tk.Entry(window)
        entry[2].pack(pady=5)
        
        label4 = tk.Label(window, text = "Threshold for instant return (in percentage). The return is (new_price - old_price) / old_price. Suggested 2. Use 0 to deactive this alert",**styleText)
        label4.pack(pady=5)
        entry[3] = tk.Entry(window)
        entry[3].pack(pady=5)
        
        label5 = tk.Label(window, text = "Threshold for return during the time of analysis (in percentage). Same as above but comparing to the first price in the time period. Suggested 3. Use 0 to deactive this alert",**styleText)
        label5.pack(pady=5)
        entry[4] = tk.Entry(window)
        entry[4].pack(pady=5)
        
        label6 = tk.Label(window, text = "Whether or not to alert for the 20 minute average surpassing the 70 minute one to capture developing fast-change (true or false) . Suggested true.",**styleText)
        label6.pack(pady=5)
        entry[5] = tk.Entry(window)
        entry[5].pack(pady=5)
        
        label7 = tk.Label(window, text = "Threshold to alert for anomalies, if a price is higher or lower than this percent of the data (percentage). Suggested 98+. Use 0 to deactive this alert",**styleText)
        label7.pack(pady=5)
        entry[6] = tk.Entry(window)
        entry[6].pack(pady=5)
        
        widgets.extend([label0, label1, entry[1],label2,entry[2],label3,entry[3],label4, entry[4],label5, entry[5],label6, entry[6],label7, entry[0]])
        #entries.extend([entry[1],entry[2],entry[3],entry[4],entry5,entry6,entry7])
        return
    
    elif val == "Change program config":
        label0 = tk.Label(window, text = "Empty fields below are kept to the same value.",**styleText)
        label0.pack(pady=5)
        
        label1 = tk.Label(window, text = "How often should the API be called to update prices? (minutes). Too often will get you rate-limited. Default 1.",**styleText)
        label1.pack(pady=5)

        entry[0] = tk.Entry(window)
        entry[0].pack(pady=5)
        
        label2 = tk.Label(window, text="Alert mode. Send e-mail (write mail) or save locally to log.txt (write local). Default local",**styleText)
        label2.pack(pady=5)
        entry[1] = tk.Entry(window)
        entry[1].pack(pady=5)


        
        widgets.extend([label0,label1,entry[0],label2,entry[1]])
        #entries.extend([entry[1]])
        return
        
    return

def showError(code, error = ''):
    '''
    Function to show expected and unexpected errors to screen. 
    '''
    if code == 0:
        error_label.config(text=f"FAILED reading database, You may be executing this program in a different directory to Crypto.db. Error: {error}",fg="red")
    elif code == 1:
        error_label.config(text="Fields cannot be empty", fg="red")
    elif code == 2:
        error_label.config(text="You are using that crypto currency or name already", fg="red")
    elif code == 3:
        error_label.config(text= f"Error writing to database: {error}", fg = "red")
    elif code == 4:
        error_label.config(text="Name not found", fg="red")
    elif code == 5:
        error_label.config(text="Invalid time. It should be the hours in the past that will be considered for analysis if data was saved then", fg="red")
    elif code == 6:
        error_label.config(text="No input should be negative", fg="red")
    elif code == 7:
        error_label.config(text="Failed to read input as real number", fg="red")
    elif code == 8:
        error_label.config(text="Invalid percentage (0-100)", fg="red")
    elif code == 9:
        error_label.config(text="The moving average alert is neither true nor false", fg="red")
    elif code == 10:
        error_label.config(text="The update frequency (in minutes) should be a real positive number", fg="red")
    elif code == 11:
        error_label.config(text="Invalid mode, choose mail or local", fg="red")
    elif code == 12:
        error_label.config(text= f"Error reading database, did you delete the configs?: {error}", fg = "red")
    elif code == -1:
        error_label.config(text="Changes saved succesfully", fg="green")
    
    error_label.pack(pady=10)
    return

def save(val, entries):
    '''
    Check user input for errors, then write inputs to database. This is for every entry field in the GUI
    '''
    error_label.pack_forget()
    con = sqlite3.connect("Crypto.db")
    findID = "SELECT ID FROM Currencies WHERE name = ? "
    try:
        names = pd.read_sql("SELECT * FROM Currencies;",con)
        pd.read_sql("SELECT * FROM Prices;",con)
        pd.read_sql("SELECT * FROM Mode;",con)
        pd.read_sql("SELECT * FROM Configs;",con)
    except (sqlite3.Error, pd.errors.DatabaseError) as e:
        showError(0, error = e); con.close()
        return

    if val == "Add a new currency":

        cursor = sqlite3.Cursor(con)
        statement = "INSERT INTO Currencies (GeckoID,name) VALUES (?,?);"
        alertDefault = "INSERT INTO Configs (CurrencyID, minimumData,timeWindow, gain, longGain, movingAvg, anomaly) VALUES (?,30,5,2,3,'True',98.5);"
        
        if entry[0].get() == "" or entry[1].get() == "":
           showError(1)
        elif entry[0].get() in names["GeckoID"].values or entry[1].get() in names["name"].values:
           showError(2)
        else:
            try:
                cursor.execute(statement, (entry[0].get(),entry[1].get()))
                ID = int(pd.read_sql(findID ,con, params = (entry[1].get(),)).values[0][0])
                cursor.execute(alertDefault, (ID,))                
                con.commit()
            except sqlite3.Error as err:
                con.rollback()
                showError(3,error = err); con.close()
                return
            showError(-1)
            
    elif val == "Delete a currency":
        
        cursor = sqlite3.Cursor(con)
        statement = "DELETE FROM Currencies WHERE name = ?;"
        if entry[0].get() == "":
           showError(1)
        elif entry[0].get() not in names["name"].values:
           showError(4)
        else:
            try:
                cursor.execute(statement, (entry[0].get(),))
                con.commit()
            except sqlite3.Error as err:
                con.rollback()
                showError(3,error = err); con.close()
                return
            showError(-1)

    elif val == "Change alert thresholds":

        cursor = sqlite3.Cursor(con)
        statement = "INSERT INTO Configs (CurrencyID,minimumData,timeWindow,gain, longGain, movingAvg, anomaly) VALUES (?,?,?,?,?,?,?);"
        getAlert = "SELECT minimumData,timeWindow,gain, longGain, movingAvg, anomaly FROM Configs WHERE CurrencyID = ? ORDER BY alertID DESC LIMIT 1;"
        if entry[0].get() in names["name"].values and entry[0].get() != "":
            ID = int(pd.read_sql(findID ,con, params = (entry[0].get(),)).values[0][0])
            cursor = sqlite3.Cursor(con)
            values = pd.read_sql(getAlert,con,params=(ID,)).values[0]
            
            for i in range(len(entry)-1):
                if entry[i+1].get() == '':
                    continue
                
                # Check for the true or false field. Only check for real numbers if it isn't the boolean field
                if i != 4:
                    try:
                        val = float(entry[i+1].get())
                    except ValueError:
                        showError(7)
                        con.close()
                        return
                elif entry[i+1].get().lower() not in ['true','false'] :
                    showError(9)
                    con.close()
                    return
                else:
                    val = entry[i+1].get().lower()
                    values[i] = val[0].upper() + val[1:]
                    continue
                    
                if val  < 0:
                    showError(6)
                    con.close()
                    return
                elif val > 100 and i in [2,3,5]:
                    showError(8)
                    con.close()
                    return
                else:
                    values[i] = val

            # poner con.close en show err

            try:
                cursor.execute(statement, (ID,values[0],values[1],values[2],values[3],values[4],values[5]))
                con.commit()
            except sqlite3.Error as err:
                con.rollback()
                showError(3,error = err); con.close()
                return
            showError(-1)
            
        else:
            showError(4)
            
    elif val == "Change program config":
        cursor = sqlite3.Cursor(con)
        statement = "INSERT INTO Mode (updateFreq,mode) VALUES (?,?);"
        getMode = "SELECT updateFreq, mode FROM Mode ORDER BY key DESC LIMIT 1;"
        
        try:
            Vals = pd.read_sql(getMode,con).values[0]
        except pd.errors.DatabaseError as e:
            showError(12,e)    
            
            
        if entry[0].get() != '':
            try:
                Vals[0] = (float(entry[0].get()))
                if Vals[0] <= 9e-2:
                    showError(10)
                    con.close
                    return
            except ValueError:
                showError(10)
                con.close()
                return
        
        if entry[1].get() != '':
            try:
                Vals[1] = entry[1].get().lower()
            except ValueError:
                showError(11)
                con.close()
                return
            if Vals[1] != "mail" and Vals[1] != "local":
                showError(11)
                con.close()
                return
        try:
            cursor.execute(statement,(Vals[0],Vals[1]))
            con.commit()
        except sqlite3.Error as e:
            con.rollback()
            showError(3,e); con.close()
            return
        showError(-1)

    con.close()
    return
                

# Create the main window
window = tk.Tk()
window.title("CryptoAnalysis V1.0")
window.geometry("1200x820")  # Window size

styleTitle = {"font": ("Helvetica", 14),"bd": 1,"highlightcolor": "#4CAF50","highlightbackground": "#4CAF50"}
styleButton = {"font": ("Helvetica", 12),"bd": 2,"relief": "solid","background": "#f0f0f0","highlightcolor": "#4CAF50","highlightbackground": "#4CAF50"}
styleError = style = {"font": ("Helvetica", 13),"bd": 2,"highlightcolor": "#4CAF50","highlightbackground": "#4CAF50"}
styleText = style = {"font": ("Helvetica", 12),"bd": 2,"highlightcolor": "#4CAF50","highlightbackground": "#4CAF50"}

labelWelcome = tk.Label(window, text="Welcome! Remember to save changes. Suggested values are defaults. log.txt contains status",**styleTitle)
labelWelcome.pack(pady=5)
error_label = tk.Label(window, text="", fg="red",**styleError)
entries = []

options = ["Add a new currency", "Delete a currency", "Change alert thresholds", "Change program config"]
option_var = tk.StringVar(window)
option_var.set(options[0])

button = tk.Button(window, text="Show Data", command=showData,**styleButton)
button.pack(pady=10)

data = tk.Label(window,**styleText)
data.pack(pady=5)

button2 = tk.Button(window, text="Save changes", command=lambda: updateFields(entry, mode='save'),**styleButton)
button2.pack(pady=10)

option_menu = tk.OptionMenu(window, option_var, *options)
option_menu.pack(pady=5)
option_var.trace("w", lambda a,b,mode: updateFields(entry,mode=mode))

entry = [tk.Entry(window),tk.Entry(window),tk.Entry(window),tk.Entry(window),tk.Entry(window),tk.Entry(window),tk.Entry(window),tk.Entry(window)]
updateFields(entry)


window.mainloop()
