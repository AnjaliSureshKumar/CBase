#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "Building..."
make -s

echo "Starting server..."
./server &
SERVER_PID=$!

sleep 0.5

echo "Starting client 1..."
./client &

sleep 0.3

echo "Starting client 2..."
./client &

echo "Server PID: $SERVER_PID"
echo "Press Ctrl+C to stop the server."

trap "kill $SERVER_PID 2>/dev/null; echo 'Server stopped.'" INT TERM

wait $SERVER_PID
