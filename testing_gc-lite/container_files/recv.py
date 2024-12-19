import sys
import socket

# print usage information
def usage(n):
    print(f'Usage: {sys.argv[0]} PROG_SIZE DATA_SIZE ROUNDS')
    sys.exit(n)

# ensure party parameter is provided
if len(sys.argv) < 2:
    usage(1)

# get and verify party parameter
party = int(sys.argv[1])
if party == 0:
    port = 64321
    files = ['/GC-Lite/GC-Lite/inputs/alice/D_A.txt',
             '/GC-Lite/GC-Lite/inputs/alice/P_A.txt',
             '/GC-Lite/GC-Lite/randoms/alice/AlphaRandoms_A.txt',
             '/GC-Lite/GC-Lite/randoms/alice/Permutations_DA.txt',
             '/GC-Lite/GC-Lite/randoms/alice/Permutations_PA.txt',
             '/GC-Lite/GC-Lite/randoms/alice/Pi_diff_R_DB_DA.txt',
             '/GC-Lite/GC-Lite/randoms/alice/Pi_diff_R_PB_PA.txt',
             '/GC-Lite/GC-Lite/randoms/alice/Randoms_A.txt',
             '/GC-Lite/GC-Lite/randoms/alice/R_DA.txt',
             '/GC-Lite/GC-Lite/randoms/alice/R_PA.txt',
             '/GC-Lite/GC-Lite/randoms/alice/U_DA.txt',
             '/GC-Lite/GC-Lite/randoms/alice/U_PA.txt']
elif party == 1:
    port = 64322
    files = ['/GC-Lite/GC-Lite/inputs/bob/D_B.txt',
             '/GC-Lite/GC-Lite/inputs/bob/P_B.txt',
             '/GC-Lite/GC-Lite/randoms/bob/AlphaRandoms_B.txt',
             '/GC-Lite/GC-Lite/randoms/bob/Permutations_DB.txt',
             '/GC-Lite/GC-Lite/randoms/bob/Permutations_PB.txt',
             '/GC-Lite/GC-Lite/randoms/bob/Pi_diff_R_DA_DB.txt',
             '/GC-Lite/GC-Lite/randoms/bob/Pi_diff_R_PA_PB.txt',
             '/GC-Lite/GC-Lite/randoms/bob/Randoms_B.txt',
             '/GC-Lite/GC-Lite/randoms/bob/R_DB.txt',
             '/GC-Lite/GC-Lite/randoms/bob/R_PB.txt',
             '/GC-Lite/GC-Lite/randoms/bob/U_DB.txt',
             '/GC-Lite/GC-Lite/randoms/bob/U_PB.txt']
else:
    print('PARTY must be 0 or 1')
    usage(1)

# connect to sender
helper = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
while (True):
    try:
        helper.connect(("localhost", port))
        break
    except ConnectionRefusedError:
        pass

# receive files
# start by getting file size then full file
for file_path in files:
    print(f'Reading {file_path}')
    with open(file_path, 'wb') as f:
        f.write(helper.recv(int.from_bytes(helper.recv(8), byteorder='big', signed=False), socket.MSG_WAITALL))
