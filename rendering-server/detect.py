#!/usr/bin/python

import os
import sys
import csv
import time

PIXELS = 1920 * 1200
FRAMERATE = 10
THRESHOLD = 0.5
begin = -1
end = -1
last_change = -1
renderingTime = {}

def setPixels():
	try:
		line = open("resolution", "r").readline()
		parts = line.split("x")
		width = int(parts[0])
		height = int(parts[1])
		PIXELS = width* height
	except Exception, e:
		print "Error in reading resolution"
#get the page load time reported by the webdriver browser
def getBroserPLT(bPLTFilename):
	broserPLT = ""
	try:
		line = open(bPLTFilename, "r").readline()
		broserPLT = line.trim()
		os.remove(bPLTFilename)
	except Exception, e:
		print "Error in reading resolution"
	return broserPLT
if __name__ == '__main__':
	setPixels()
	difftxt = sys.argv[2] + ".txt"
	website = sys.argv[3]
	try:
		lines = open(difftxt, 'r').readlines()
		for line in lines[1:]:
			line = line.strip()
			parts = line.split(" ")
			prev = int(parts[0])
			i = int(parts[1])
			errors = float(parts[2])
			delta = 100.0 * errors / PIXELS
			print delta
			if begin < 0 and delta > THRESHOLD:
				begin = i
				last_change = 0
				continue
			if delta > THRESHOLD:
				end = i
				last_change = 0
			if last_change != -1 and errors == 0:
				last_change += 1
				if end < 0:
					end = i  # incase the end never set 
				rt = ((end - begin) / float(FRAMERATE)) * 1000
				key = last_change * 10 
				if not renderingTime.has_key(key):
					renderingTime[key] = rt
			if last_change == 30:
				break
		
		rendTime = ((end - begin) / float(FRAMERATE)) * 1000
	
		if len(renderingTime) < 10:  # means that there is very small pixel difference and the web site doesn't reach stable state 
			maxIndex = max(renderingTime.keys())
			while maxIndex < 100:
				maxIndex +=10
				renderingTime[maxIndex] = rendTime
			
		csvfn = sys.argv[1]
		keyset = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]  # for 10%, 20% , etc of the frame is not changing 
		
		log = ""
		try:
			csvin = open(csvfn, "rb")
			log += "file name = " + csvfn + "\n"
			csvout = open(csvfn + ".rendering", "wb+")
			reader = csv.reader(csvin, delimiter=';', quoting=csv.QUOTE_NONE)
			writer = csv.writer(csvout, delimiter=';', quoting=csv.QUOTE_NONE)
			if reader != None:
				lastline = reader.next()
				for line in reader:
					lastline = line
				for k in keyset:
					lastline.append(renderingTime.get(k))
					log+= "vaule = "+str(renderingTime.get(k))
				
				broserPLT = ""
				try:
					tmpFN = csvfn.replace(".csv", "")+".tmp"
					line = open(tmpFN, "r").read()
					broserPLT = line.trim()
					os.remove(tmpFN)
				except Exception, e:
					print "Error in reading resolution"
				#broserPLT = getBroserPLT(csvfn.replace(".csv", "")+".tmp")
				lastline.append(broserPLT)
				writer.writerow(lastline)
				log += "write is successful \n"
			else:
				log += "reader not found..."
			csvout.close()
			csvin.close()
			# os.rename(csvfn, csvfn+".old")
			# os.rename(csvfn+".new", csvfn)
		except Exception, e:
			log += "Exception: " + str(e)
		logfile = open("log.txt", 'w')
		logfile.write(log)
		logfile.close()
		
		
		strbegin = "Begin render: %d" % begin
		strend = "End render: %d" % end
		strrender = "Rendering time: %.2f seconds" % rendTime 
	
		renderingStr = ""
		
		for k in sorted(renderingTime):
			renderingStr += str(renderingTime.get(k)) + " "
		renderingFile = website
		if not os.path.exists("output/" + renderingFile):
			tmpstr = "10 20 30 40 50 60 70 80 90 100\n"
			f = open("output/" + renderingFile, "w")
			f.write(tmpstr)
			f.write(renderingStr+"\n")
			f.close()
		else:
			f = open ("output/" + renderingFile, 'a')
			f.write(renderingStr)
			f.close
		tmplist = csvfn.split('/')
		leng = len(tmplist)
	    vedioname = tmplist[leng-1].replace(".csv","")
		d = open("finished", "a")
		d.write("%s\t%s" % (vedioname, time.asctime()))
	    d.close()
	except IOError as e:
		print "IO Error occured " 
