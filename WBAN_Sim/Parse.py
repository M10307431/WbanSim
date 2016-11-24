# -*- coding: cp950 -*-
import os


def main():
    task = 3
    uti = 0.5
    policy = 0
    
    os.chdir(".\\WBAN_GenResult") # change current path
    output = open('Average_Result.txt','w')

    for s in range(3):
        if s == 0:
            output.writelines("Meet_R\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        elif s == 1:
            output.writelines("Energy\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        else:
            output.writelines("Resp\nNOFLD\tmyOFLD\tAOFLDC\tSeGW\n")
        task = 3
        for k in range(4):
            uti = 0.5
            for j in range(3):
                policy = 0
                for i in range(4):
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
            task += 1
        output.write('\n\n')

    #Energy = float(line.split("\n")[1].split(' : ')[1])
    #print type(Meet)
    #print type(Energy)
    
    output.close()
if __name__ == "__main__":
    main()
    print("End of main")
