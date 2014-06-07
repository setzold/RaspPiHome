import urllib
import urllib2
import json

myData = []

#set variable from outer context
url = 'http://myservice.tld/'

def addSensorData(sensorData):
	myData.append(sensorData)
	print sensorData + ' added'

def sendSensorData():
	print 'Send to url: ' + url;
	uniqueSensors = {}
	#split data to nibbles
	for sensorValue in myData:
        	uniqueSensors[sensorValue[:2]] = ((float(sensorValue[3:6]) / 10) - 40)
	
	sensors = []
	for sensor, temp  in uniqueSensors.items():
		current = {'sensor':sensor,'temp':temp}
		print current
		sensors.append(current)	
		#values = [{'sensor' : 'eins', 'temp' : 11.2}, {'sensor':'sens1','temp':23.4}]

	data=json.dumps(sensors)
	print 'Data: ' + data

	headers = {'Content-type': 'application/json', 'Accept': 'text/plain'}
	req = urllib2.Request(url, data, headers)
	response = urllib2.urlopen(req)
	the_page = response.read()
	
	myData[:] = []

"""
# Testdata
readedValues = [
'94C5226AEA',
'9285186A4D',
'9B06216AA4',
'9285186A4D',
'9B06216AA4',
'94C5226AEA'
]

for value in readedValues:
	addSensorData(value)

sendSensorData()
"""
