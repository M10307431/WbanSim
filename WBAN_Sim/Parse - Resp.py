# -*- coding: cp950 -*-
import os


def main():
    task = 3
    uti = 0.5
    policy = 0
    
    os.chdir(".\\WBAN_GenResult") # change current path
    output = open('Average_Result.txt','w')

    for s in range(5):
        if s == 0:
            output.writelines("Meet_R\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        elif s == 1:
            output.writelines("Energy\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        elif s == 2:
            output.writelines("Resp\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        elif s == 3:
            output.writelines("CloudEng\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        else:
            output.writelines("FogEng\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        task = 3
        for k in range(6): #3 4 5 6 8 10
            uti = 0.5
            for j in range(4):  #0.5 1.0 1.5 2.0
                policy = 0
                for i in range(4): #0 1 2 4
                    filename = "Result_GW-3_Task-"+str(task)+"_U"+str(uti)+"00000_"+str(policy)+".txt"
                    file = open(filename, 'r')
                    line = file.read().split("============== Average Result ==============\n")[1]
                    file.close()
                    Meet = line.split("\n")[s].split(' : ')[1]
                    output.write(Meet)
                    if policy == 2:
                        policy = 4
                    else:
                        policy += 1
                    output.write('\t')
                output.write('\n')
                uti += 0.5
            if task >= 6:
                task +=2
            else:
                task += 1
        output.write('\n\n')

    #Energy = float(line.split("\n")[1].split(' : ')[1])
    #print type(Meet)
    #print type(Energy)
    
    output.close()
if __name__ == "__main__":
    main()
    print("End of Parse")
