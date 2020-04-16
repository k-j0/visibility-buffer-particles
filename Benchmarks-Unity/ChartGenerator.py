import plotly.graph_objects as go
import pandas as pd
import numpy as np
import math
import os

# chart parameters
csvfile = 'Frametime-Full'		# source CSV file
outputDir = 'Frametime-OUTPUT'		# destination graph file's directory
dataTitle = "Frametime (ms)"		# main axis name
boxAndWhiskers = False		# whether to render b&w or bar chart
def Result(renderer, mode, pCount, rWidth, rHeight, pComplexity, pDensity, pSpread):
	# Return None if we should skip this column; otherwise return the name of the column.
	#if renderer != "VBuffer":
	#	return None
	if mode != "VertVert":
		return None
	if pCount != 1048576:
		return None
	if rWidth != 1024:
		return None
	if rHeight != 768:
		return None
	if pComplexity != 2:
		return None
	if pDensity != 400:
		return None
	#if pSpread != 29:
	#	return None
	return renderer + ", " + str(float(pSpread if pSpread != 29 else 30)*0.001)
def getColour(renderer, mode, pCount, rWidth, rHeight, pComplexity, pDensity, pSpread):
	# Returns the colour to use for a certain Series.
	hue = 0
	sat = 73 if boxAndWhiskers else 40
	val = 50
	# assign hue depending on renderer
	if renderer == "Forward":
		hue = 0
	elif renderer == "GBuffer3":
		hue = 90
	elif renderer == "GBuffer6":
		hue = 180
	else:
		hue = 270
	# assign value depending on current variable for test
	#if mode == "CompComp":
	#	val = 65
	#elif mode == "GeomGeom":
	#	val = 55
	#elif mode == "VertVert":
	#	val = 45
	#else:
	#	val = 35
	# assign value depending on density
	if pSpread == 3:
		val = 65
	elif pComplexity == 29:
		val = 50
	elif pComplexity == 300:
		val = 35
	# return colour as HSV
	return "hsl("+str(hue)+","+str(sat)+"%,"+str(val)+"%)"
def Settings(): # the string returned by this function will be written to a text file ".settings" in the destination folder.
	str = outputDir + "\n\n"
	#str += "Renderer: VBuffer\n"
	str += "Mode: VertVert\n"
	str += "ParticleCount: 1048576\n"
	str += "ResolutionWidth: 1024\n"
	str += "ResolutionHeight: 768\n"
	str += "ParticleComplexity: 2\n"
	str += "ParticleDensity: 400\n"
	#str += "ParticleSpread: 29\n"
	return str


firstCol = 4	# index in the CSV file of the first data column

gpu = None
renderer = None
mode = None

# Create an empty chart
fig = go.Figure()

# Function to create a directory if it doesn't exist
def mkdir(path):
	try:
		os.mkdir(path)
	except OSError:
		return

# Update the charts, save as HTML file, and show it in the default browser.
def ShowResults():
	global fig, gpu
	fig.update_layout(title=gpu, yaxis_title=dataTitle, plot_bgcolor='hsl(180,20%,98%)')
	if not os.path.isfile('graphs/'):
		mkdir('graphs/')
	if not os.path.isfile('graphs/'+outputDir+'/'):
		mkdir('graphs/'+outputDir+'/')
	fig.write_html('graphs/' + outputDir + '/' + gpu + ".html", auto_open=True)
	fig = go.Figure()

# Iterate over each column, reading the data.
df = pd.read_csv("src/" + csvfile + ".csv")
col = 0
for(columnName, columnData) in df.iteritems():
	if col >= firstCol and col < len(df.columns)-1: # ignore initial Settings columns and very last column
		if columnName[0:7] != "Unnamed":
			if gpu != None: # output current data
				ShowResults()
			gpu = columnName
		if type(columnData[0]) == type("str"):
			renderer = columnData[0]
		if type(columnData[1]) == type("str"):
			mode = columnData[1]
		pCount = int(columnData[2])
		rWidth = int(columnData[3])
		rHeight = int(columnData[4])
		pComplexity = int(columnData[5])
		pDensity = int(columnData[6])
		pSpread = int(columnData[7])
		avg = columnData[9]
		name = Result(renderer, mode, pCount, rWidth, rHeight, pComplexity, pDensity, pSpread)
		colour = getColour(renderer, mode, pCount, rWidth, rHeight, pComplexity, pDensity, pSpread)
		if name != None:
			if boxAndWhiskers:
				fig.add_trace(go.Box(y=columnData[17:columnData.size-1], name=name, line=dict(color=colour)))
			else:
				fig.add_trace(go.Bar(x=[name], y=[avg], name=name, marker=dict(color=colour)))
	col+=1

ShowResults()
settingsFile = open("graphs/" + outputDir + "/.settings.txt", "w")
settingsFile.write(Settings())
settingsFile.close()