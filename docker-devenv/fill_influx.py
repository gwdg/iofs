#!/usr/bin/env python3

# change if needed
INFLUX_HOSTNAME = "localhost"
INFLUX_PORT = 8086
# end user configuration

import os
import re
import requests
from datetime import datetime

SCRIPT_DIRECTORY = os.path.dirname(os.path.abspath(__file__))
DOTENV_PATH = os.path.join(SCRIPT_DIRECTORY, ".env")

def get_dotenv(pattern):
    with open(DOTENV_PATH, 'r') as dotenv_file:
        for line in dotenv_file:
            line = line.strip()
            match = re.match(pattern, line)
            if match:
                return match.group(1)
    assert False, f"No match for {pattern} found"

INFLUXDB_ADMIN_USER = get_dotenv(r"^INFLUXDB_ADMIN_USER=(.*?)$")
INFLUXDB_ADMIN_PASSWORD = get_dotenv(r"^INFLUXDB_ADMIN_PASSWORD=(.*?)$")
INFLUX_DB = get_dotenv(r"^INFLUX_DB=(.*?)$")

def write_test_data():
    influx_url = f"http://{INFLUX_HOSTNAME}:{INFLUX_PORT}/write"
    params = {
        "db": INFLUX_DB,
        "u": INFLUXDB_ADMIN_USER,
        "p": INFLUXDB_ADMIN_PASSWORD
    }

    data = f"temperature,location=office value=23.5 {int(datetime.utcnow().timestamp())}000000000"

    try:
        response = requests.post(influx_url, params=params, data=data)
        response.raise_for_status()
        print("Data written successfully!")
    except requests.exceptions.RequestException as e:
        print("Failed to write data:", e)



print("INFLUXDB_ADMIN_USER:", INFLUXDB_ADMIN_USER)
print("INFLUXDB_ADMIN_PASSWORD:", INFLUXDB_ADMIN_PASSWORD)
print("INFLUX_DB:", INFLUX_DB)
write_test_data()
print("some data sent!")
