from netaddr import *

tier1 = [1,2,3,4,5]
transit = [6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35]
stub = [36,37,38,39,40]
#transit=[6,7,8,9,10]
#stub=[11,12,13,14,15]

ip = IPNetwork('179.24.0.0/16')
subnets = sorted(list(ip.subnet(24)))

fd = open('vms_connections', 'w')

sub_dic = {}
def get_subnet(i, p):
    if i not in sub_dic:
        sub_dic[i] = {}
    if p not in sub_dic:
        sub_dic[p] = {}
    if i not in sub_dic[p]:
        s = subnets.pop(0)
        sub_dic[p][i] = s
        sub_dic[i][p] = s
    else:
        s = sub_dic[p][i]

    return s

for i in tier1:
    tmp = list(tier1)
    tmp.remove(i)

    customer1 = i+5
    customer2 = i+9 if (i-1)%5 == 0 else i+4

    loc=['NEWY','SALT','SEAT','WASH']
    k = 1
    for j, l in zip(tmp, loc):
        s = get_subnet(i, j)
        fd.write('Tier1\tPeer'+str(k)+'\t'+str(i)+'\t'+str(j)+'\t'+l+'\t'+str(s)+'\n')
        k += 1


    s = get_subnet(i, customer1)
    fd.write('Tier1\tCust1'+'\t'+str(i)+'\t'+str(customer1)+'\t'+'KANS'+'\t'+str(s)+'\n')

    s = get_subnet(i, customer2)
    fd.write('Tier1\tCust2'+'\t'+str(i)+'\t'+str(customer2)+'\t'+'LOSA'+'\t'+str(s)+'\n')

    fd.write('Tier1\tMgnt'+'\t'+str(i)+'\t'+str(99)+'\t'+'HOUS-MGT'+'\t'+str(i)+'.0.199.1'+'/24\n')


for i in transit:
    peer1 = i-4 if i%5==0 else i+1
    peer2 = i+4 if i%5==1 else i-1

    customer1 = i+5
    customer2 = i+9 if (i-1)%5 == 0 else i+4

    provider1 = i-5
    provider2 = i-9 if i%5 == 0 else i-4

    s = get_subnet(i, peer1)
    fd.write('NotTier1\tPeer1'+'\t'+str(i)+'\t'+str(peer1)+'\t'+'WASH'+'\t'+str(s)+'\n')
    s = get_subnet(i, peer2)
    fd.write('NotTier1\tPeer2'+'\t'+str(i)+'\t'+str(peer2)+'\t'+'SALT'+'\t'+str(s)+'\n')

    s = get_subnet(i, customer1)
    fd.write('NotTier1\tCust1'+'\t'+str(i)+'\t'+str(customer1)+'\t'+'KANS'+'\t'+str(s)+'\n')
    s = get_subnet(i, customer2)
    fd.write('NotTier1\tCust2'+'\t'+str(i)+'\t'+str(customer2)+'\t'+'LOSA'+'\t'+str(s)+'\n')

    s = get_subnet(i, provider1)
    fd.write('NotTier1\tProv1'+'\t'+str(i)+'\t'+str(provider1)+'\t'+'NEWY'+'\t'+str(s)+'\n')
    s = get_subnet(i, provider2)
    fd.write('NotTier1\tProv2'+'\t'+str(i)+'\t'+str(provider2)+'\t'+'SEAT'+'\t'+str(s)+'\n')

    fd.write('NotTier1\tMgnt'+'\t'+str(i)+'\t'+str(99)+'\t'+'HOUS-MGT'+'\t'+str(i)+'.0.199.1'+'/24\n')

for i in stub:
    peer1 = i-4 if i%5==0 else i+1
    peer2 = i+4 if i%5==1 else i-1

    customer1 = i+5
    customer2 = i+9 if (i-1)%5 == 0 else i+4

    provider1 = i-5
    provider2 = i-9 if i%5 == 0 else i-4

    s = get_subnet(i, peer1)
    fd.write('Stub\tPeer1'+'\t'+str(i)+'\t'+str(peer1)+'\t'+'WASH'+'\t'+str(s)+'\n')
    s = get_subnet(i, peer2)
    fd.write('Stub\tPeer2'+'\t'+str(i)+'\t'+str(peer2)+'\t'+'SALT'+'\t'+str(s)+'\n')

    s = get_subnet(i, provider1)
    fd.write('Stub\tProv1'+'\t'+str(i)+'\t'+str(provider1)+'\t'+'NEWY'+'\t'+str(s)+'\n')
    s = get_subnet(i, provider2)
    fd.write('Stub\tProv2'+'\t'+str(i)+'\t'+str(provider2)+'\t'+'SEAT'+'\t'+str(s)+'\n')

    fd.write('Stub\tMgnt'+'\t'+str(i)+'\t'+str(99)+'\t'+'HOUS-MGT'+'\t'+str(i)+'.0.199.1'+'/24\n')



print "Done"
