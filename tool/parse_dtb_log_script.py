import re
import os
import sys
from collections import defaultdict

g_dtb_name  = [{} for _ in range(5)]
g_used_node_name = [{} for _ in range(5)]
g_unused_node_name = [{} for _ in range(5)]
g_unuse_leaf_node_name = [{} for _ in range(5)]
g_not_in_hash_name = [{} for _ in range(5)]
g_cost_creat_hash_time =[None] * 5
g_cost_tatol_time =[None] * 5
g_tatol_access_nodes =[None] * 5
g_blob_address =[]
g_blob_id = -1

RED = '\033[91m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'  # reset

# success return 0 else return negative
# -1 : log file incomplete
# -2 : not define mirco ENABLE_HASH_FOR_DTB , ENABLE_DTB_PROFILING , ENABLE_PRINT_DTB_ALL_NODE
def get_all_and_used_dtb_nodes(file_path=None)->int:
    global g_blob_id 
    start_parse_dtb_node_flag = False
    #dtb log file 
    source = open(file_path, 'r')
    #source = open(r'C:\Users\wenzhong\Desktop\dtb_log.txt', 'r')
    lines = source.readlines()  # read all line form log file
    current_line = 0
    ret_values = 0
    #start parse flag
    try:
        while current_line < len(lines):
            line = lines[current_line].rstrip('\n')
            
            # match "start creat dtb hash table" and store blob address 
            if re.search(r'start creat dtb hash table', line):
               blob_address = re.search(r'blob+.*?\[([^]]+)\]',line)
               if blob_address:
                   g_blob_address.append(blob_address.group(1))
               else:
                   return -1
               start_parse_dtb_node_flag = True
               current_line = current_line +1
               g_blob_id += 1
               continue
            
            # match end create dtb hash table and add uesd node
            if re.search(r'end create dtb hash table', line):
               if start_parse_dtb_node_flag == True:
                    start_parse_dtb_node_flag = False
               else:
                   return -1
               creat_hash_time = re.search(r'end create dtb hash table+.*?\[([^]]+)\]', line)
               if creat_hash_time:
                   g_cost_creat_hash_time[g_blob_id] = creat_hash_time.group(1)
               inner_line = 0
               while inner_line < len(lines):
                    line2 = lines[inner_line].rstrip('\n')
                    blob_address = re.search(r'lookup in+.*?address\[([^]]+)\]',line2)
                    used_node_name = re.search(r'current_node\[([^]]+)\]',line2)
                    cost_tatol_time = re.search(r'access_time\[([^]]+)\]',line2)
                    access_node_count = re.search(r'access_node\[([^]]+)\]',line2)
                    #if blob_address and used_node_name and (g_blob_id > -1) and (blob_address.group(1) == g_blob_address[g_blob_id]) and (used_node_name.group(1) not in g_used_node_name[g_blob_id]):
                    if blob_address and used_node_name and (g_blob_id > -1) and (blob_address.group(1) == g_blob_address[g_blob_id]) and (fix_node_name(used_node_name.group(1)) not in g_used_node_name[g_blob_id]):
                        g_used_node_name[g_blob_id][fix_node_name(used_node_name.group(1))] = hash_djb2(fix_node_name(used_node_name.group(1)))
                    
                    if blob_address and used_node_name and (g_blob_id > -1) and (blob_address.group(1) == g_blob_address[g_blob_id]) and cost_tatol_time and access_node_count:
                        g_cost_tatol_time[g_blob_id] = cost_tatol_time.group(1)
                        g_tatol_access_nodes[g_blob_id] = access_node_count.group(1)

                    if blob_address and used_node_name and (g_blob_id > -1) and (blob_address.group(1) == g_blob_address[g_blob_id]) and (used_node_name.group(1) not in g_dtb_name[g_blob_id]) and (used_node_name.group(1) not in g_not_in_hash_name[g_blob_id]): 
                        g_not_in_hash_name[g_blob_id][used_node_name.group(1)] = hash_djb2(used_node_name.group(1))

                    inner_line += 1
  
            #match "found dtb node..." string 
            if start_parse_dtb_node_flag:
               name = re.search(r'found dtb node:\[([^]]+)\]',line)
               if name:
                    g_dtb_name[g_blob_id][name.group(1)] = hash_djb2(name.group(1))
            
            current_line += 1
    finally:
        if file_path:
            source.close()

    if start_parse_dtb_node_flag == True:
        ret_values = -1
    elif g_blob_address:
        ret_values = 0
    else :
        ret_values = -2

    return ret_values

#fix example node "/soc/memorymap/" to "/soc/memorymap", remove last "/"
def fix_node_name(path):
    if path and path[-1] == '/' and len(path) > 1:
         return path[:-1]
    return path



def get_unuse_node():
    for i in  range(5):
        if g_dtb_name and g_dtb_name[i]:
            g_unused_node_name[i] = {k : g_dtb_name[i][k] for k in  g_dtb_name[i] if (k not in g_used_node_name[i] and not any(any_used_name.startswith(k) and len(any_used_name) > len(k) for any_used_name in g_used_node_name[i] ))}

def get_unuse_leaf_node():
    for i in range(5):
        if g_dtb_name[i] and g_unused_node_name[i]:
                g_unuse_leaf_node_name[i] = {k: g_unused_node_name[i][k] for k in g_unused_node_name[i] if not any(any_name.startswith(k) and len(any_name) > len(k) for any_name in g_dtb_name[i] ) }


def hash_djb2(s: str) -> int:
    
    #DJB2 hash algorithm
    #hash = ((hash << 5) + hash) + c;
    hash_value = 5381
    max_64bit = 2**64  # order to hash value min 64 bit max value
    
    for char in s:
        # get character ascii value
        c = ord(char)
        # get hash value
        hash_value = ((hash_value << 5) + hash_value + c) % max_64bit
    return hash_value

# if return -1 no collision else collision g_dtb_name[i]
def is_hash_collision()-> int:
    for i in range(5):
        if g_dtb_name[i] and (len(g_dtb_name[i].values()) != len(set(g_dtb_name[i].values()))):
            return i
    return -1 


def get_hash_collision_key(d:dict)->dict:
    value_to_keys = defaultdict(list)
    for key, value in d.items():
        value_to_keys[value].append(key)

    return {value:keys for value, keys in value_to_keys.items() if len(keys) > 1}

def handle_hash_collision():
    ret_value = is_hash_collision()
    if(ret_value != -1):
        print(f"{RED}warning : occur hash collision!!!!!!!!!!!!!!!!!!!!!!\n")
        collision_dict = get_hash_collision_key(g_dtb_name[ret_value])
        print("The following nodes collision:")
        for hash_value in collision_dict:
            for name in collision_dict[hash_value]:
                print("node name:", "{:<128}".format(name), "node hash value:", hash_value)
        sys.exit(0)   
    else :
        print(f"{YELLOW}no hash collision occur{RESET}")

def print_node_info():
    # print all dtb name
    for i in  range(5):
        if g_dtb_name[i]:
            #print(f"{YELLOW}************************************************************* blob address:[{g_blob_address[i]}] summary satrt:format[node name---node hash value]***********************************************{RESET}\n")
            print(f"{YELLOW}blob address {g_blob_address[i]} summary{RESET}")
            handle_hash_collision()
            print(f"{YELLOW}dtb tatol nodes: {len(g_dtb_name[i])}{RESET}")
            print(f"{YELLOW}dtb tatol used nodes: {len(g_used_node_name[i])}{RESET}")
            print(f"{YELLOW}dtb tatol unused nodes: {len(g_unused_node_name[i])}{RESET}")
            print(f"{YELLOW}dtb tatol not in hash table nodes: {len(g_not_in_hash_name[i])}{RESET}")
            print(f"{YELLOW}dtb creat hash table time: {g_cost_creat_hash_time[i]} us{RESET}")
            print(f"{YELLOW}dtb cost tatol time: {g_cost_tatol_time[i]} us{RESET}")
            print(f"{YELLOW}########################################## dtb blob address:{g_blob_address[i]} all nodes start #######################################{RESET}")
            print(f"{RED}{'[dtb node name]':<100} [dtb node name hash value]{RESET}")
            for all_dtb_name in g_dtb_name[i]:
                print(f"{all_dtb_name:<100} 0x{g_dtb_name[i][all_dtb_name]:016X}")
            print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} all nodes end #########################################{RESET}\n")

            print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} all used nodes start ###################################{RESET}")
            for used_node in g_used_node_name[i]:
                print(f"{used_node:<100} 0x{g_used_node_name[i][used_node]:016X}")
            print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} all used nodes end #####################################{RESET}\n")

            print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} unused nodes start ######################################{RESET}")
            for unused_node in g_unused_node_name[i]:
                #print(f"{unused_node:<100} 0x{g_unused_node_name[i][unused_node]:016X}")
                print(f"{unused_node:<100}")
            print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} all unused nodes end ####################################{RESET}\n")

            print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} not in hash table nodes start ######################################{RESET}")
            for not_in_hash_node in g_not_in_hash_name[i]:
                print(f"{not_in_hash_node:<100} 0x{g_not_in_hash_name[i][not_in_hash_node]:016X}")
            print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} not in hash table nodes end ####################################{RESET}\n")

            if '-a' in sys.argv or '-all' in sys.argv:
                print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} unused leaf nodes start  ######################################{RESET}")
                for unsued_leaf_node in g_unuse_leaf_node_name[i]:
                    #print(f"{unsued_leaf_node:<100} 0x{g_unuse_leaf_node_name[i][unsued_leaf_node]:016X}")
                    print(f"{unsued_leaf_node:<100}")
                print(f"{YELLOW}########################################## dtb blob address {g_blob_address[i]} unused leaf nodes end ####################################{RESET}\n")
            
            print(f"{YELLOW}************************************************************* blob address:[{g_blob_address[i]}] summary end **************************************************************************************{RESET}\n")

def main():
    status  = 0
    if len(sys.argv) < 2:
        print("please input a dtb log file")
        print(r"example run command: python C:\Users\parse_dtb_log_script.py  C:\Users\dtb_log.txt")
        sys.exit()
    
    if not os.path.exists(sys.argv[1]):
        print("the input log file not exist")
        sys.exit()
        
    status = get_all_and_used_dtb_nodes(sys.argv[1])
    if status == -2:
        print(f"{RED}please enable ENABLE_HASH_FOR_DTB , ENABLE_DTB_PROFILING{RESET}")
        sys.exit()
    elif status == -1:
        print(f"{RED}input log file incomplete{RESET}")
        sys.exit()
    get_unuse_node()
    get_unuse_leaf_node()

    #handle_hash_collision()
    print_node_info()

if __name__ == "__main__":
    main()
      
 