# -*- coding: cp950 -*-
import os
import subprocess

def main():
    '''process = subprocess.Popen("./3-0.5-0/WBAN_Sim.exe", stderr=subprocess.PIPE)
    print("end of subprocess call")
    if process.stderr:#把 exe 執行出來的結果讀回來
        print("**************************************************************")
        print process.stderr.readlines()
        print("**************************************************************")
    print("End of program")'''
    os.chdir(".\\3-0.5-0") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-0.5-1") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-0.5-2") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-0.5-4") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])

    os.chdir("..\\3-1.0-0") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-1.0-1") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-1.0-2") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-1.0-4") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])

    os.chdir("..\\3-1.5-0") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-1.5-1") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-1.5-2") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-1.5-4") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])

    os.chdir("..\\3-2.0-0") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-2.0-1") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-2.0-2") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
    os.chdir("..\\3-2.0-4") # change current path
    subprocess.Popen(["WBAN_Sim.exe"])
 
if __name__ == "__main__":
    main()
    print("End of main")
