#!/usr/bin/python3

import sys
import cgi
import cgitb
import os
import datetime
cgitb.enable()

def Upload(file, path_info):
	file_name = file.filename
	file_value = file.value
	file_path = path_info + file_name
	with open(file_path, 'wb') as file:
		file.write(file_value)

request_method = os.environ.get("REQUEST_METHOD")
if request_method == 'GET':
	print("Status: 403", end="\n\n")
	exit()
elif request_method == 'POST':
	content_type = os.environ.get("CONTENT_TYPE")
	content_length = int(os.environ.get("CONTENT_LENGTH"))
	path_info = os.environ.get("PATH_INFO")
	form = cgi.FieldStorage(
			fp=None,
			environ={'CONTENT_TYPE': content_type,
							'CONTENT_LENGTH':content_length,
							'REQUEST_METHOD': request_method},
			keep_blank_values=True
	)

	if not os.path.exists(path_info):
					os.makedirs(path_info)

	cnt = 0
	error_flag = False
	for file in form:
		try:
			if form[file].filename is not None and form[file].filename != "":
				print(file)
				Upload(form[file], path_info)
				cnt += 1
		except:
			error_flag = True
			break

	if error_flag == True:
		print("Status: 502", end="\n\n")
		exit()
	else:
		if cnt == 1:
			print("Location: ", end="\n")
			print("Status: 201", end="\n")
		else:
			print("Status: 200", end="\n")

	print("Content-type: text/html", end="\n")
	print("")
	html_template = """<!DOCTYPE html>
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
					color: #007bff;
				}
			</style>
	</head>
	<body>
		<div class="container">
			<h1>%s!</h1>
		</div>
	</body>
	</html>"""

	html_template = html_template % ("201 Created" if cnt == 1 else "200 OK")

	print(html_template)
