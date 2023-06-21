import sys
file1 = open('read.txt', 'r')
line = file1.readlines()
line2 = []
line3 = []
for i in line:
    if(i[5:13] == "mlx5_1/1"):
        line2.append(i)
for k in line2[2:]:
    cc = k[58:67];
    cc = cc.replace(' ', '')
    cc = cc.replace('s', '')
    cc = cc.replace('q', '')
    cc = cc.replace('-', '')
    cc = cc.replace('p', '')
    print(cc)
    line3.append(int(cc))
fl = open("new.txt", "w")
for i in range(len(line3)):
    string = "        p4_pd.register_write_psn(" + str(i) + ", unsigned_to_signed(" + str(line3[i] - 1) + " + pow(2, 31), 32))\n"
    #string += "        p4_pd.register_write_psn_def(" + str(i) + ", " + str(line3[i] - 1) + ")\n"
    fl.write(string)
fl.close()
