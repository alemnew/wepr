#! /usr/bin/python
import sys
import csv

PIXELS = 1920 * 1200
FRAMERATE = 10
THRESHOLD = 0.5
BROWSER_START_UP = 18 #BROWSER start up time in second*FRAMERATE
begin = -1
end = -1
last_change = -1


def round(number):
	dec = number - int(number)
	if dec < 0.5:
		return int(number)
	else:
		return int(number) + 1
	
difftxt = sys.argv[1]
renderingTime = {}
lines = open(difftxt, 'r').readlines()

def reset(i):
	#begin = -1
	end = -i
	last_change = -1
for line in lines[1:]: 
	line = line.strip()
	parts = line.split(" ")
	prev = int(parts[0].strip())
	i = int(parts[1].strip())
	errors = float(parts[2].strip())
	delta = 100.0 * errors / PIXELS 
	'''
	At time t there is delta % difference between the pixels of the two frames
	Which implies that at time t 100% - delta% of the webpage is not changing  
	percentage = 100 - round(delta)
	rt = ((end - begin) / float(FRAMERATE))
	renderingTime[percentage] = rt
	'''
	#print "Difference between frame %s and %s is %s and error is %f" % (prev, i, str(delta), errors)
	print "Prev = %d and i = %d  err = %f del =  %f beg = %d end = %d l = %d" % (prev, i, errors, delta, begin, end, last_change)
	if begin < 0 and delta >= THRESHOLD:
		begin = i
		last_change = 0
		continue
	if delta >= THRESHOLD:
		end = i
		last_change = 0
	if last_change != -1 and errors == 0:
		last_change += 1
		if end < 0:
			end = i
		rt = ((end - begin) / float(FRAMERATE))
		key = last_change*10
		if not renderingTime.has_key(key):
			renderingTime[key] = rt
	if last_change == 10:
		'''if i <= BROWSER_START_UP: # there might be false positive pixel changes specially at the begining
			reset(i)
			renderingTime.clear()
		else:'''
		break	
	
	#print "L= %d b = %d e= %d d = %f er = %f i= %d" % (last_change, begin, end, delta, errors, i)
	print "l = %d" % last_change

rendTime = ((end - begin) / float(FRAMERATE))	
if len(renderingTime) < 100: # means that there is very small pixel difference and the website doesn't reach stable state 
	maxIndex = max(renderingTime.keys())
	while maxIndex <= 100:
		maxIndex +=10
		renderingTime[maxIndex] = rendTime
'''
		- calculate the rendering time 
		- calcualte the pixel change 
'''

strbegin = "Begin render: %d" % begin
strend = "End render: %d" % end
strrender = "Rendering time: %.2f seconds" % rendTime


strtmp = strbegin + "\n" + strend + "\n" + strrender 
print renderingTime
	
print strtmp
