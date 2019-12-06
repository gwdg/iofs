#!/bin/bash

curl -X DELETE "http://10.215.0.35:9200/iofs"

curl -X PUT "http://10.215.0.35:9200/iofs?pretty" -H 'Content-Type: application/json' -d'
{
    "settings" : {
        "number_of_shards" : 1
    },
    "mappings" : {
       "_doc" : {
          "properties" : {
              "time" : { "type" : "date",
                "format" : "epoch_second" },
              "MD get" : { "type" : "integer"}
          }
        }
    }
}
'

