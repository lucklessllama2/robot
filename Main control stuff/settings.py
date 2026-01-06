import json
import os
f = os.getcwd().split("robot")[0]+"robot"
os.chdir(f)
with open("Main control stuff/buildData.json","r") as jsonF:
    bodyData = json.load(jsonF)
