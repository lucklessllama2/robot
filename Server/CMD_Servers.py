import socket
import threading
import json
import StateSystem.State

class CMD_Server:
    def __init__(self, state:StateSystem.State.State):
        self.thread = None
        self.State:StateSystem.State.State = state
        pass
    def Loop(self):
        pass
    def decode(self, message):#figuer out what to do with the message
        # if message is a controller update
        if message.startswith("con_update"):
            decoded_data = message.replace("con_update", "")
            self.State.controller = json.loads(decoded_data)
        elif message == "getData":
            self.sendData()
    def sendData(self):
        dataDict = {
            "mac":self.getMac(),
            "ip":self.get_local_ip()
        }
        self.sendResponse(json.dumps(dataDict))
    def sendResponse(self, data):
       pass
    def runServer(self):
         self.thread.start()

    # utility stuff
    def get_local_ip():
        hostname = socket.gethostname()
        ip_address = socket.gethostbyname(hostname)
        return ip_address
    def getMac(self):
        try:  # load the json with the server data and update vars
            with open('Server/SeverData.json', "r") as jFile:
                jData = json.load(jFile)
                mac = jData["mac"]
        except FileNotFoundError:
            print("when starting UDP sever \"SeverData.json\" not found")
        except json.JSONDecodeError:
            print("error in the file \"SeverData.json\"")
        return mac