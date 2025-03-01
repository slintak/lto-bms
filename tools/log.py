#!/usr/bin/env python3

import serial
import sys
import time
import os

from dotenv import load_dotenv

from influxdb_client import InfluxDBClient
from influxdb_client.client.write_api import SYNCHRONOUS

if len(sys.argv) < 2:
    print(f"Usage: {sys.argv[0]} [COM]")
    exit(1)

load_dotenv()
bucket = os.getenv("INFLUXDB_BUCKET")
org = os.getenv("INFLUXDB_ORG")
token = os.getenv("INFLUXDB_TOKEN")
url = os.getenv("INFLUXDB_URL")

port = serial.Serial(sys.argv[1], baudrate=9600, timeout=3.0)

client = InfluxDBClient(url=url, token=token, org=org)
write_api = client.write_api(write_options=SYNCHRONOUS)

while True:
    try:
        rcv = port.readline().decode('utf-8').strip()
    except UnicodeDecodeError:
        continue

    timestamp = time.time_ns()
    line = f"{rcv} {timestamp}\n"

    sys.stdout.write(line)
    sys.stdout.flush()

    write_api.write(
        bucket=bucket,
        record=line,
    )
