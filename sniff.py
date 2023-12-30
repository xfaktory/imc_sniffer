import serial
import ffDecoder

ser = serial.Serial("COM3", 38400)

bytes = bytearray()

decoder = ffDecoder.ffDecoder()

f = bytearray()

while True:
	count = ser.inWaiting()
	if count > 0:
		buf = ser.read(count)
		for b in buf:
			if (decoder.ff_Decoder(b, f)[0]):
				print (decoder.packet)
	        