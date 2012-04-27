#Test modules for server and client program.
#Written by: Kenneth Chik 2012

import unittest
import socket
from threading import Thread
from subprocess import check_output
from time import sleep
from os import system,path

HOST = ''
LOCALHOST='127.0.0.1'
PORT = 50025
BACKLOG = 5
SIZE = 1024

def waitForConnection(port):
    """Function used to wait for a connection to come up.
    """
    tries = 5 #Max time in seconds to wait.
    connected = False
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    while connected == False:
        if tries < 0:
            return False
        try:
            s.connect((LOCALHOST,port))
            connected = True
        except:
            sleep(1)
            tries -= 1
    s.close()
    return True  


class Test_Client(unittest.TestCase):
    """Tests for client program.
    """
    server_socket=None
    client_socket=None  
    t=None              #Server thread
    run_server=True     #Flag to shutdown the server.
    send_filter=None    #Used to filter the data echoed back from the mock server
    
    def startEchoServer(self):
        """starts the mock echo server used to test the client program. The server will be located
        at localhost on port defined by the PORT constant. The mock server is able to transform
        data using the send_filter."""
        self.run_server=True
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) #set for address reuse.
        self.server_socket.bind((LOCALHOST,PORT))
        self.server_socket.listen(BACKLOG)
        while self.run_server: #run_server is set from stopEchoServer method to break this loop.
            client, address = self.server_socket.accept()                    
            data = client.recv(SIZE)                        
            while data and self.run_server:
                if self.send_filter is not None:
                    data = self.send_filter(data) #used to transform the received data.
                client.send(data)
                data = client.recv(SIZE)
            client.close() 
    
    def stopEchoServer(self):
        """Stops the Mock Echo server.
        """
        self.run_server = False
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect((LOCALHOST,PORT))
        self.client_socket.send('a') #Sends any data used to break the while loop of startEchoServer.
    
    def setUp(self):
        #print "start setUp"
        send_filter=None
        self.t = Thread(target=self.startEchoServer)
        self.t.start() #Run the mock echo server on a new thread.
        self.assertTrue(waitForConnection(PORT)) #Wait for the echo server to come up.
        #print "finished setUp"
        
    def tearDown(self):
        #print "Start tearDown"
        self.stopEchoServer()
        self.t.join() #Wait for mock echo server to really finish.
        self.server_socket.close()
        self.client_socket.close()
        #print "finished tearDown"      
        
    def test_python_server(self):
        """Tests the mock python server to see if it can echo data.
        """
        print "Start test_python_server"
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((LOCALHOST,PORT))
        s.send('Hello World!')
        data = s.recv(SIZE)
        s.close()
        self.assertEqual('Hello World!',data)
        
    def test_normal_case(self):
        """Tests normal case of the usage of the client program.
        """
        print "Start test_normal_case"
        r = check_output(['./client',LOCALHOST,'5',str(PORT)])
        self.assertEqual( 'Received: 1 2 3 4 5',r.split('\n')[0].strip())
        
    def test_one_case(self):
        """ Tests only send one character
        """
        print "Start test_one_case"
        r = check_output(['./client',LOCALHOST,'1',str(PORT)])
        self.assertEqual( 'Received: 1',r.split('\n')[0].strip())
        
    def test_drop_packets(self):
        """ Tests if the some packets cannot reach the client.
        The client is supposed to give up on the missing data after a period of time.
        Then work out the stats without the missing data."""
        print "Start test_drop_packets"
        self.send_filter=lambda x : x if len(x) <= 1 else x[1]        
        r = check_output(['./client',LOCALHOST,'5',str(PORT)])
        print "==========================="
        print "missing return data output:\n"
        print r
        print "==========================="                
        self.assertTrue(len(r.split('\n')[0].strip().split(' '))<6)
        
class Test_Server(unittest.TestCase):
    """Tests for the server program.
    """
    def setUp(self):
        #print "start setUp"        
        system("./server %d &" % (PORT+1)) #Start the server program on a different process.
        self.assertTrue(waitForConnection(PORT+1)) #Wait for the server to come up.
        #print "finished setUp"
        
    def tearDown(self):
        #print "Start tearDown"
        system("killall server")
        #print "finished tearDown"
        
    def test_char_server(self):
        """Tests only sending one character to the server
        """
        print "Start test_char_server"
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((LOCALHOST,PORT+1))
        s.send('H')
        data = s.recv(SIZE)
        s.close()
        self.assertEqual('H',data)
        
    def test_string_server(self):
        """Tests sending a string of characters to the server.
        """
        print "Start test_char_server"
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((LOCALHOST,PORT+1))
        s.send('Hello World!')
        buf = ''
        while len(buf) < 12:
            buf += s.recv(SIZE)        
        s.close()
        self.assertEqual('Hello World!',buf)
        
    def test_sudden_connection_cut(self):
        """Tests cutting the connection right after sending a string to the server.
        The server is not supposed to die."""
        print "Start test_sudden_connection_cut"
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((LOCALHOST,PORT+1))
        s.send('Hello World!'*7)
        s.close() #cut connection
        sleep(0.5)
        self.assertTrue(waitForConnection(PORT+1)) #Server still supposed to be up and accepting connections.
      
class Test_Integration(unittest.TestCase):
    """Tests to test the server and the client program operating together.
    """
    def setUp(self):
        #print "start setUp"        
        system("./server %d &" % (PORT+1))
        self.assertTrue(waitForConnection(PORT+1))
        #print "finished setUp"
        
    def tearDown(self):
        #print "Start tearDown"
        system("killall server")
        #print "finished tearDown"
        
    def test_integration_normal_case(self):
        """Test sending a string to the server.
        """
        print "Start test_integration_normal_case"
        r = check_output(['./client',LOCALHOST,'5',str(PORT+1)])
        self.assertEqual( 'Received: 1 2 3 4 5',r.split('\n')[0].strip())
        
    def test_integration_one_case(self):
        """Test sending a character only to the server.
        """
        print "Start test_integration_one_case"
        r = check_output(['./client',LOCALHOST,'1',str(PORT+1)])
        self.assertEqual( 'Received: 1',r.split('\n')[0].strip())   
        
if __name__ == '__main__':
    if path.isfile('server') and path.isfile('client'):
        unittest.main()
    else:
        print "Cannot find server and client programs in the same directory as test.py\nMaybe you need to compile the client and server program with make first"
