#!/bin/bash

./thrones_server & disown
./thrones -d -c & disown
