import subprocess, os

class Ardemo:
    def __init__(self, ip : str, tool_location : str, tool_file : str,rtsp_location : str ,video_port : str, capture_path : str):
        self.ardemo_path = os.path.join("ARDEMO_PATH", 'run.sh')
        self.connect_to = ip 
        self.tool_directory = tool_location
        self.tool_file = tool_file 
        self.rtsp_location = rtsp_location
        self.video_port = video_port 
        self.capture_path = capture_path 

    def run(self):
        if self.tool_directory[-1] != "/":
            self.tool_directory += "/"
            
        if self.capture_path[-1] != "/":
            self.capture_path += "/"
        subprocess.run(["bash",self.ardemo_path, "run", self.connect_to, self.tool_directory, self.tool_file, self.rtsp_location, self.video_port, self.capture_path])