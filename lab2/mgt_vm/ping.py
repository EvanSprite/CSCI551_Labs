from subprocess import Popen, PIPE
import shlex
from time import sleep

nb_vms = 40

# Load the MAC addresses of the mgt interface on HOUS
mac_dic = {}
# Connectivity dictionnary
co_dic = {}
# Ping processes dic
proc_dic = {}

# Connectivity dictionnary initializatin
for from_g in range(1, nb_vms+1):
    co_dic[from_g] = {}
    for to_g in range(1, nb_vms+1):
        co_dic[from_g][to_g] = False

while True:
    fd = open('../mac_addresses', 'r')
    for line in fd.readlines():
        if len(line) > 1 and '#' not in line:
            linetab = line.split()
            grp = int(linetab[0][1:])
            mac = linetab[1].rstrip('\n')

            if 'g' not in mac:
                mac_dic[grp] = mac
    fd.close()

    # Perform the ping measurements
    for from_g in range(1, nb_vms+1):
        if from_g in mac_dic:
            proc_dic[from_g] = {}
            for to_g in range(1, nb_vms+1):
                if to_g == 39:
                    continue
                cmd =  "nping --dest-mac "+mac_dic[from_g]+" --source-ip "+str(from_g)+".0.199.2 --dest-ip "+str(to_g)+".102.0.1 --interface g"+str(from_g)+" -c 3 --delay 500ms"
                proc_dic[from_g][to_g] = Popen(shlex.split(cmd), stdout=PIPE)

            for to_g in proc_dic[from_g]:
                if to_g == 39:
                    continue
                output_tmp = proc_dic[from_g][to_g].communicate()[0]
                if "Echo reply" in output_tmp:
                    co_dic[from_g][to_g] = True
                    print "Connectivity Between "+str(from_g)+" and "+str(to_g)
                else:
                    co_dic[from_g][to_g] = False
                    print "No Connectivity Between "+str(from_g)+" and "+str(to_g)
                    print output_tmp
                    #print output_tmp.split('\n')[3]
            sleep(1)




    success_symbol = 'glyphicon glyphicon-ok text-success'
    fail_symbol = 'glyphicon glyphicon-remove text-danger'

    fd = open('final.html', 'r')
    html_str = fd.read()
    fd.close()

    # Modify the html file accordingly
    for from_g in range(1, nb_vms+1):
        for to_g in range(1, nb_vms+1):
            if co_dic[from_g][to_g]:
                html_str = html_str.replace('<!-- BIN:'+str(from_g)+'-'+str(to_g)+' -->', '"'+success_symbol+'"')
            else:
                html_str = html_str.replace('<!-- BIN:'+str(from_g)+'-'+str(to_g)+' -->', '"'+fail_symbol+'"')

    fd = open('matrix.html', 'w')
    fd.write(html_str)
    fd.close()
