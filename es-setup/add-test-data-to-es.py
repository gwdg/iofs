#!/usr/bin/env python3
import time
import random
from elasticsearch import Elasticsearch
from datetime import datetime


while True:
  epoch = int(round(time.time()))
  data = {"host" : "localhost", "MD get" : random.randint(1, 100), "time": epoch }
  try:
    es=Elasticsearch([{'host' : '10.215.0.35', 'port' : 9200}])
    es.index(index="iofs", doc_type ="_doc", body=data)
  except e:
    print(e)
    print("Error accessing DB")
  time.sleep(1)
