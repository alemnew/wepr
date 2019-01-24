#!/usr/bin/python
import sys
import BaseHTTPServer
import subprocess
from SocketServer import ThreadingMixIn
import threading
import urlparse
import os
import httplib
import errno
import json
import time
import hashlib
import ssl
import code
import socket
import Queue
import multiprocessing 
import threading


VERSION = 0.3

HOST_NAME = '0.0.0.0'
PORT_NUMBER = 2084
DATA_STORE =  os.environ['HOME'] +  "/data-store/"
SIG_KEY = open('auth.key').read()
DELETE_HTML = True
SCREENS = [99,88,77,66,55,44,33,22,11,98,87,76,65,54,43,32,21,10,97,86,75,64,53,42,31,101,102,103,105,104,106,107,108,109,110]
OCCOPIED_SCREENS = []
#define MAXSIZE for the queue
MAXSIZE = 1000000
URLQueue = multiprocessing.Queue()
pathQueue = multiprocessing.Queue()
jsonFnQueue = multiprocessing.Queue()


class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def log_request(code, size):
        pass
    def do_HEAD(s):
        s.send_response(200)
        s.send_header("Content-type", ":q/html")
        s.end_headers()
    def do_POST(s):
        request = {}
        separator = "_"
        #macAdd = ""
        timeStamp = ""
        remoteAddress = s.client_address[0]
        log = "%s\t%s" % (time.asctime(), remoteAddress)
           # print "clinet address is  " + remoteAddress
        requestURL = urlparse.urlparse(s.path)
        requestParameters = urlparse.parse_qs(requestURL.query)
        if not "MAC" in requestParameters:
            log += "MAC address missing"
            print log
            s.send_response(400)
                # s.send_head("Content-Length", 0)
            s.end_headers()
            return
        if not "TS" in requestParameters:
            log += "Time stamp missing"
            print log
            s.send_response(400)
            s.end_headers()
            return 
        if not "mainURL" in requestParameters:
            log += "URL missing"
            print log
            s.send_response(400)
            s.end_headers()
            return            
        timeStamp = requestParameters["TS"][0]
        macAddress = requestParameters["MAC"][0]
        url = requestParameters["mainURL"][0]
        filename = macAddress + separator + timeStamp
        line = ""
           
        storedir = os.environ['HOME'] +  "/data-store/" 
        directory = storedir+ filename + "/"
        jsonfn = directory + filename + ".json"
            
        URLQueue.put(url)
        pathQueue.put(directory)
        jsonFnQueue.put(jsonfn)
        
        #debuging
        f = open("queue", "a")
        s1 = URLQueue.qsize()
        s2 = pathQueue.qsize()
        s3 = jsonFnQueue.qsize()        
        tm = time.asctime()
        strw = filename + " -> "+tm + " : " + str(s1)+"," + str(s2) +"," + str(s3)+"\n"
        f.write(strw)
        f.close()
        
        #Check if there is previous rendering result 
        previousRendering = []
        for i in os.listdir(storedir):
            if os.path.isdir(os.path.join(storedir, i)) and  i.startswith(macAddress):
                tmpdir =os.path.join(storedir, i)
                for csvf in os.listdir(tmpdir):
                    if csvf.endswith(".rendering"):
                        previousRendering.append(i)
            
            #Get all previous result and send to the probes
        if len(previousRendering) > 0:
            renderingOutput = ""
            for k in previousRendering:    
                tmpPath = os.path.join(storedir, k)
                for csvfile in os.listdir(tmpPath):
                    if csvfile.endswith(".rendering"):
                        f = open(os.path.join(tmpPath, csvfile))
                        renderingOutput += f.read()
                        f.close()
                #rename the directory sothat in the next round it will not be considered
                os.rename(tmpPath, os.path.join(storedir, "__"+k))
            s.send_response(200) 
            s.send_header("Content-Length", len(renderingOutput))
            s.send_header("Content-type", "text/csv")
            s.end_headers()
            s.wfile.write(renderingOutput)
            
            del  previousRendering[:]
        else:
            s.send_response(201) 

def myScheduler():
    b = True
    #start the thread here
    # call in side the loop to execute some task 
    while b:
        try:
            while pathQueue.qsize() <= 0:
                try:
                    time.sleep(1)
                except KeyboardInterrupt:
                    b = False
                    raise
                    break
                
            srtT = time.asctime()
            directory = pathQueue.get()
            jsonfn = jsonFnQueue.get()
            url = URLQueue.get()
            
            csvfn = jsonfn.replace(".json", ".csv")
                
            #for rendering vedio name
            tmplist = jsonfn.split('/')
            leng = len(tmplist)
            vedioname = tmplist[leng-1].replace(".json","")
            log = ""
            try:
                   
                clientsock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                clientsock.connect(('localhost', 2083))
                
                clientsock.settimeout(5)    #5 sec timeout for the socket,... 
                clientsock.sendall(directory) 
                rcv = clientsock.recv(1024)
                 
                clientsock.sendall(jsonfn)
                rcv = clientsock.recv(1024)
                #print log                
                clientsock.close() #close connection with the play back server after sending the data
                
            except socket.timeout:
                log += "%s : Socket time out\n" % (time.asctime())
               # pass
            screen = SCREENS.pop(0) #load balancing if the number of request is larger 
            SCREENS.append(screen)
            renderProcess = subprocess.Popen(['./rendering-time.sh', url, csvfn, vedioname, screen], stdout=subprocess.PIPE)
           # renderProcess.wait() #wait the started rendering process to finish 
            tm = time.asctime()
            f = open("processingtime", "a")
            s1 = URLQueue.qsize()
            strw = vedioname + "\t" + srtT + "\t" + tm + " : " + str(s1)+"\n"
            f.write(strw)
            f.close()
            f = open("log", "a")
            f.write(log)
            f.close()
            
        except KeyboardInterrupt:
            b = False
            raise
            pass

class ThreadedHTTPServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
        """Handle requests in a separate thread."""

if __name__ == '__main__':
    server_class = ThreadedHTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
    httpd.socket = ssl.wrap_socket(httpd.socket, certfile='leone-ssl.pem', server_side=True)
    print "LEONE rendering server"
    
    renderingSched = multiprocessing.Process(name = "scheduler", target=myScheduler)
    try:
        renderingSched.start()
    except KeyboardInterrupt:
        renderingSched.terminate()
        pass
    
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
       renderingSched.terminate()
       pass
    httpd.server_close()
    print "Sever closed. "
                