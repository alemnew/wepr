#!/usr/bin/python
import BaseHTTPServer
import subprocess
from random import randint
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

VERSION=0.2

HOST_NAME = '0.0.0.0'
PORT_NUMBER = 2080
DATA_STORE='/home/parsing-server/data-store'
#SIG_KEY=open('auth.key').read()
DELETE_HTML=True

class MyHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def log_request(code, size):
		pass
		
	def do_HEAD(s):
		s.send_response(200)
		s.send_header("Content-type", "text/html")
		s.end_headers()

	def do_POST(s):
		try:
			request = { }
			#Parse request URL
			serverTimestamp = int(time.time())
			remoteAddress = s.client_address[0]
			log = "%s\t%s" % (time.asctime(), remoteAddress)
			requestURL = urlparse.urlparse(s.path)
			requestParameters = urlparse.parse_qs(requestURL.query)
			if not "MAC" in requestParameters:
				log += "\tMAC parameter missing."
				print log
				s.send_response(400)
				s.send_header("Content-Length", 0);
				s.end_headers()
				return
			if not "mainURL" in requestParameters:
				log += "\tMain URL parameter missing."
				print log
				s.send_response(400)
				s.send_header("Content-Length", 0);
				s.end_headers()
				return
			if not "timestamp" in requestParameters:
				log += "\tTimestamp parameter missing."
				print log
				s.send_response(400)
				s.send_header("Content-Length", 0);
				s.end_headers()
				return
			MAC = requestParameters["MAC"][0]
			mainURL = requestParameters["mainURL"][0]
			timestamp = requestParameters["timestamp"][0]
			
			log += "\t%s\t\"%s\"" % (MAC, mainURL)
			
			#Save submitted HTML to local file
			length = int(s.headers['Content-Length'])
			HTMLData = s.rfile.read(length);
			HTMLHash = hashlib.md5(HTMLData).hexdigest()
			HTMLFile = "%s/%s.html" % (DATA_STORE, HTMLHash)
			elementsFile = "%s/%s.elements" % (DATA_STORE, HTMLHash)
			if not os.path.isfile(elementsFile):
				log += "\tCACHE-MISS"
				fpHTML = open(HTMLFile, 'w')
				fpHTML.write(HTMLData)
				fpHTML.close()
			
				#Call parser script
				fpList = open(elementsFile, 'w')
				parserProcess = subprocess.Popen(['./parse-page.js', HTMLFile, mainURL],stdout=subprocess.PIPE)
				elements = []
				elementsText = ""
				line = parserProcess.stdout.readline()
				nrofElements = 0
				while line != '':
					if line not in elements:
						elements.append(line)
						elementsText += line
						nrofElements += 1
					line = parserProcess.stdout.readline()
				
				parserProcess.wait()
				fpList.write(elementsText)
				fpList.close()
				parserExitCode = parserProcess.returncode
				nrofElements = nrofElements
				#Delete HTML file
				os.remove(HTMLFile)
			else:
				#The HTML has already been parsed.
				elementsText = open(elementsFile).read()
				log += "\tCACHE-HIT"
				nrofElements = len(elementsText.split("\n"))
			
			log += "\t%d elements" % nrofElements

			s.send_response(200)
			s.send_header("Content-Length", len(elementsText))
			s.send_header("Content-type", "text/plain")
			s.end_headers()
			s.wfile.write(elementsText)
			print log
		except Exception as e:
			log += "\t%s" % str(e)
			print log
			s.send_response(500)
			s.send_header("Content-Length", 0);
			s.end_headers()

class ThreadedHTTPServer(ThreadingMixIn, BaseHTTPServer.HTTPServer):
	"""Handle requests in a separate thread."""
	
if __name__ == '__main__':
	server_class = ThreadedHTTPServer
	httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
	httpd.socket = ssl.wrap_socket (httpd.socket, certfile='leone-ssl.pem', server_side=True)
	print time.asctime(), "LEONE Parsing Server v. %s started." % VERSION
	cwd = os.getcwd()
	if DELETE_HTML:
		print time.asctime(), "HTML files will be deleted."
	else:
		print time.asctime(), "HTML files will NOT be deleted -- monitor disk usage regularly."
	print time.asctime(), "Working directory: %s" % cwd
	if not os.path.isdir(DATA_STORE):
		print time.asctime(), "Creating data store path: %s" % DATA_STORE
		os.mkdir("/tmp/leone-render/");
	print time.asctime(), "Waiting for connections on %s:%s" % (HOST_NAME, PORT_NUMBER)
	try:
        	httpd.serve_forever()
	except KeyboardInterrupt:
        	pass
	httpd.server_close()
	print time.asctime(), "Server Stops - %s:%s" % (HOST_NAME, PORT_NUMBER)
