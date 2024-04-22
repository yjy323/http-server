#!/usr/bin/python3

import cgi
import cgitb
import os
import datetime
cgitb.enable()

def Upload(file, path_info):
    file_name = file.filename
    file_value = file.value
    if file_name is not None:
					file_path = path_info + file_name
					with open(file_path, 'wb') as file:
								file.write(file_value)

request_method = os.environ.get("REQUEST_METHOD")
content_type = os.environ.get("CONTENT_TYPE")
content_length = int(os.environ.get("CONTENT_LENGTH"))
path_info = os.environ.get("PATH_INFO")


form = cgi.FieldStorage(
    fp=None,
    headers={'content-type': content_type,
						 'content-length': content_length},
    environ={'REQUEST_METHOD': request_method},
    keep_blank_values=True
)

if not os.path.exists(path_info):
				os.makedirs(path_info)


error_flag = False
for file in form:
	try:
		Upload(form[file], path_info)
	except:
		error_flag = True
		break

if error_flag == True:
	print('Status: 502')
else:
	if len(form) == 1:
		print("Location: ", end="\n")
		print("Status: 201", end="\n")
	else:
		print("Status: 200", end="\n")
	print("Content-type: text/html", end="\n")
	html_template = """
				<!DOCTYPE html>
				<html lang="en">
				<head>
						<meta charset="UTF-8">
						<meta name="viewport" content="width=device-width, initial-scale=1.0">
						<title>Welcome to My Web Server</title>
						<style>
							body {
								font-family: Arial, sans-serif;
								margin: 0;
								padding: 0;
								background-color: #f0f0f0;
								color: #333;
							}

							.container {
								max-width: 800px;
								margin: 50px auto;
								padding: 20px;
								background-color: #fff;
								border-radius: 5px;
								box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
							}

							h1 {
								text-align: center;
								color: #ff2f00;
							}
						</style>
				</head>
				<body>
					<div class="container">
						<h1>%s</h1>
					</div>
				</body>
				</html>"""

	print(html_template.format("201 Created" if len(form) == 1 else "200 OK"))
