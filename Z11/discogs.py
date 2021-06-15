#!/usr/bin/python3
import requests
import sys
import time
def make_request(URL):
    r = requests.get(url = URL)

    data = r.json()
    if int(r.headers.get("X-Discogs-Ratelimit-Remaining")) <=0:
        time.sleep(60)
        r = requests.get(url = URL)
        data = r.json()
    return data
if len(sys.argv) != 2:
    print("bad argument number")
    exit()
URL = "https://api.discogs.com/artists/"

data =make_request(URL+sys.argv[1]) 

bandMembers = data['members']
bands={}

if len(bandMembers)>0:
    for member in bandMembers:
        data =make_request(member['resource_url']) 
        for band in data['groups']:
            if not band['id'] in bands:
                bands.update({band['id']:{"name":band['name'],"members":[]}})
            bands[band['id']]['members'].append(member['name'])
    del bands[int(sys.argv[1])]
    sortedBands=dict(sorted(bands.items(), key=lambda item: item[1]['name']))
    for band in sortedBands.values():
        if len(band['members']) > 1:
            print(band['name'],end=': ')
            for m in band['members']:
                print(m, end=" ")
            print("\n",end="")
