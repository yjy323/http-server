#!/usr/bin/python3

import cgi
import cgitb
import os
import datetime
cgitb.enable()

# Content-Type 헤더에서 boundary 값을 가져옴
content_type = "multipart/form-data; boundary=----Boundary123456789"  # 예시 boundary 값
boundary = content_type.split("=")[1]

# multipart/form-data 형식의 요청 본문
request_body = '''
------Boundary123456789
Content-Disposition: form-data; name="username"

user
------Boundary123456789
Content-Disposition: form-data; name="password"

pass
------Boundary123456789--
'''


form = cgi.FieldStorage(
    fp=None,
    headers={'content-type': content_type,
						 'content-length': len(request_body)},
    environ={'REQUEST_METHOD': 'POST'},
    keep_blank_values=True
)

if "username" in form:
    file_item = form["username"]
    if file_item.value:
        file_name = "username"
        file_path = 'uploads/' + "username"
        if not os.path.exists('uploads/'):
                os.makedirs('uploads/')
        with open(file_path, 'wb') as file:
            file.write(file_item.value)
