# Local Grafana Setup with Docker

# [Link to Docker Files](https://github.com/gwdg/iofs/tree/master/docker-devenv)

## How to start
- Fill out the `.env` file
- Start the `docker-compose`
- (Optional): Add some dummy data to find out whether it actually works
- Next, we have to define `influxdb` as the data source in grafana. See below.
- If not done previously, we have to enable `user_allow_other` in the `/etc/fuse.conf`
- Lastly, we have to compile `iofs` (see roots `README.md`) and run it via
```
./iofs <PATH_TO_WHERE_YOU_WRITE> <PATH_ON_WHAT_YOU_MAP> --in-server=http://localhost:8086 --in-db=mydb --in-username=influx --in-password=influxdblongpassword123
```
If you changed the parameters in `.env`, change `--in-db` accordingly.

Now you can create your first dashboard!

## (Optional): Add some dummy data

This is optional, but recommended for the following reason: If you have an empty InfluxDB, Grafana can't verify whether the connection actually works. If you configure the data source correct, you can see the 200 returned by InfluxDB in your docker logs. But since the returned JSON contains no data, Grafana interprets it as a failed fetch, thus showing a false error. So, even if optional, this will make your debugging a lot easier.

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
4. Once in the influxdb shell, use the database specified in the `.env` file.
```
USE mydb;
```
The database name should match the name specified in the `.env`. For created all databases, see
```
SHOW DATABASES;
```
5. Create some dummy data. [Here the related docs.](https://docs.influxdata.com/influxdb/v1.6/tools/shell/#write-data-to-influxdb-with-insert)
```
INSERT treasures,captain_id=pirate_king value=2
```
Note that the semicolon is missing ;)

## Add the (already initialized) influxdb to grafana
1. Start the docker-container (check with `docker container ls` afterwards).
2. Connect to grafana via <http://localhost:3000/>
3. Login with the credentials set in `.env` (`grafana`:`grafana` is default)
4. Go to `Configuration->Data Sources->Add Data Source->InfluxDB`
5. Add the following configuration:

**Query Language**: Since we explicitly use `influxdb` version 1, we have to use InfluxQL.

**URL**: Since we use docker compose (instead of the default network bridge) container name DNS is enabled. Thus we can use <http://influxc:8086>.

**InfluxDB Details**: Use the `Database`, `User`, `Password` specified in the `.env`

6. Afterwards test the configuration via `Save & test`. If everything was configured successfully, it should show an alert with `Data source is working`. If you have the `docker-compose` logs open, you should see a request with a `Grafana/x.x.x` user agent as well.
