from fabric.api import *

#env.key_filename= 'vm_key'
env.user = "root"
env.password = 'thomas'
#env.host_string = '192.168.56.101'




def test_vm (dst_ip, port_number):
    with settings (host_string=dst_ip+':'+port_number):
        try:
            run ('echo $USER')
        except:
            print "Command failed for "+dst_ip+':'+port_number

def start_mininext (dst_ip, port_number, group_number):
    with settings (host_string=dst_ip+':'+port_number):
        run ("./.start_mininext.sh "+str(group_number))

def set_cred (dst_ip, port_number, new_passwd):
    with settings (host_string=dst_ip+':'+port_number):
        run ("echo \"root:"+new_passwd+"\" | chpasswd")

def set_ip (dst_ip, port_number, ip_eth1):
    with settings (host_string=dst_ip+':'+port_number):
        run ("ifconfig eth1 "+ip_eth1+" up")

def make_vm_connection(dst_ip, port_number, dst_vm, intf):
    with settings (host_string=dst_ip+':'+port_number):
        run ("./.create_tunnels.sh "+str(dst_vm)+" "+str(intf))
        print ("./.create_tunnels.sh "+str(dst_vm)+" "+str(intf))

def make_tunnels(dst_ip, port_number):
    with settings (host_string=dst_ip+':'+port_number):
        run ("./.make_tunnels.sh")
        print ("./.make_tunnels.sh")

def clean_make_tunnels(dst_ip, port_number):
    with settings (host_string=dst_ip+':'+port_number):
        run ("echo '' > .make_tunnels.sh")
