from locust import task, constant, FastHttpUser
from uuid import uuid4


class WebsiteUser(FastHttpUser):
    wait_time = constant(1)
    
    @ task
    def echo(self):
        msg = uuid4().hex
        res = self.client.get("/echo?message="+msg, name="/echo?message=[message]")
        if res.status_code != 200 or res.json()["message"] != msg:
            self.environment.runner.stop()
