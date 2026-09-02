#!/usr/bin/env python3
import http.server
import socketserver
import json
import urllib.parse
import os
import subprocess

PORT = 8088
DIRECTORY = "/Users/gerhardwillemse/yaml_editor"

class YAMLStudioHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        parsed_url = urllib.parse.urlparse(self.path)
        path_only = parsed_url.path
        query = urllib.parse.parse_qs(parsed_url.query)

        # API: Load File Content
        if path_only == "/api/load-file":
            file_path = query.get("path", ["/Users/gerhardwillemse/esphome_webserver_di.yaml"])[0]
            try:
                if os.path.exists(file_path):
                    with open(file_path, "r", encoding="utf-8") as f:
                        content = f.read()
                    self.send_json_response({"success": True, "path": file_path, "content": content})
                else:
                    self.send_json_response({"success": False, "error": f"File not found: {file_path}"})
            except Exception as e:
                self.send_json_response({"success": False, "error": str(e)})
            return

        # API: Run ESPHome Validation Command
        if path_only == "/api/esphome-check":
            file_path = query.get("path", ["/Users/gerhardwillemse/esphome_webserver_di.yaml"])[0]
            try:
                cmd = ["/Users/gerhardwillemse/.local/bin/esphome", "config", file_path]
                res = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
                output = res.stdout if res.stdout else res.stderr
                success = (res.returncode == 0)
                self.send_json_response({"success": success, "output": output, "returncode": res.returncode})
            except Exception as e:
                self.send_json_response({"success": False, "error": str(e)})
            return

        # Standard static file handling
        return super().do_GET()

    def do_POST(self):
        parsed_url = urllib.parse.urlparse(self.path)

        # API: Save File Content to Disk
        if parsed_url.path == "/api/save-file":
            try:
                length = int(self.headers.get('Content-Length', 0))
                body = self.rfile.read(length).decode('utf-8')
                data = json.loads(body)
                file_path = data.get("path", "/Users/gerhardwillemse/esphome_webserver_di.yaml")
                content = data.get("content", "")

                with open(file_path, "w", encoding="utf-8") as f:
                    f.write(content)

                self.send_json_response({"success": True, "message": f"Successfully saved to {file_path}"})
            except Exception as e:
                self.send_json_response({"success": False, "error": str(e)})
            return

        # API: Flash to Connected ESP32
        if parsed_url.path == "/api/flash-esp32":
            try:
                length = int(self.headers.get('Content-Length', 0))
                body = self.rfile.read(length).decode('utf-8')
                data = json.loads(body)
                file_path = data.get("path", "/Users/gerhardwillemse/esphome_webserver_di.yaml")

                # Determine target USB port based on file
                device_arg = []
                if "esp32_di_sender" in file_path:
                    device_arg = ["--device", "/dev/cu.usbserial-13310"]
                elif "esp32_gateway" in file_path:
                    device_arg = ["--device", "/dev/cu.usbserial-1320"]

                cmd = ["/Users/gerhardwillemse/.local/bin/esphome", "run", file_path] + device_arg
                res = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
                output = res.stdout if res.stdout else res.stderr
                success = (res.returncode == 0)
                self.send_json_response({"success": success, "output": output, "returncode": res.returncode})
            except Exception as e:
                self.send_json_response({"success": False, "error": str(e)})
            return

        self.send_error(404, "Not Found")

    def send_json_response(self, data_dict):
        body = json.dumps(data_dict).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

if __name__ == "__main__":
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("0.0.0.0", PORT), YAMLStudioHandler) as httpd:
        print(f"🚀 ApexYAML Studio Server running at http://localhost:{PORT}")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nServer stopped.")
