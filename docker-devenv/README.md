# Local Grafana+InfluxDB+iofs setup

## Notes
- The `iofs` influx integration still uses InfluxQL, not Flex for auth

## How to start
- Fill out the `.env` file
- Start the `docker-compose`
- Sadly, `influx:1.x` doesn't initialize a database by default, even though it is defined in via the `.env` file. Thus we have to define our own database. See below.
- Next, we have to define `influxdb` as the data source in grafana. See below.
- If not done previously, we have to enable `user_allow_other` in the `/etc/fuse.conf`
- Lastly, we have to compile `iofs` (see roots `README.md`) and run it via
```
./iofs <PATH_TO_WHERE_YOU_WRITE> <PATH_ON_WHAT_YOU_MAP> --in-server=http://localhost:8086 --in-db=mydb
```
If you changed `INFLUX_DB` in `.env`, change `--in-db` accordingly.

Now you can create your first dashboard!

## Create initial database manually
1. Start the docker-container (check with `docker container ls` afterwards).
2. Get a shell into the influxdb container. By default, the container is named `influxc`:
```
docker exec -it influxc bash
```
3. Now manually connect to the influxdb-cli. Use credentials according to the `.env` file.
```
influx -username influx -password influxdblongpassword123 -precision rfc3339
```
The precision defines the date format and `rfc3339` is used everywhere in the documentation
4. Once in the influxdb shell, create a database with
```
CREATE DATABASE mydb;
```
The database name should match the name specified in the `.env`. Verify that it worked with
```
SHOW DATABASES;
```

## Add the (already initialized) influxdb to grafana
1. Start the docker-container (check with `docker container ls` afterwards).
2. Connect to grafana via http://localhost:3000/
3. Login with the credentials set in `.env` (`grafana`:`grafana` is default)
4. Go to `Configuration->Data Sources->Add Data Source->InfluxDB`
5. Add the following configuration:

**Query Language**: Since we explicitly use `influxdb` version 1, we have to use InfluxQL.

**URL**: Since we use docker compose (instead of the default network bridge) container name DNS is enabled. Thus we can use `http://influxc:8086`.

**InfluxDB Details**: Use the `Database`, `User`, `Password` specified in the `.env`

6. Afterwards test the configuration via `Save & test`. If everything was configured successfully, it should show an alert with `Data source is working`. If you have the `docker-compose` logs open, you should see a request with a `Grafana/x.x.x` user agent as well.
