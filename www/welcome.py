#!/usr/bin/python3

import os
import sys
import cgi

# Set the content type to HTML
print("Content-type: text/html", end="\n")


#Check if the request method is POST
if os.environ.get("REQUEST_METHOD") == 'POST':
    # Read POST data to get user's name
    try:
        post_data = sys.stdin.read(int(os.environ.get("CONTENT_LENGTH")))
        post_params = post_data.split('&')
        for param in post_params:
            key, value = param.split('=')
            if key == 'username':
                username = value
                break
    except:
        # If there's an error in reading POST data, set username to "Guest"
        username = "42 Guest"
else:
		# Get user's name from form input
		form = cgi.FieldStorage()
		# Check if username exists in the query string
		if 'username' in form:
				username = form.getvalue('username')
		else:
				# If username is not provided, set it to "Guest"
				username = "42 Guest"

# HTML template for the response
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
    <h1>Thank you for evaluation, %s!</h1>
  </div>
</body>
</html>"""

html_template = html_template % username
print("", end="\n")

# Print the HTML response with the username dynamically inserted
print(html_template, end="")
