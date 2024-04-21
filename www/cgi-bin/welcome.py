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
    except ValueError:
        # If there's an error in reading POST data, set username to "Guest"
        username = "Guest"
else:
		# Get user's name from form input
		form = cgi.FieldStorage()
		# Check if username exists in the query string
		if 'username' in form:
				username = form.getvalue('username')
		else:
				# If username is not provided, set it to "Guest"
				username = "Guest"

# HTML template for the response
html_template = """<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Welcome to My Web Server</title>
</head>
<body>
    <h1>Welcome to My Web Server, %s!</h1>
    <p>This is a dynamically generated HTML page served by your own web server.</p>
    <p>You can modify this file to serve any content you want.</p>
    <p>If you have any questions or need further assistance, feel free to contact me.</p>
</body>
</html>"""

html_template = html_template % username
print("", end="\n")

# Print the HTML response with the username dynamically inserted
print(html_template, end="")
