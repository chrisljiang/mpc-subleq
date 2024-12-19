import socket
import os

# files to send
alice_files = ['/GC-Lite/GC-Lite/inputs/alice/D_A.txt',
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
bob_files = ['/GC-Lite/GC-Lite/inputs/bob/D_B.txt',
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

# create socket to send files to alice
alice = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
alice.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
alice.bind(("localhost", 64321))
alice.listen()

# create socket to send files to bob
bob = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
bob.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
bob.bind(("localhost", 64322))
bob.listen()

# accept connections
alice = alice.accept()[0]
bob = bob.accept()[0]

# send files to alice and bob
# start by sending file size then full file
for alice_path, bob_path in zip(alice_files, bob_files):
    print(f'Sending {alice_path} and {bob_path}')
    alice.send(os.path.getsize(alice_path).to_bytes(8, byteorder='big', signed=False))
    bob.send(os.path.getsize(bob_path).to_bytes(8, byteorder='big', signed=False))
    with open(alice_path, 'rb') as alice_file:
        alice.sendfile(alice_file)
    with open(bob_path, 'rb') as bob_file:
        bob.sendfile(bob_file)
