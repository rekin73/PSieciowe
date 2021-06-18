#!/usr/bin/python3
from bs4 import BeautifulSoup
import requests
import re
r= requests.get(url="http://krakow.infometeo.pl/")
soup = BeautifulSoup(r.text,'html.parser')
temp=soup.find(id="meteopw").find(attrs={"title":"Temperatura rzeczywista"}).text
pattern = re.compile("-?[\d]*[.,]?[\d]* ?°C")
celc=pattern.search(temp)
if celc:
    print(celc.group())
else:
    print("error")
