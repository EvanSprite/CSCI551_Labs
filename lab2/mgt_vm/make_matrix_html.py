skeleton_file='connectivity_matrix.html'
group_table_file='group_html_base.html'
final_file='final.html'

fd = open(group_table_file, 'r')
group_string = fd.read()
fd.close()



fd = open(skeleton_file, 'r')
skeleton = fd.read()
fd.close

for i in range(1, 41):
    str_tmp = group_string.replace('GroupNumber', 'G'+str(i))
    for j in range(1, 41):
        str_tmp = str_tmp.replace('<!-- LINE:'+str(j)+' -->', '<!-- BIN:'+str(i)+'-'+str(j)+' -->')
    skeleton = skeleton.replace('<!-- GROUP'+str(i)+' -->', str_tmp)

fd = open(final_file, 'w')
fd.write(skeleton)
fd.close()
