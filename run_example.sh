mkdir -p /tmp/iofs/{logs,mounted,real}

# Comment out the classifcations if not generated

./build/src/iofs \
  --classificationfile=./blackheap_model.csv \
  --in-db=mydb \
  --in-password=influxdblongpassword123 \
  --in-server=http://localhost:8086 \
  --in-username=influx \
  --logfile=/tmp/iofs/logs/iofs.log \
  --outfile=/tmp/iofs/logs/iofs.out \
  /tmp/iofs/mounted \
  /tmp/iofs/real

