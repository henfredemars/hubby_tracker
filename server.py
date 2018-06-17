
#!/usr/bin/python3

from flask import Flask, request
import sys
from logzero import logger
import serial

app = Flask(__name__)

ARDUINO_HELLO = 'HELLO RPI!'.encode('ASCII')

ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)

@app.route('/', methods=['POST', 'GET'])
def main():
  if request.method == 'GET':
    logger.info("Got a GET request. This shouldn't happen often.")
    return "HappyTracker API server on Saucy."
  else:
    d = int(request.values['ring'])
    logger.info("Received: %d" % d)

    # Handshake
    while True:
      try:
        ser.write(ARDUINO_HELLO)
        r = ser.read(len(ARDUINO_HELLO)).decode('ASCII')
        if r == ARDUINO_HELLO:
          break
        logger.warn('Got unexpected serial reply: ' + r)
      except serial.SerialTimeoutException:
        logger.warn('Serial timeout. Trying again...')
      ser.write(('RING %d!' % d).encode('ASCII'))
  return "Done!"

