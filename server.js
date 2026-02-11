const express = require('express');
const http = require('http');
const { Server } = require('socket.io');
const { SerialPort } = require('serialport');
const { ReadlineParser } = require('@serialport/parser-readline');

const app = express();
const server = http.createServer(app);
const io = new Server(server);

app.use(express.static(__dirname + '/public'));

const port = new SerialPort({ path: 'COM10', baudRate: 9600 }); // Adjust COM port as needed
const parser = port.pipe(new ReadlineParser({ delimiter: '\r\n' }));

parser.on('data', (data) => {
  // console.log('Data from Arduino:', data); // Debug output
  
  if (data.startsWith('access:')) {
    
    const accessStatus = data.split(':')[1]; // Either 'granted' or 'denied'
    io.emit('accessStatus', { status: accessStatus });
    console.log('Access Status:', accessStatus); // Debug output
  }

  if (data.startsWith('count:')) {
    const count = parseInt(data.split(':')[1]);
    const available = 2 - count;
    const parkingStatus = available > 0 ? 'Available' : 'Full';
    io.emit('parkingData', { count, available, parkingStatus });
  }
});

server.listen(3000, () => {
  console.log('Server running at http://localhost:3000');
});
/* This system connects Arduino with a web application using Node.js.
Arduino sends access and parking data through serial communication.
Node.js receives the data and sends it to the browser using Socket.IO.
The website updates parking status and access information in real-time.*/