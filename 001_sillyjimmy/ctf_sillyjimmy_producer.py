## My first consumer-producer calculation server
## Now with encryption and authentication!
## By Jimmy 

import socket
import consumer
import jimmy

PORT = 7331

# start consumer
consumer.start()

# start server
myserver = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
myserver.bind(('', PORT))
myserver.listen(10)

# wait for a client to connect
client, dumbperson = myserver.accept()
print('Someone connected', dumbperson)
plaindataisbad = client.recv(100)

# make the data impossible to decrypt
# only the consumer knows how to decrypt it
# Jimmy is the best! HAHAHAHA
crypted = jimmy.encrypt(plaindataisbad)

# check for the secret key, Jimmy can't allow intruders!
if crypted[4] == 0xbc && crypted[6] == 0xa8 && crypted[8]== 0xa8 && crypted[10] == 0xac && crypted[12] == 0xaf:
    # ok good, now send the data to the consumer server and get the result
    consumer.send(crypted)
    result = consumer.read()
    # now do something with the result
    if result != '\xfe':
        client.send(result)
    else:
        # nobody can get this muahahaha
        secret = open('jimmys secret.txt', 'r')
        client.send(secret.readline())
else:
    # we have an intruder, kick him out!
    client.send(b'Jimmy doesnt like your shellcode :/\n')

